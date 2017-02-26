///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2017) Alexander Stukowski
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
#include "PromiseWatcher.h"
#include "Promise.h"
#include "Future.h"
#include "Task.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

void PromiseWatcher::setPromise(const PromiseBasePtr& promise, bool pendingAssignment)
{
	if(promise == _promise)
		return;

	if(_promise) {
		_promise->unregisterWatcher(this);
		if(pendingAssignment) {
	        _finished = false;
	        QCoreApplication::removePostedEvents(this);
		}
	}
	_promise = promise;
	if(_promise)
		_promise->registerWatcher(this);
}

void PromiseWatcher::promiseCanceled()
{
	if(promise())
		Q_EMIT canceled();
}

void PromiseWatcher::promiseFinished()
{
	if(promise()) {
		_finished = true;
		Q_EMIT finished();
	}
}

void PromiseWatcher::promiseStarted()
{
	if(promise())
    	Q_EMIT started();
}

void PromiseWatcher::promiseResultReady()
{
	if(promise() && !promise()->isCanceled())
		Q_EMIT resultReady();
}

void PromiseWatcher::promiseProgressRangeChanged(int maximum)
{
	if(promise() && !promise()->isCanceled())
		Q_EMIT progressRangeChanged(maximum);
}

void PromiseWatcher::promiseProgressValueChanged(int progressValue)
{
	if(promise() && !promise()->isCanceled())
		Q_EMIT progressValueChanged(progressValue);
}

void PromiseWatcher::promiseProgressTextChanged(QString progressText)
{
	if(promise() && !promise()->isCanceled())
		Q_EMIT progressTextChanged(progressText);
}

void PromiseWatcher::cancel()
{
	if(promise())
		promise()->cancel();
}

bool PromiseWatcher::isCanceled() const
{
	return promise() ? promise()->isCanceled() : false;
}

bool PromiseWatcher::isFinished() const
{
	return _finished;
}

int PromiseWatcher::progressMaximum() const
{
	return promise() ? promise()->totalProgressMaximum() : 0;
}

int PromiseWatcher::progressValue() const
{
	return promise() ? promise()->totalProgressValue() : 0;
}

QString PromiseWatcher::progressText() const
{
	return promise() ? promise()->progressText() : QString();
}

void PromiseWatcher::waitForFinished() const
{
	if(promise())
		promise()->waitForFinished();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
