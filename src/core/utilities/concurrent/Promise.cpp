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
#include "Promise.h"
#include "Future.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

enum {
    MaxProgressEmitsPerSecond = 20
};

PromiseBase::~PromiseBase()
{
}

void PromiseBase::cancel()
{
	QMutexLocker locker(&_mutex);

	if(_subTask) {
		_subTask->cancel();
	}

	if(isCanceled()) {
		return;
	}

	_state = State(_state | Canceled);
	_waitCondition.wakeAll();
	for(PromiseWatcher* watcher : _watchers)
		QMetaObject::invokeMethod(watcher, "promiseCanceled", Qt::QueuedConnection);
}

bool PromiseBase::setStarted()
{
    QMutexLocker locker(&_mutex);
    if(isStarted())
        return false;	// It's already started. Don't run it again.
    OVITO_ASSERT(!isFinished() || isRunning());
    _state = State(Started | Running);
	for(PromiseWatcher* watcher : _watchers)
		QMetaObject::invokeMethod(watcher, "promiseStarted", Qt::QueuedConnection);
	return true;
}

void PromiseBase::setFinished()
{
	QMutexLocker locker(&_mutex);
    OVITO_ASSERT(isStarted());
    if(!isFinished()) {
        _state = State((_state & ~Running) | Finished);
        _waitCondition.wakeAll();
		for(PromiseWatcher* watcher : _watchers)
			QMetaObject::invokeMethod(watcher, "promiseFinished", Qt::QueuedConnection);
    }
}

void PromiseBase::setException()
{
	QMutexLocker locker(&_mutex);
	if(isCanceled() || isFinished())
		return;

	setException(std::current_exception());
}

void PromiseBase::setException(std::exception_ptr ex)
{
	_exceptionStore = ex;
	_state = State(_state | ResultSet);
	_waitCondition.wakeAll();
	for(PromiseWatcher* watcher : _watchers)
		QMetaObject::invokeMethod(watcher, "promiseResultReady", Qt::QueuedConnection);
}

void PromiseBase::setResultReady()
{
	if(isCanceled() || isFinished())
		return;

	_state = State(_state | ResultSet);
    _waitCondition.wakeAll();
	for(PromiseWatcher* watcher : _watchers)
		QMetaObject::invokeMethod(watcher, "promiseResultReady", Qt::QueuedConnection);
}

void PromiseBase::waitForResult()
{
	throwPossibleException();

	QMutexLocker lock(&_mutex);
	if(!isRunning() && isStarted())
		return;

	lock.unlock();

	// To avoid deadlocks and reduce the number of threads used, try to
	// run the runnable in the current thread.
	tryToRunImmediately();

	lock.relock();
	if(!isRunning() && isStarted())
		return;

	while(isRunning() && isResultSet() == false)
		_waitCondition.wait(&_mutex);

	throwPossibleException();

	if(isCanceled())
		throw Exception("No result available, because promise has been canceled.");

	OVITO_ASSERT(isResultSet());
}

void PromiseBase::waitForFinished()
{
	QMutexLocker lock(&_mutex);
    const bool alreadyFinished = !isRunning() && isStarted();
    lock.unlock();

    if(!alreadyFinished) {
    	tryToRunImmediately();
        lock.relock();
        while(isRunning() || !isStarted())
            _waitCondition.wait(&_mutex);
    }

    throwPossibleException();
}

void PromiseBase::registerWatcher(PromiseWatcher* watcher)
{
	QMutexLocker locker(&_mutex);

	if(isStarted())
		QMetaObject::invokeMethod(watcher, "promiseStarted", Qt::QueuedConnection);

	if(isResultSet())
		QMetaObject::invokeMethod(watcher, "promiseResultReady", Qt::QueuedConnection);

	if(isCanceled())
		QMetaObject::invokeMethod(watcher, "promiseCanceled", Qt::QueuedConnection);

	if(isFinished())
		QMetaObject::invokeMethod(watcher, "promiseFinished", Qt::QueuedConnection);

	_watchers.push_back(watcher);
}

void PromiseBase::unregisterWatcher(PromiseWatcher* watcher)
{
	QMutexLocker locker(&_mutex);
	_watchers.removeOne(watcher);
}

bool PromiseBase::waitForSubTask(const FutureBase& subFuture) 
{
	return waitForSubTask(subFuture.promise());
}

bool PromiseBase::waitForSubTask(const PromiseBasePtr& subTask)
{
	QMutexLocker locker(&_mutex);
	if(this->isCanceled()) {
		subTask->cancel();
		return false;
	}
	if(subTask->isCanceled()) {
		locker.unlock();
		this->cancel();
		return false;
	}
	this->_subTask = subTask.get();
	locker.unlock();
	try {
		QMutexLocker subtaskLock(&subTask->_mutex);
	    const bool subTaskAlreadyFinished = !subTask->isRunning() && subTask->isStarted();
	    subtaskLock.unlock();

	    if(!subTaskAlreadyFinished) {
	    	subTask->tryToRunImmediately();
	    	subtaskLock.relock();
	        while(!subTask->isCanceled() && (subTask->isRunning() || !subTask->isStarted()))
	        	subTask->_waitCondition.wait(&subTask->_mutex);
	    }

	    subTask->throwPossibleException();
	}
	catch(...) {
		locker.relock();
		this->_subTask = nullptr;
		throw;
	}
	locker.relock();
	this->_subTask = nullptr;
	locker.unlock();
	if(subTask->isCanceled()) {
		this->cancel();
		return false;
	}
	return true;
}

