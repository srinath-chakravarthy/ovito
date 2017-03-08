///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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

#pragma once


#include <core/Core.h>
#include <core/utilities/Exception.h>

namespace Ovito {

/**
 * \brief The main application.
 */
class OVITO_CORE_EXPORT Application : public QObject
{
	Q_OBJECT
	
public:

	/// \brief Returns the one and only instance of this class.
	inline static Application* instance() { return _instance; }

	/// \brief Constructor.
	Application();

	/// \brief Destructor.
	virtual ~Application();

	/// \brief Initializes the application.
	/// \return \c true if the application was initialized successfully;
	///         \c false if an error occurred and the program should be terminated.
	bool initialize();

	/// \brief Handler method for Qt error messages.
	///
	/// This can be used to set a debugger breakpoint for the OVITO_ASSERT macros.
	static void qtMessageOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg);

	/// \brief Returns whether the application has been started in graphical mode.
	/// \return \c true if the application should use a graphical user interface;
	///         \c false if the application has been started in the non-graphical console mode.
	bool guiMode() const { return !_consoleMode; }

	/// \brief Returns whether the application has been started in console mode.
	/// \return \c true if the application has been started in the non-graphical console mode;
	///         \c false if the application should use a graphical user interface.
	bool consoleMode() const { return _consoleMode; }

	/// \brief Returns whether the application runs in headless mode (without an X server on Linux and no OpenGL support).
	bool headlessMode() const { return _headlessMode; }

	/// \brief When in console mode, this specifies the exit code that will be returned by the application on shutdown.
	void setExitCode(int code) { _exitCode = code; }

	/// \brief Returns a pointer to the main dataset container.
	/// \return The dataset container of the first main window when running in GUI mode;
	///         or the global dataset container when running in console mode.
	DataSetContainer* datasetContainer() const;

	/// Returns the global FileManager class instance.
	FileManager* fileManager() const { return _fileManager.get(); }

	/// This registers a functor object to be called after all events in the UI event queue have been processed and
	/// before control returns to the event loop. For a given target object, only one functor can be registered at a
	/// time. Subsequent calls to runOnceLater() with the same target, before control returns to the event loop, will do nothing.
	template<class F>
	void runOnceLater(QObject* target, F&& func) {
		if(_runOnceList.isEmpty())
			metaObject()->invokeMethod(this, "processRunOnceList", Qt::QueuedConnection);
		else if(_runOnceList.contains(target))
			return;
		_runOnceList.insert(target, std::forward<F>(func));
	}

	/// Returns the list of auto-start objects created at application startup.
	const std::vector<OORef<AutoStartObject>>& autostartObjects() const { return _autostartObjects; }

	/// Returns the number of parallel threads to be used by the application when doing computations.
	int idealThreadCount() const { return _idealThreadCount; }

	/// Sets the number of parallel threads to be used by the application when doing computations.
	void setIdealThreadCount(int count) { _idealThreadCount = std::max(1, count); }

	/// Returns the major version number of the application.
	static int applicationVersionMajor();

	/// Returns the minor version number of the application.
	static int applicationVersionMinor();

	/// Returns the revision version number of the application.
	static int applicationVersionRevision();

	/// Create the global instance of the right QCoreApplication derived class.
	virtual void createQtApplication(int& argc, char** argv);

	/// Handler function for exceptions.
	virtual void reportError(const Exception& exception, bool blocking);

private:

	/// Executes the functions registered with the runOnceLater() function.
	/// This method is called after the events in the event queue have been processed.
	Q_INVOKABLE void processRunOnceList();

protected:

	/// Creates the global FileManager class instance.
	virtual FileManager* createFileManager();

	/// Indicates that the application is running in console mode.
	bool _consoleMode;

	/// Indicates that the application is running in headless mode (without OpenGL support).
	bool _headlessMode;

	/// In console mode, this is the exit code returned by the application on shutdown.
	int _exitCode;

	/// The list of functor objects registered with runOnceLater(), which are to be executed as soon as control returns to the event loop.
	QMap<QPointer<QObject>,std::function<void()>> _runOnceList;

	/// The main dataset container.
	QPointer<DataSetContainer> _datasetContainer;

	/// The auto-start objects created at application startup.
	std::vector<OORef<AutoStartObject>> _autostartObjects;

	/// The number of parallel threads to be used by the application when doing computations.
	int _idealThreadCount;

	/// The global file manager instance.
	std::unique_ptr<FileManager> _fileManager;

	/// The default message handler method of Qt.
	static QtMessageHandler defaultQtMessageHandler;

	/// The one and only instance of this class.
	static Application* _instance;
};

}	// End of namespace


