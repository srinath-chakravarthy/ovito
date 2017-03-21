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

#include <core/Core.h>
#include <core/utilities/concurrent/TaskManager.h>
#include <core/viewport/ViewportConfiguration.h>
#include <core/dataset/DataSetContainer.h>

#ifdef Q_OS_UNIX
	#include <signal.h>
#endif

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

/******************************************************************************
* Initializes the task manager.
******************************************************************************/
TaskManager::TaskManager()
{
	qRegisterMetaType<PromiseBasePtr>("PromiseBasePtr");
}

/******************************************************************************
* Registers a promise with the task manager.
******************************************************************************/
PromiseWatcher* TaskManager::addTaskInternal(PromiseBasePtr promise)
{
	// Check if task is already registered.
	for(PromiseWatcher* watcher : runningTasks()) {
		if(watcher->promise() == promise)
			return watcher;
	}

	// Create a task watcher, which will generate start/stop notification signals.
	PromiseWatcher* watcher = new PromiseWatcher(this);
	connect(watcher, &PromiseWatcher::started, this, &TaskManager::taskStartedInternal);
	connect(watcher, &PromiseWatcher::finished, this, &TaskManager::taskFinishedInternal);
	
	// Activate the watcher.
	watcher->setPromise(promise);
	return watcher;
}

/******************************************************************************
* Is called when a task has started to run.
******************************************************************************/
void TaskManager::taskStartedInternal()
{
	PromiseWatcher* watcher = static_cast<PromiseWatcher*>(sender());
	_runningTaskStack.push_back(watcher);

	Q_EMIT taskStarted(watcher);
}

/******************************************************************************
* Is called when a task has finished.
******************************************************************************/
void TaskManager::taskFinishedInternal()
{
	PromiseWatcher* watcher = static_cast<PromiseWatcher*>(sender());
	
	OVITO_ASSERT(std::find(_runningTaskStack.begin(), _runningTaskStack.end(), watcher) != _runningTaskStack.end());
	_runningTaskStack.erase(std::find(_runningTaskStack.begin(), _runningTaskStack.end(), watcher));

	Q_EMIT taskFinished(watcher);

	watcher->deleteLater();	
}

/******************************************************************************
* Cancels all running background tasks.
******************************************************************************/
void TaskManager::cancelAll()
{
	for(PromiseWatcher* watcher : runningTasks()) {
		watcher->cancel();
	}
}

/******************************************************************************
* Cancels all running background tasks and waits for them to finish.
******************************************************************************/
void TaskManager::cancelAllAndWait()
{
	cancelAll();
	waitForAll();
}

/******************************************************************************
* Waits for all tasks to finish.
******************************************************************************/
void TaskManager::waitForAll()
{
	for(PromiseWatcher* watcher : runningTasks()) {
		try {
			watcher->waitForFinished();
		}
		catch(...) {}
	}
}

/******************************************************************************
* This should be called whenever a local event handling loop is entered.
******************************************************************************/
void TaskManager::startLocalEventHandling() 
{
	OVITO_ASSERT_MSG(QThread::currentThread() == QCoreApplication::instance()->thread(), "TaskManager::waitForTask", "Function may only be called from the main thread.");
	
	_inLocalEventLoop++;
	Q_EMIT localEventLoopEntered();
}

/******************************************************************************
* This should be called whenever a local event handling loop is left.
******************************************************************************/
void TaskManager::stopLocalEventHandling()
{
	OVITO_ASSERT_MSG(QThread::currentThread() == QCoreApplication::instance()->thread(), "TaskManager::waitForTask", "Function may only be called from the main thread.");
	OVITO_ASSERT(_inLocalEventLoop > 0);

	_inLocalEventLoop--;
	Q_EMIT localEventLoopExited();
}

/******************************************************************************
* Waits for the given task to finish.
******************************************************************************/
bool TaskManager::waitForTask(const PromiseBasePtr& promise)
{
	OVITO_ASSERT_MSG(QThread::currentThread() == QCoreApplication::instance()->thread(), "TaskManager::waitForTask", "Function may only be called from the main thread.");

	// Before entering the local event loop, check if task has already finished.
	if(promise->isFinished()) {
		return !promise->isCanceled();
	}

	// Register the task in cases it hasn't been registered with this TaskManager yet.
	PromiseWatcher* watcher = addTaskInternal(promise); 

	// Start a local event loop and wait for the task to generate a signal when it finishes.
	QEventLoop eventLoop;
	connect(watcher, &PromiseWatcher::finished, &eventLoop, &QEventLoop::quit);

#ifdef Q_OS_UNIX
	// Boolean flag which is set by the POSIX signal handler when user
	// presses Ctrl+C to interrupt the program.
	static QAtomicInt userInterrupt;
	userInterrupt.storeRelease(0);

	// Install POSIX signal handler to catch Ctrl+C key signal.
	static QEventLoop* activeEventLoop = nullptr; 
	activeEventLoop = &eventLoop;
	auto oldSignalHandler = ::signal(SIGINT, [](int) {
		userInterrupt.storeRelease(1);
		if(activeEventLoop)
			QMetaObject::invokeMethod(activeEventLoop, "quit");
	});
#endif
	
	startLocalEventHandling();
	eventLoop.exec();
	stopLocalEventHandling();

#ifdef Q_OS_UNIX
	::signal(SIGINT, oldSignalHandler);
	activeEventLoop = nullptr;
	if(userInterrupt.load()) {
		cancelAll();
		return false;
	}
#endif

	return !promise->isCanceled();
}

/******************************************************************************
* Process events from the event queue when the tasks manager has started
*  a local event loop. Otherwise does nothing and lets the main event loop
* do the processing. 
******************************************************************************/
void TaskManager::processEvents()
{
	if(_inLocalEventLoop)
		QCoreApplication::processEvents();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