void PromiseBase::setProgressMaximum(int maximum)
{
    QMutexLocker locker(&_mutex);

	if(maximum == _progressMaximum || isCanceled() || isFinished())
		return;

    _progressMaximum = maximum;
    computeTotalProgress();
	for(PromiseWatcher* watcher : _watchers)
		QMetaObject::invokeMethod(watcher, "promiseProgressRangeChanged", Qt::QueuedConnection, Q_ARG(int, totalProgressMaximum()));
}

bool PromiseBase::setProgressValue(int value)
{
    QMutexLocker locker(&_mutex);
	_intermittentUpdateCounter = 0;

    if(value == _progressValue || isCanceled() || isFinished())
        return !isCanceled();

    _progressValue = value;
    computeTotalProgress();

    if(!_progressTime.isValid() || _progressValue == _progressMaximum || _progressTime.elapsed() >= (1000 / MaxProgressEmitsPerSecond)) {
		_progressTime.start();
		for(PromiseWatcher* watcher : _watchers)
			QMetaObject::invokeMethod(watcher, "promiseProgressValueChanged", Qt::QueuedConnection, Q_ARG(int, totalProgressValue()));
    }

    return !isCanceled();
}

bool PromiseBase::setProgressValueIntermittent(int progressValue, int updateEvery)
{
	if(_intermittentUpdateCounter == 0 || _intermittentUpdateCounter > updateEvery) {
		setProgressValue(progressValue);
	}
	_intermittentUpdateCounter++;
	return !isCanceled();
}

bool PromiseBase::incrementProgressValue(int increment)
{
    QMutexLocker locker(&_mutex);

    if(isCanceled() || isFinished())
        return !isCanceled();

    _progressValue += increment;
    computeTotalProgress();

    if(!_progressTime.isValid() || _progressValue == _progressMaximum || _progressTime.elapsed() >= (1000 / MaxProgressEmitsPerSecond)) {
		_progressTime.start();
		for(PromiseWatcher* watcher : _watchers)
			QMetaObject::invokeMethod(watcher, "promiseProgressValueChanged", Qt::QueuedConnection, Q_ARG(int, progressValue()));
    }

    return !isCanceled();
}

void PromiseBase::computeTotalProgress()
{
	if(subStepsStack.empty()) {
		_totalProgressMaximum = _progressMaximum;
		_totalProgressValue = _progressValue;
	}
	else {
		double percentage;
		if(_progressMaximum > 0)
			percentage = (double)_progressValue / _progressMaximum;
		else
			percentage = 0.0;
		for(auto level = subStepsStack.crbegin(); level != subStepsStack.crend(); ++level) {
			OVITO_ASSERT(level->first >= 0 && level->first < level->second.size());
			int weightSum1 = std::accumulate(level->second.cbegin(), level->second.cbegin() + level->first, 0);
			int weightSum2 = std::accumulate(level->second.cbegin() + level->first, level->second.cend(), 0);
			percentage = ((double)weightSum1 + percentage * level->second[level->first]) / (weightSum1 + weightSum2);
		}
		_totalProgressMaximum = 1000;
		_totalProgressValue = int(percentage * 1000.0);
	}
}

void PromiseBase::beginProgressSubSteps(std::vector<int> weights)
{
    QMutexLocker locker(&_mutex);
    OVITO_ASSERT(std::accumulate(weights.cbegin(), weights.cend(), 0) > 0);
    subStepsStack.push_back(std::make_pair(0, std::move(weights)));
    _progressMaximum = 0;
    _progressValue = 0;
    computeTotalProgress();
}

void PromiseBase::nextProgressSubStep()
{
	QMutexLocker locker(&_mutex);
	OVITO_ASSERT(!subStepsStack.empty());
	OVITO_ASSERT(subStepsStack.back().first < subStepsStack.back().second.size() - 1);
	subStepsStack.back().first++;
    _progressMaximum = 0;
    _progressValue = 0;
    computeTotalProgress();
}

void PromiseBase::endProgressSubSteps()
{
	QMutexLocker locker(&_mutex);
	OVITO_ASSERT(!subStepsStack.empty());
	subStepsStack.pop_back();
    _progressMaximum = 0;
    _progressValue = 0;
    computeTotalProgress();
}

void PromiseBase::setProgressText(const QString& progressText)
{
    QMutexLocker locker(&_mutex);

    if(isCanceled() || isFinished())
        return;

    _progressText = progressText;
	for(PromiseWatcher* watcher : _watchers)
		QMetaObject::invokeMethod(watcher, "promiseProgressTextChanged", Qt::QueuedConnection, Q_ARG(QString, progressText));
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
