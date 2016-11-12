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
#include <core/utilities/concurrent/ProgressDisplay.h>
#include "ScriptEngine.h"

namespace PyScript {

/// Flag that indicates whether the global Python interpreter has been initialized.
bool ScriptEngine::_isInterpreterInitialized = false;

/// The script engine that is currently active (i.e. which is executing a script).
ScriptEngine* ScriptEngine::_activeEngine = nullptr;

/// Head of linked list that contains all initXXX functions.
PythonPluginRegistration* PythonPluginRegistration::linkedlist = nullptr;

/******************************************************************************
* Initializes the scripting engine and sets up the environment.
******************************************************************************/
ScriptEngine::ScriptEngine(DataSet* dataset, QObject* parent, bool redirectOutputToConsole)
	: QObject(parent), _dataset(dataset)
{
	try {
		// Initialize the underlying Python interpreter if it isn't initialized already.
		initializeInterpreter();
	}
	catch(Exception& ex) {
		ex.setContext(dataset);
		throw;
	}

	// Initialize state of the script engine.
	try {
		// Import the main module and get a reference to the main namespace.
		// Make a local copy of the global main namespace for this engine.
		// The original namespace dictionary is not touched.
		py::object main_module = py::module::import("__main__");
		_mainNamespace = main_module.attr("__dict__").attr("copy")();

		// Add a reference to the current dataset to the namespace.
        PyObject* mod = PyImport_ImportModule("ovito");
        if(!mod) throw py::error_already_set();
		py::module ovito_module(mod, false);
        py::setattr(ovito_module, "dataset", py::cast(dataset));
	}
	catch(py::error_already_set& ex) {
		ex.restore();
		if(PyErr_Occurred())
			PyErr_Print();
		throw Exception(tr("Failed to initialize Python interpreter."), dataset);
	}
	catch(const std::exception& ex) {
		throw Exception(tr("Failed to initialize Python interpreter. %1").arg(ex.what()), dataset);
	}

	// Install default signal handlers for Python script output, which forward the script output to the host application's stdout/stderr.
	if(redirectOutputToConsole) {
		connect(this, &ScriptEngine::scriptOutput, [](const QString& str) { std::cout << str.toLocal8Bit().constData(); });
		connect(this, &ScriptEngine::scriptError, [](const QString& str) { std::cerr << str.toLocal8Bit().constData(); });
	}
}

/******************************************************************************
* Destructor.
******************************************************************************/
ScriptEngine::~ScriptEngine()
{
	try {
		// Explicitly release all objects created by Python scripts.
		_mainNamespace.clear();
	}
	catch(py::error_already_set& ex) {
		ex.restore();
		PyErr_Print();
	}
}

/******************************************************************************
* Initializes the Python interpreter and sets up the global namespace.
******************************************************************************/
void ScriptEngine::initializeInterpreter()
{
	// This is a one-time global initialization. 
	if(_isInterpreterInitialized)
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
		// This is always required for static builds where all Ovito plugins are linked into the main executable file.
		// On Windows this pre-registration is also needed, because OVITO plugin dynamic libraries have an .dll extension and the Python interpreter 
		// can only find modules that have a .pyd extension.
		for(PythonPluginRegistration* r = PythonPluginRegistration::linkedlist; r != nullptr; r = r->_next) {
			// Note: "const_cast" is for backward compatibility with Python 2.6
			PyImport_AppendInittab(const_cast<char*>(r->_moduleName), r->_initFunc);
		}

		// Initialize the Python interpreter.
		Py_Initialize();

		py::object sys_module = py::module::import("sys");

		// Install output redirection (don't do this in console mode as it interferes with the interactive interpreter).
		if(Application::instance().guiMode()) {
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

		// Install Ovito to Python exception translator.
		py::register_exception_translator([](std::exception_ptr p) {
			try {
				if(p) std::rethrow_exception(p);
			}
			catch(const Exception& ex) {
				PyErr_SetString(PyExc_RuntimeError, ex.messages().join(QChar('\n')).toUtf8().constData());
			}
		});

		// Prepend directories containing OVITO's Python modules to sys.path.
		py::list sys_path = sys_module.attr("path").cast<py::list>();

		for(const QDir& pluginDir : PluginManager::instance().pluginDirs()) {
			py::object path = py::cast(QDir::toNativeSeparators(pluginDir.absolutePath() + "/python"));
			PyList_Insert(sys_path.ptr(), 0, path.ptr());
		}

		// Prepend current directory to sys.path.
		sys_path.attr("insert")(0, "");
	}
	catch(const Exception&) {
		throw;
	}
	catch(py::error_already_set& ex) {
		ex.restore();
		if(PyErr_Occurred())
			PyErr_Print();
		throw Exception(tr("Failed to initialize Python interpreter. %1").arg(ex.what()), dataset());
	}
	catch(const std::exception& ex) {
		throw Exception(tr("Failed to initialize Python interpreter: %1").arg(ex.what()), dataset());
	}
	catch(...) {
		throw Exception(tr("Unhandled exception thrown by Python interpreter."), dataset());
	}

	_isInterpreterInitialized = true;
}

/******************************************************************************
* Executes one or more Python statements.
******************************************************************************/
int ScriptEngine::executeCommands(const QString& commands, const QStringList& scriptArguments)
{
	if(QThread::currentThread() != QCoreApplication::instance()->thread())
		throw Exception(tr("Can run Python scripts only from the main thread."));

	if(_mainNamespace.is_none())
		throw Exception(tr("Python script engine is not initialized."), dataset());

	// Remember the script engine that was active so we can restore it later.
	ScriptEngine* previousEngine = _activeEngine;
	_activeEngine = this;

	try {
		// Pass command line parameters to the script.
		py::list argList;
		argList.append(py::cast("-c"));
		for(const QString& a : scriptArguments)
			argList.append(py::cast(a));
		py::module::import("sys").attr("argv") = argList;

		_mainNamespace["__file__"] = py::none();
		py::eval<py::eval_statements>(py::cast(commands), _mainNamespace, _mainNamespace);
		_activeEngine = previousEngine;
		return 0;
	}
	catch(py::error_already_set& ex) {
		ex.restore();
		if(PyErr_Occurred()) {
			// Handle call to sys.exit()
			if(PyErr_ExceptionMatches(PyExc_SystemExit)) {
				_activeEngine = previousEngine;
				return handleSystemExit();
			}
			PyErr_Print();
		}
	    _activeEngine = previousEngine;
		throw Exception(tr("Script execution error: %1").arg(ex.what()), dataset());
	}
	catch(Exception& ex) {
	    _activeEngine = previousEngine;
	    ex.setContext(dataset());
		throw;
	}
	catch(const std::exception& ex) {
	    _activeEngine = previousEngine;
		throw Exception(tr("Script execution error: %1").arg(ex.what()), dataset());
	}
	catch(...) {
	    _activeEngine = previousEngine;
		throw Exception(tr("Unhandled exception thrown by Python interpreter."), dataset());
	}
}

/******************************************************************************
* Executes the given C++ function, which in turn may invoke Python functions in the
* context of this engine, and catches possible exceptions.
******************************************************************************/
void ScriptEngine::execute(const std::function<void()>& func)
{
	if(QThread::currentThread() != QCoreApplication::instance()->thread())
		throw Exception(tr("Can run Python scripts only from the main thread."));

	if(_mainNamespace.is_none())
		throw Exception(tr("Python script engine is not initialized."), dataset());

	// Remember the script engine that was active so we can restore it later.
	ScriptEngine* previousEngine = _activeEngine;
	_activeEngine = this;

	try {
		func();
	    _activeEngine = previousEngine;
	}
	catch(py::error_already_set& ex) {
		ex.restore();
		if(PyErr_Occurred())
			PyErr_Print();
	    _activeEngine = previousEngine;
		throw Exception(tr("Python interpreter has exited with an error. See interpreter output for details."), dataset());
	}
	catch(Exception& ex) {
	    _activeEngine = previousEngine;
	    ex.setContext(dataset());
		throw;
	}
	catch(const std::exception& ex) {
	    _activeEngine = previousEngine;
		throw Exception(tr("Script execution error: %1").arg(QString::fromLocal8Bit(ex.what())), dataset());
	}
	catch(...) {
	    _activeEngine = previousEngine;
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
		throw Exception(tr("Can run Python scripts only from the main thread."));

	if(_mainNamespace.is_none())
		throw Exception(tr("Python script engine is not initialized."), dataset());

	// Remember the script engine that was active so we can restore it later.
	ScriptEngine* previousEngine = _activeEngine;
	_activeEngine = this;

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

	    _activeEngine = previousEngine;
	    return 0;
	}
	catch(py::error_already_set& ex) {		
		ex.restore();

		// Handle calls to sys.exit()
		if(PyErr_ExceptionMatches(PyExc_SystemExit)) {
		    _activeEngine = previousEngine;
			return handleSystemExit();
		}

		// Prepare C++ exception object.
	    Exception exception(tr("The Python script '%1' has exited with an error.").arg(filename), dataset());

		// Retrieve Python error message and traceback.
	    if(Application::instance().guiMode()) {
			PyObject* extype;
			PyObject* value;
			PyObject* traceback;
			PyErr_Fetch(&extype, &value, &traceback);
			PyErr_NormalizeException(&extype, &value, &traceback);
			if(extype) {
				py::object o_extype(extype, true);
				py::object o_value(value, true);
				try {
					if(traceback) {
						py::object o_traceback(traceback, true);
						py::object mod_traceback = py::module::import("traceback");
						bool chain = PyObject_IsInstance(value, extype) == 1;
						py::sequence lines(mod_traceback.attr("format_exception")(o_extype, o_value, o_traceback, py::none(), chain));
						if(lines.check()) {
							QString tracebackString;
							for(int i = 0; i < py::len(lines); ++i)
								tracebackString += lines[i].cast<QString>();
							exception.appendDetailMessage(tracebackString);
						}
					}
					else {
						exception.appendDetailMessage(o_value.str().cast<QString>());
					}
				}
				catch( py::error_already_set& ex) {
					ex.restore();
					PyErr_Print();
				}
			}
	    }
		else {
			// Print error message to the console.
			PyErr_Print();
		}

		// Deactivate script engine.
	    _activeEngine = previousEngine;

	    // Raise C++ exception.
	    throw exception;
	}
	catch(Exception& ex) {
	    _activeEngine = previousEngine;
	    ex.setContext(dataset());
		throw;
	}
	catch(const std::exception& ex) {
	    _activeEngine = previousEngine;
		throw Exception(tr("Script execution error: %1").arg(QString::fromLocal8Bit(ex.what())), dataset());
	}
	catch(...) {
	    _activeEngine = previousEngine;
		throw Exception(tr("Unhandled exception thrown by Python interpreter."), dataset());
	}
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
	if(value == NULL || value == Py_None)
		goto done;
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
			if (value == Py_None)
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
		py::str s(PyObject_Str(value), false);
		QString errorMsg;
		if(s.check())
			errorMsg = s.cast<QString>() + QChar('\n');
		if(!errorMsg.isEmpty())
			Q_EMIT scriptError(errorMsg);
		exitcode = 1;
	}

done:
	PyErr_Restore(exception, value, tb);
	PyErr_Clear();
	return exitcode;
}

};
