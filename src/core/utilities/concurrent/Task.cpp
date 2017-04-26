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
#include <core/utilities/concurrent/Task.h>
#include <core/utilities/concurrent/TaskManager.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

/******************************************************************************
* Constructor.
******************************************************************************/
SynchronousTask::SynchronousTask(TaskManager& taskManager) : _promise(std::make_shared<Promise<void>>()), _taskManager(taskManager)
{
	taskManager.registerTask(_promise);
	_promise->setStarted();
}

/******************************************************************************
* Destructor.
******************************************************************************/
SynchronousTask::~SynchronousTask() 
{
	_promise->setFinished();
}

/******************************************************************************
* Sets the status text to be displayed.
******************************************************************************/
void SynchronousTask::setProgressText(const QString& text)
{
	_promise->setProgressText(text);

	// Yield control to the event loop to process user interface events.
	// This is necessary so that the user can interrupt the running opertion.
	_taskManager.processEvents();
}

/******************************************************************************
* Sets the value displayed by the progress bar.
******************************************************************************/
void SynchronousTask::setProgressValue(int v) 
{
	_promise->setProgressValue(v);

	// Yield control to the event loop to process user interface events.
	// This is necessary so that the user can interrupt the running opertion.
	_taskManager.processEvents();
}

/******************************************************************************
* Returns whether the operation has been canceled by the user.
******************************************************************************/
bool SynchronousTask::isCanceled() const 
{ 
	// Note: The SynchronousTask may get destroyed during event processing. Better access it first.
	bool result = _promise->isCanceled();

	// Yield control to the event loop to process user interface events.
	// This is necessary so that the user can interrupt the running opertion.
	_taskManager.processEvents();
	
	return result; 
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
