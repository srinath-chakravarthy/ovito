///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2014) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#include <plugins/pyscript/PyScript.h>
#include <plugins/pyscript/binding/PythonBinding.h>
#include <core/plugins/PluginManager.h>
#include <core/app/Application.h>
#include <core/dataset/DataSetContainer.h>
#include "ScriptEngine.h"

namespace PyScript {

/// Points to the script engine that is currently active (i.e. which is executing a script).
ScriptEngine* ScriptEngine::_activeEngine = nullptr;

/// Head of linked list containing all initXXX functions. 
PythonPluginRegistration* PythonPluginRegistration::linkedlist = nullptr;

/******************************************************************************
* Sets the dataset that is currently active in the Python interpreter.
******************************************************************************/
void ScriptEngine::setActiveDataset(DataSet* dataset)
{
	OVITO_ASSERT(dataset);
	OVITO_ASSERT(dataset->container());

	// Add an attribute to the ovito module that provides access to the active dataset.
	py::module ovito_module = py::module::import("ovito");
	py::setattr(ovito_module, "dataset", py::cast(dataset, py::return_value_policy::reference));

	// Add an attribute to the ovito module that provides access to the global task manager.
	py::setattr(ovito_module, "task_manager", py::cast(&dataset->container()->taskManager(), py::return_value_policy::reference));
}

/******************************************************************************
* Returns the dataset that is currently active in the Python interpreter.
******************************************************************************/
DataSet* ScriptEngine::activeDataset()
{
	// Read the ovito module's attribute that provides access to the active dataset.
	py::module ovito_module = py::module::import("ovito");
	return py::cast<DataSet*>(py::getattr(ovito_module, "dataset", py::none()));
}

/******************************************************************************
* Returns the task manager that is currently active in the Python interpreter.
******************************************************************************/
TaskManager& ScriptEngine::activeTaskManager()
{
	// Read the ovito module's attribute that provides access to the active dataset.
	py::module ovito_module = py::module::import("ovito");
	TaskManager* taskManager = py::cast<TaskManager*>(py::getattr(ovito_module, "task_manager", py::none()));
	if(!taskManager) throw Exception(tr("Invalid OVITO context state: There is no active task manager. This should not happen. Please contact the developer."));
	return *taskManager;
}

/******************************************************************************
* Initializes the scripting engine and sets up the environment.
******************************************************************************/
ScriptEngine::ScriptEngine(DataSet* dataset, TaskManager& taskManager, bool privateContext, QObject* parent)
	: QObject(parent), _dataset(dataset), _taskManager(&taskManager)
{
	try {
		// Initialize our embedded Python interpreter if it isn't running already.
		if(!Py_IsInitialized())
			initializeEmbeddedInterpreter();
		
		// Import the main module and get a reference to the main namespace.
		// Make a local copy of the global main namespace for this execution context.
		// The original namespace dictionary is not touched.
		py::object main_module = py::module::import("__main__");
		if(privateContext) {
			_mainNamespace = py::getattr(main_module, "__dict__").attr("copy")();
		}
		else {
			_mainNamespace = py::getattr(main_module, "__dict__");
		}
		
		// Add the 'dataset' attribute to the ovito module that provides access to the active dataset.
		setActiveDataset(dataset);
	}
	catch(py::error_already_set& ex) {
		ex.restore();
		if(PyErr_Occurred())
			PyErr_PrintEx(0);
		throw Exception(tr("Failed to initialize Python interpreter."), dataset);
	}
	catch(Exception& ex) {
		ex.setContext(dataset);
		throw;
	}
	catch(const std::exception& ex) {
		throw Exception(tr("Failed to initialize Python interpreter. %1").arg(ex.what()), dataset);
	}
}

/******************************************************************************
* Destructor.
******************************************************************************/
ScriptEngine::~ScriptEngine()
{
	if(_activeEngine == this) {
		qWarning() << "Deleting active script engine.";
		_activeEngine = nullptr;
	}
	try {
		// Explicitly release all objects created by Python scripts.
		if(_mainNamespace) _mainNamespace.clear();
	}
	catch(py::error_already_set& ex) {
		ex.restore();
		PyErr_PrintEx(0);
	}
}

/******************************************************************************
* Initializes the embedded Python interpreter and sets up the global namespace.
******************************************************************************/
void ScriptEngine::initializeEmbeddedInterpreter()
{
	// This is a one-time global initialization.
	static bool isInterpreterInitialized = false; 
	if(isInterpreterInitialized)
		return;	// Interpreter is already initialized.

	try {

		// Call Py_SetProgramName() because the Python interpreter uses the path of the main executable to determine the
		// location of Python standard library, which gets shipped with the static build of OVITO.
#if PY_MAJOR_VERSION >= 3
		static std::wstring programName = QDir::toNativeSeparators(QCoreApplication::applicationFilePath()).toStdWString();
		Py_SetProgramName(const_cast<wchar_t*>(programName.data()));
#else
		static QByteArray programName = QDir::toNativeSeparators(QCoreApplication::applicationFilePath()).toLocal8Bit();
		Py_SetProgramName(programName.data());
#endif

		// Make our internal script modules available by registering their initXXX functions with the Python interpreter.
		// This is required for static builds where all Ovito plugins are linked into the main executable file.
		// On Windows this is needed, because OVITO plugins have an .dll extension and the Python interpreter 
		// only looks for modules that have a .pyd extension.
		for(PythonPluginRegistration* r = PythonPluginRegistration::linkedlist; r != nullptr; r = r->_next) {
			// Note: const_cast<> is for backward compatibility with Python 2.6.
			PyImport_AppendInittab(const_cast<char*>(r->_moduleName.c_str()), r->_initFunc);
		}

		// Initialize the Python interpreter.
		Py_Initialize();

		py::module sys_module = py::module::import("sys");

		// Install output redirection (don't do this in console mode as it interferes with the interactive interpreter).
		if(Application::instance()->guiMode()) {
			// Register the output redirector class.
			py::class_<InterpreterStdOutputRedirector>(sys_module, "__StdOutStreamRedirectorHelper")
					.def("write", &InterpreterStdOutputRedirector::write)
					.def("flush", &InterpreterStdOutputRedirector::flush);
			py::class_<InterpreterStdErrorRedirector>(sys_module, "__StdErrStreamRedirectorHelper")
					.def("write", &InterpreterStdErrorRedirector::write)
					.def("flush", &InterpreterStdErrorRedirector::flush);
			// Replace stdout and stderr streams.
			sys_module.attr("stdout") = py::cast(new InterpreterStdOutputRedirector(), py::return_value_policy::take_ownership);
			sys_module.attr("stderr") = py::cast(new InterpreterStdErrorRedirector(), py::return_value_policy::take_ownership);
		}

		// Determine path where Python source files are located.
		QDir prefixDir(QCoreApplication::applicationDirPath());
#if defined(Q_OS_WIN)
		QString pythonModulePath = prefixDir.absolutePath() + QStringLiteral("/plugins/python");
#elif defined(Q_OS_MAC)
		QString pythonModulePath = prefixDir.absolutePath() + QStringLiteral("/../Resources/python");
#else
		QString pythonModulePath = prefixDir.absolutePath() + QStringLiteral("/../lib/ovito/plugins/python");
#endif

		// Prepend directory containing OVITO's Python source files to sys.path.
		py::object sys_path = sys_module.attr("path");
		PyList_Insert(sys_path.ptr(), 0, py::cast(QDir::toNativeSeparators(pythonModulePath)).ptr());

		// Prepend current directory to sys.path.
		PyList_Insert(sys_path.ptr(), 0, py::str().ptr());
	}
	catch(const Exception&) {
		throw;
	}
	catch(py::error_already_set& ex) {
		ex.restore();
		if(PyErr_Occurred())
			PyErr_PrintEx(0);
		throw Exception(tr("Failed to initialize Python interpreter. %1").arg(ex.what()), dataset());
	}
	catch(const std::exception& ex) {
		throw Exception(tr("Failed to initialize Python interpreter: %1").arg(ex.what()), dataset());
	}
	catch(...) {
		throw Exception(tr("Unhandled exception thrown by Python interpreter."), dataset());
	}

	isInterpreterInitialized = true;
}

/******************************************************************************
* Executes one or more Python statements.
******************************************************************************/
int ScriptEngine::executeCommands(const QString& commands, const QStringList& scriptArguments)
{
	if(QCoreApplication::instance() && QThread::currentThread() != QCoreApplication::instance()->thread())
		throw Exception(tr("Can run Python scripts only from the main thread."), dataset());

	// Activate this engine.
	ActiveScriptEngineSetter engineSetter(this);

	try {
		// Pass command line parameters to the script.
		py::list argList;
		argList.append(py::cast("-c"));
		for(const QString& a : scriptArguments)
			argList.append(py::cast(a));
		py::module::import("sys").attr("argv") = argList;

		_mainNamespace["__file__"] = py::none();
		PyObject* result = PyRun_String(commands.toUtf8().constData(), Py_file_input, _mainNamespace.ptr(), _mainNamespace.ptr());
		if(!result) throw py::error_already_set();
		Py_XDECREF(result);

		return 0;
	}
	catch(py::error_already_set& ex) {
		return handlePythonException(ex);
	}
	catch(Exception& ex) {
	    ex.setContext(dataset());
		throw;
	}
	catch(const std::exception& ex) {
		throw Exception(tr("Script execution error: %1").arg(ex.what()), dataset());
	}
	catch(...) {
		throw Exception(tr("Unhandled exception thrown by Python interpreter."), dataset());
	}
}

/******************************************************************************
* Executes the given C++ function, which in turn may invoke Python functions in the
* context of this engine, and catches possible exceptions.
******************************************************************************/
void ScriptEngine::execute(const std::function<void()>& func)
{
	if(QCoreApplication::instance() && QThread::currentThread() != QCoreApplication::instance()->thread())
		throw Exception(tr("Can run Python scripts only from the main thread."), dataset());

	// Activate this engine.
	ActiveScriptEngineSetter engineSetter(this);

	try {
		func();
	}
	catch(py::error_already_set& ex) {
		handlePythonException(ex);
	}
	catch(Exception& ex) {
	    ex.setContext(dataset());
		throw;
	}
	catch(const std::exception& ex) {
		throw Exception(tr("Script execution error: %1").arg(ex.what()), dataset());
	}
	catch(...) {
		throw Exception(tr("Unhandled exception thrown by Python interpreter."), dataset());
	}
}

/******************************************************************************
* Calls a callable Python object (typically a function).
******************************************************************************/
py::object ScriptEngine::callObject(const py::object& callable, const py::tuple& arguments, const py::dict& kwargs)
{
	py::object result;
	execute([&result, &callable, &arguments, &kwargs]() {
		result = callable(*arguments, **kwargs);
	});
	return result;
}

/******************************************************************************
* Executes a Python program.
******************************************************************************/
int ScriptEngine::executeFile(const QString& filename, const QStringList& scriptArguments)
{
	if(QThread::currentThread() != QCoreApplication::instance()->thread())
		throw Exception(tr("Can run Python scripts only from the main thread."), dataset());

	// Activate this engine.
	ActiveScriptEngineSetter engineSetter(this);

	try {
		// Pass command line parameters to the script.
		py::list argList;
		argList.append(py::cast(filename));
		for(const QString& a : scriptArguments)
			argList.append(py::cast(a));
		py::module::import("sys").attr("argv") = argList;

		py::str nativeFilename(py::cast(QDir::toNativeSeparators(filename)));
		_mainNamespace["__file__"] = nativeFilename;
		py::eval_file(nativeFilename, _mainNamespace, _mainNamespace);

	    return 0;
	}
	catch(py::error_already_set& ex) {
		return handlePythonException(ex, filename);
	}
	catch(Exception& ex) {
	    ex.setContext(dataset());
		throw;
	}
	catch(const std::exception& ex) {
		throw Exception(tr("Script execution error: %1").arg(ex.what()), dataset());
	}
	catch(...) {
		throw Exception(tr("Unhandled exception thrown by Python interpreter."), dataset());
	}
}

/******************************************************************************
* Handles an exception raised by the Python side.
******************************************************************************/
int ScriptEngine::handlePythonException(py::error_already_set& ex, const QString& filename)
{
	ex.restore();

	// Handle calls to sys.exit()
	if(PyErr_ExceptionMatches(PyExc_SystemExit)) {
		return handleSystemExit();
	}

	// Prepare C++ exception object.
	Exception exception(filename.isEmpty() ? 
		tr("The Python script has exited with an error.") :
		tr("The Python script '%1' has exited with an error.").arg(filename), dataset());

	// Retrieve Python error message and traceback.
	if(Application::instance()->guiMode()) {
		PyObject* extype;
		PyObject* value;
		PyObject* traceback;
		PyErr_Fetch(&extype, &value, &traceback);
		PyErr_NormalizeException(&extype, &value, &traceback);
		if(extype) {
			py::object o_extype = py::reinterpret_borrow<py::object>(extype);
			py::object o_value = py::reinterpret_borrow<py::object>(value);
			try {
				if(traceback) {
					py::object o_traceback = py::reinterpret_borrow<py::object>(traceback);
					py::object mod_traceback = py::module::import("traceback");
					bool chain = PyObject_IsInstance(value, extype) == 1;
					py::sequence lines = mod_traceback.attr("format_exception")(o_extype, o_value, o_traceback, py::none(), chain);
					if(py::isinstance<py::sequence>(lines)) {
						QString tracebackString;
						for(int i = 0; i < py::len(lines); ++i)
							tracebackString += lines[i].cast<QString>();
						exception.appendDetailMessage(tracebackString);
					}
				}
				else {
					exception.appendDetailMessage(py::str(o_value).cast<QString>());
				}
			}
			catch(py::error_already_set& ex) {
				ex.restore();
				PyErr_PrintEx(0);
			}
		}
	}
	else {
		// Print error message to the console.
		PyErr_PrintEx(0);
	}

	// Raise C++ exception.
	throw exception;
}

/******************************************************************************
* Handles a call to sys.exit() in the Python interpreter.
* Returns the program exit code.
******************************************************************************/
int ScriptEngine::handleSystemExit()
{
	PyObject *exception, *value, *tb;
	int exitcode = 0;

	PyErr_Fetch(&exception, &value, &tb);
#if PY_MAJOR_VERSION < 3
	if(Py_FlushLine())
		PyErr_Clear();
#endif
	if(value && value != Py_None) {
#ifdef PyExceptionInstance_Check
		if(PyExceptionInstance_Check(value)) {	// Python 2.6 or newer
#else
		if(PyInstance_Check(value)) {			// Python 2.4
#endif
			// The error code should be in the code attribute.
			PyObject *code = PyObject_GetAttrString(value, "code");
			if(code) {
				Py_DECREF(value);
				value = code;
				if(value == Py_None)
					goto done;
			}
			// If we failed to dig out the 'code' attribute, just let the else clause below print the error.
		}
#if PY_MAJOR_VERSION >= 3
		if(PyLong_Check(value))
			exitcode = (int)PyLong_AsLong(value);
#else
		if(PyInt_Check(value))
			exitcode = (int)PyInt_AsLong(value);
#endif
		else {
			py::str s(value);
			Q_EMIT scriptError(s.cast<QString>() + QChar('\n'));
			exitcode = 1;
		}
	}

done:
	PyErr_Restore(exception, value, tb);
	PyErr_Clear();
	return exitcode;
}

};
