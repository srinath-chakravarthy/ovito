///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2016) Alexander Stukowski
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
#include "Future.h"
#include "Task.h"
#include "PromiseWatcher.h"

#include <QThreadPool>
#include <QMetaObject>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

/**
 * \brief Manages the background tasks.
 */
class OVITO_CORE_EXPORT TaskManager : public QObject
{
	Q_OBJECT

public:

	/// Constructor.
	TaskManager();

	/// Shuts down the task manager after canceling and stopping all active tasks.
	~TaskManager() {
		cancelAllAndWait();
	}

	/// \brief Returns the list of watchers of all currently running tasks.
	///
	/// This method may only be called from the main thread.
	const std::vector<PromiseWatcher*>& runningTasks() const { return _runningTaskStack; }

	/// \brief Asynchronously executes a C++ function in the background.
	///
	/// This function may be called from any thread. It returns immediately and schedules
	/// the function for execution at a later time.
	template<typename Function>
	Future<typename std::result_of<Function(PromiseBase&)>::type> execAsync(Function f) {
		auto task = std::make_shared<FunctionRunner<Function>>(f);
		QThreadPool::globalInstance()->start(task.get());
		registerTask(task);
		return Future<typename std::result_of<Function(PromiseBase&)>::type>(task);
	}

	/// \brief Executes a function in a different thread and blocks the GUI until the function returns
	///        or the user cancels the operation.
	/// \return \c true if the function finished successfully;
	///         \c false if the operation has been cancelled by the user.
	///
	/// The function signature of \c func must be:
	///
	///      void func(PromiseBase& promise);
	///
	/// exec() may only be called from the main thread. In GUI mode, exec() will display
	/// a modal progress dialog while running the worker function, allowing the user to
	/// cancel the operation.
	///
	/// If \c func throws an exception, the exception is re-thrown by exec().
	template<typename Function>
	bool exec(Function func) {
		Future<void> future = execAsync<void>(func);
		if(!waitForTask(future)) return false;
		// This is to re-throw the exception if an error has occurred.
		future.result();
		return true;
	}

	/// \brief Executes an asynchronous task in a background thread.
	///
	/// This function is thread-safe.
	void runTaskAsync(const std::shared_ptr<AsynchronousTask>& task) {
		QThreadPool::globalInstance()->start(task.get());
		registerTask(task);
	}

	/// \brief Executes a task and blocks until the task has finished.
	///
	/// This function must be called from the main thread.
	/// Any exceptions thrown by the task are forwarded.
	bool runTask(const std::shared_ptr<AsynchronousTask>& task) {
		runTaskAsync(task);
		if(!waitForTask(task)) return false;
		// This is to re-throw the exception if an error has occurred.
		task->waitForFinished();
		return true;
	}

	/// \brief Registers a future's promise with the progress manager, which will display the progress of the background task
	///        in the main window.
	///
	/// This function is thread-safe.
	template<typename R>
	void registerTask(const Future<R>& future) {
		registerTask(future.promise());
	}

	/// \brief Registers a promise with the progress manager, which will display the progress of the background task
	///        in the main window.
	///
	/// This function is thread-safe.
	void registerTask(const PromiseBasePtr& promise) {
		// Execute the function call in the main thread.
		QMetaObject::invokeMethod(this, "addTaskInternal", Q_ARG(PromiseBasePtr, promise));
	}

	/// \brief Waits for the given task to finish and displays a modal progress dialog
	///        to show the task's progress.
	/// \return False if the task has been cancelled by the user.
	///
	/// This function must be called from the main thread.
	template<typename R>
	bool waitForTask(const Future<R>& future) {
		return waitForTask(future.promise());
	}

	/// \brief Waits for the given task to finish.
	bool waitForTask(const PromiseBasePtr& futureInterface);

	/// \brief Process events from the event queue when the tasks manager has started
	///        a local event loop. Otherwise does nothing and lets the main event loop
	///        do the processing. 
	void processEvents();

	/// \brief This should be called whenever a local event handling loop is entered.
	void startLocalEventHandling();

	/// \brief This should be called whenever a local event handling loop is left.
	void stopLocalEventHandling();

public Q_SLOTS:

	/// Cancels all running tasks.
	void cancelAll();

	/// Cancels all running tasks and waits for them to finish.
	void cancelAllAndWait();

	/// Waits for all running tasks to finish.
	void waitForAll();

Q_SIGNALS:

	/// \brief This signal is generated by the task manager whenever a new task started to run.
	/// The PromiseWatcher can be used to track the task's progress.
	void taskStarted(PromiseWatcher* taskWatcher);

	/// \brief This signal is generated by the task manager whenever a task finished or stopped running.
	void taskFinished(PromiseWatcher* taskWatcher);

	/// \brief This signal is generated whenever a local event loop is entered to wait for a task to finish.
	void localEventLoopEntered();

	/// \brief This signal is generated whenever a local event loop was exited after waiting for a task to finish.
	void localEventLoopExited();

private:

	/// \brief Registers a promise with the progress manager.
	Q_INVOKABLE PromiseWatcher* addTaskInternal(PromiseBasePtr futureInterface);

	/// Helper class used by asyncExec().
	template<typename Function>
	class FunctionRunner : public Promise<typename std::result_of<Function(PromiseBase&)>::type>, public QRunnable
	{
		Function _function;
	public:
		FunctionRunner(Function fn) : _function(fn) {
			setAutoDelete(false);
		}
		virtual void run() override { tryToRunImmediately(); }
		virtual void tryToRunImmediately() override {
			if(!this->setStarted()) return;
			try {
				this->setResult(_function(*this));
			}
			catch(...) {
				this->setException();
			}
			this->setFinished();
		}
	};

private Q_SLOTS:

	/// \brief Is called when a task has started to run.
	void taskStartedInternal();

	/// \brief Is called when a task has finished.
	void taskFinishedInternal();

private:
	
	/// The list of watchers for the active tasks.
	std::vector<PromiseWatcher*> _runningTaskStack;

	/// Indicates that waitForTask() has started a local event loop.
	int _inLocalEventLoop = 0; 
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::PromiseBasePtr);


