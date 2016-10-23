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

#ifndef __OVITO_PYSCRIPT_ENGINE_H
#define __OVITO_PYSCRIPT_ENGINE_H

#include <plugins/pyscript/PyScript.h>
#include <core/dataset/DataSet.h>

namespace PyScript {

using namespace Ovito;

/**
 * \brief A scripting engine that provides bindings to OVITO's C++ classes.
 */
class OVITO_PYSCRIPT_EXPORT ScriptEngine : public QObject
{
public:

	/// \brief Initializes the scripting engine and sets up the environment.
	/// \param dataset The engine will execute scripts in the context of this dataset.
	/// \param parent The owner of this QObject.
	/// \param redirectOutputToConsole Controls whether the Python script output should be forwarded to the terminal.
	ScriptEngine(DataSet* dataset, QObject* parent = nullptr, bool redirectOutputToConsole = true);

	/// \brief Destructor
	virtual ~ScriptEngine();

	/// \brief Returns the dataset that provides the context for the script.
	DataSet* dataset() const { return _dataset; }

	/// \brief Returns the script engine that is currently active (i.e. which is executing a script).
	/// \return The active script engine or NULL if no script is currently being executed.
	static ScriptEngine* activeEngine() { return _activeEngine; }

	/// \brief Executes a Python script consisting of one or more statements.
	/// \param script The script commands.
	/// \param scriptArguments An optional list of command line arguments that will be passed to the script via sys.argv.
	/// \param progressDisplay An optional progress display, which will be used to show the script execution status.
	/// \return The exit code returned by the Python script.
	/// \throw Exception on error.
	int executeCommands(const QString& commands, const QStringList& scriptArguments = QStringList());

	/// \brief Executes a Python script file.
	/// \param scriptFile The script file path.
	/// \param scriptArguments An optional list of command line arguments that will be passed to the script via sys.argv.
	/// \param progressDisplay An optional progress display, which will be used to show the script execution status.
	/// \return The exit code returned by the Python script.
	/// \throw Exception on error.
	int executeFile(const QString& file, const QStringList& scriptArguments = QStringList());

	/// \brief Calls a callable Python object (typically a function).
	/// \param callable The callable object.
	/// \param arguments The list of function arguments.
	/// \param kwargs The keyword arguments to be passed to the function.
	/// \return The value returned by the Python function.
	/// \throw Exception on error.
	boost::python::object callObject(const boost::python::object& callable, const boost::python::tuple& arguments = boost::python::tuple(), const boost::python::dict& kwargs = boost::python::dict());

	/// \brief Executes the given C++ function, which in turn may invoke Python functions in the context of this engine.
	void execute(const std::function<void()>& func);

	/// \brief Provides access to the global namespace the script will be executed in by this script engine.
	boost::python::dict& mainNamespace() { return _mainNamespace; }

	/// Returns the progress display that has been registered with the engine, which
	/// is responsible for showing the progress of long-running operations
	/// performed by a script.
	AbstractProgressDisplay* progressDisplay() const { return _progressDisplay; }

	/// Sets the progress display which will be used to show the progress of long-running operations
	/// performed by a script.
	void setProgressDisplay(AbstractProgressDisplay* progressDisplay) { _progressDisplay = progressDisplay; }

Q_SIGNALS:

	/// This signal is emitted when the Python script writes to the sys.stdout stream.
	void scriptOutput(const QString& outputString);

	/// This is emitted when the Python script writes to the sys.stderr stream.
	void scriptError(const QString& errorString);

private:

	/// Initializes the Python interpreter and sets up the global namespace.
	static void initializeInterpreter();

	/// Handles a call to sys.exit() in the Python interpreter.
	/// Returns the program exit code.
	int handleSystemExit();

	/// This helper class redirects Python script write calls to the sys.stdout stream to this script engine.
	struct InterpreterStdOutputRedirector {
		void write(const QString& str) {
			if(_activeEngine) _activeEngine->scriptOutput(str);
			else std::cout << str.toStdString();
		}
		void flush() {
			if(!_activeEngine) std::cout << std::flush;
		}
	};

	/// This helper class redirects Python script write calls to the sys.stderr stream to this script engine.
	struct InterpreterStdErrorRedirector {
		void write(const QString& str) {
			if(_activeEngine) _activeEngine->scriptError(str);
			else std::cerr << str.toStdString();
		}
		void flush() {
			if(!_activeEngine) std::cerr << std::flush;
		}
	};

	/// The dataset that provides the context for the script execution.
	OORef<DataSet> _dataset;

	/// The namespace (scope) the script will be executed in by this script engine.
	boost::python::dict _mainNamespace;

	/// The progress display that has been registered with the engine, which
	/// is responsible for showing the progress of long-running operations
	/// performed by a script.
	AbstractProgressDisplay* _progressDisplay = nullptr;

	/// Flag that indicates whether the global Python interpreter has been initialized.
	static bool _isInterpreterInitialized;

	/// The script engine that is currently active (i.e. which is executing a script).
	static ScriptEngine* _activeEngine;

	Q_OBJECT
};

};	// End of namespace

#endif
