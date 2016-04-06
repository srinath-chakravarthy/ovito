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

#include <core/Core.h>
#include <core/utilities/concurrent/TaskManager.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

/******************************************************************************
* Initializes the progress manager.
******************************************************************************/
TaskManager::TaskManager()
{
	qRegisterMetaType<std::shared_ptr<FutureInterfaceBase>>("std::shared_ptr<FutureInterfaceBase>");

	connect(&_taskStartedSignalMapper, (void (QSignalMapper::*)(QObject*))&QSignalMapper::mapped, this, &TaskManager::taskStarted);
	connect(&_taskFinishedSignalMapper, (void (QSignalMapper::*)(QObject*))&QSignalMapper::mapped, this, &TaskManager::taskFinished);
	connect(&_taskProgressValueChangedSignalMapper, (void (QSignalMapper::*)(QObject*))&QSignalMapper::mapped, this, &TaskManager::taskProgressValueChanged);
	connect(&_taskProgressTextChangedSignalMapper, (void (QSignalMapper::*)(QObject*))&QSignalMapper::mapped, this, &TaskManager::taskProgressTextChanged);
}

/******************************************************************************
* Registers a future with the progress manager.
******************************************************************************/
void TaskManager::addTaskInternal(std::shared_ptr<FutureInterfaceBase> futureInterface)
{
	FutureWatcher* watcher = new FutureWatcher(this);
	connect(watcher, &FutureWatcher::started, &_taskStartedSignalMapper, (void (QSignalMapper::*)())&QSignalMapper::map);
	connect(watcher, &FutureWatcher::finished, &_taskFinishedSignalMapper, (void (QSignalMapper::*)())&QSignalMapper::map);
	connect(watcher, &FutureWatcher::progressRangeChanged, &_taskProgressValueChangedSignalMapper, (void (QSignalMapper::*)())&QSignalMapper::map);
	connect(watcher, &FutureWatcher::progressValueChanged, &_taskProgressValueChangedSignalMapper, (void (QSignalMapper::*)())&QSignalMapper::map);
	connect(watcher, &FutureWatcher::progressTextChanged, &_taskProgressTextChangedSignalMapper, (void (QSignalMapper::*)())&QSignalMapper::map);
	_taskStartedSignalMapper.setMapping(watcher, watcher);
	_taskFinishedSignalMapper.setMapping(watcher, watcher);
	_taskProgressValueChangedSignalMapper.setMapping(watcher, watcher);
	_taskProgressTextChangedSignalMapper.setMapping(watcher, watcher);

	// Activate the future watcher.
	watcher->setFutureInterface(futureInterface);
}

/******************************************************************************
* Is called when a task has started to run.
******************************************************************************/
void TaskManager::taskStarted(QObject* object)
{
	FutureWatcher* watcher = static_cast<FutureWatcher*>(object);
	_runningTaskStack.push_back(watcher);
}

/******************************************************************************
* Is called when a task has finished.
******************************************************************************/
void TaskManager::taskFinished(QObject* object)
{
	FutureWatcher* watcher = static_cast<FutureWatcher*>(object);
	OVITO_ASSERT( != _runningTaskStack.end());
	_runningTaskStack.erase(std::find(_runningTaskStack.begin(), _runningTaskStack.end(), watcher));
	watcher->deleteLater();
}

/******************************************************************************
* Cancels all running background tasks.
******************************************************************************/
void TaskManager::cancelAll()
{
	for(FutureWatcher* watcher : _taskStack)
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
	for(FutureWatcher* watcher : _taskStack) {
		try {
			watcher->waitForFinished();
		}
		catch(...) {}
	}
}

/******************************************************************************
* Waits for the given task to finish and displays a modal progress dialog
* to show the task's progress.
******************************************************************************/
bool TaskManager::waitForTask(const std::shared_ptr<FutureInterfaceBase>& futureInterface)
{
	OVITO_ASSERT_MSG(QThread::currentThread() == QCoreApplication::instance()->thread(), "TaskManager::waitForTask", "Function can only be called from the main thread.");

	// Before showing any progress dialog, check if task has already finished.
	if(futureInterface->isFinished())
		return !futureInterface->isCanceled();

	FutureWatcher watcher;
	watcher.setFutureInterface(futureInterface);
	while(!watcher.isFinished()) {
		QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, 100);
	}

	return !futureInterface->isCanceled();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
