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

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

/******************************************************************************
* Initializes the task manager.
******************************************************************************/
TaskManager::TaskManager()
{
	qRegisterMetaType<std::shared_ptr<FutureInterfaceBase>>("std::shared_ptr<FutureInterfaceBase>");
}

/******************************************************************************
* Registers a future with the task manager.
******************************************************************************/
void TaskManager::addTaskInternal(std::shared_ptr<FutureInterfaceBase> futureInterface)
{
	// Check if task is already registered.
	for(FutureWatcher* watcher : runningTasks()) {
		if(watcher->futureInterface() == futureInterface)
			return;
	}

	// Create a task watcher, which will generate start/stop notification signals.
	FutureWatcher* watcher = new FutureWatcher(this);
	connect(watcher, &FutureWatcher::started, this, &TaskManager::taskStartedInternal);
	connect(watcher, &FutureWatcher::finished, this, &TaskManager::taskFinishedInternal);
	
	// Activate the watcher.
	watcher->setFutureInterface(futureInterface);
}

/******************************************************************************
* Is called when a task has started to run.
******************************************************************************/
void TaskManager::taskStartedInternal()
{
	FutureWatcher* watcher = static_cast<FutureWatcher*>(sender());
	_runningTaskStack.push_back(watcher);

	Q_EMIT taskStarted(watcher);
}

/******************************************************************************
* Is called when a task has finished.
******************************************************************************/
void TaskManager::taskFinishedInternal()
{
	FutureWatcher* watcher = static_cast<FutureWatcher*>(sender());
	
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
	for(FutureWatcher* watcher : _runningTaskStack)
		watcher->cancel();
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
	for(FutureWatcher* watcher : _runningTaskStack) {
		try {
			watcher->waitForFinished();
		}
		catch(...) {}
	}
}

/******************************************************************************
* Waits for the given task to finish.
******************************************************************************/
bool TaskManager::waitForTask(const std::shared_ptr<FutureInterfaceBase>& futureInterface)
{
	OVITO_ASSERT_MSG(!QCoreApplication::instance() || QThread::currentThread() == QCoreApplication::instance()->thread(), "TaskManager::waitForTask", "Function can only be called from the main thread.");

	// Before entering the local event loop, check if task has already finished.
	if(futureInterface->isFinished()) {
		return !futureInterface->isCanceled();
	}

	// Register the task in cases it hasn't been registered with this TaskManager yet.
	addTaskInternal(futureInterface); 

	// Start a local event loop and wait for the task to generate the signal.
	FutureWatcher watcher;
	QEventLoop eventLoop;
	watcher.setFutureInterface(futureInterface);
	connect(&watcher, &FutureWatcher::finished, &eventLoop, &QEventLoop::quit);
	eventLoop.exec();

	return !futureInterface->isCanceled();
}

/******************************************************************************
* Constructor.
******************************************************************************/
SynchronousTask::SynchronousTask(TaskManager& taskManager) : _futureInterface(std::make_shared<FutureInterface<void>>()) 
{
	taskManager.registerTask(_futureInterface);
	_futureInterface->reportStarted();
}

/******************************************************************************
* Destructor.
******************************************************************************/
SynchronousTask::~SynchronousTask() 
{
	_futureInterface->reportFinished();
}

/******************************************************************************
* Sets the status text to be displayed.
******************************************************************************/
void SynchronousTask::setStatusText(const QString& text)
{
	_futureInterface->setProgressText(text);

	// Yield control to the event loop to process user interface events.
	// This is necessary so that the user can interrupt the running opertion.
	QCoreApplication::processEvents();
}

/******************************************************************************
* Sets the value displayed by the progress bar.
******************************************************************************/
void SynchronousTask::setValue(int v) 
{
	_futureInterface->setProgressValue(v);

	// Yield control to the event loop to process user interface events.
	// This is necessary so that the user can interrupt the running opertion.
	QCoreApplication::processEvents();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
