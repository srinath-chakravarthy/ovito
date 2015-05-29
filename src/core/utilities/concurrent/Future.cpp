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
#include "FutureWatcher.h"
#include "FutureInterface.h"
#include "Future.h"
#include "Task.h"
#include "moc_FutureWatcher.cpp"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

enum {
    MaxProgressEmitsPerSecond = 20
};

FutureInterfaceBase::~FutureInterfaceBase()
{
}

void FutureInterfaceBase::cancel()
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
	sendCallOut(FutureWatcher::CallOutEvent::Canceled);
}

bool FutureInterfaceBase::reportStarted()
{
    QMutexLocker locker(&_mutex);
    if(isStarted())
        return false;	// It's already started. Don't run it again.
    OVITO_ASSERT(!isFinished() || isRunning());
    _state = State(Started | Running);
    sendCallOut(FutureWatcher::CallOutEvent::Started);
	return true;
}

void FutureInterfaceBase::reportFinished()
{
	QMutexLocker locker(&_mutex);
    OVITO_ASSERT(isStarted());
    if(!isFinished()) {
        _state = State((_state & ~Running) | Finished);
        _waitCondition.wakeAll();
        sendCallOut(FutureWatcher::CallOutEvent::Finished);
    }
}

void FutureInterfaceBase::reportException()
{
	QMutexLocker locker(&_mutex);
	if(isCanceled() || isFinished())
		return;

	reportException(std::current_exception());
}

void FutureInterfaceBase::reportException(std::exception_ptr ex)
{
	_exceptionStore = ex;
	_state = State(_state | ResultSet);
	_waitCondition.wakeAll();
	sendCallOut(FutureWatcher::CallOutEvent::ResultReady);
}

void FutureInterfaceBase::reportResultReady()
{
	if(isCanceled() || isFinished())
		return;

	_state = State(_state | ResultSet);
    _waitCondition.wakeAll();
    sendCallOut(FutureWatcher::CallOutEvent::ResultReady);
}

void FutureInterfaceBase::waitForResult()
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
}

void FutureInterfaceBase::waitForFinished()
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

void FutureInterfaceBase::registerWatcher(FutureWatcher* watcher)
{
	QMutexLocker locker(&_mutex);

	if(isStarted())
		watcher->postCallOutEvent(FutureWatcher::CallOutEvent::Started, this);

	if(isResultSet())
		watcher->postCallOutEvent(FutureWatcher::CallOutEvent::ResultReady, this);

	if(isCanceled())
		watcher->postCallOutEvent(FutureWatcher::CallOutEvent::Canceled, this);

	if(isFinished())
		watcher->postCallOutEvent(FutureWatcher::CallOutEvent::Finished, this);

	_watchers.push_back(watcher);
}

void FutureInterfaceBase::unregisterWatcher(FutureWatcher* watcher)
{
	QMutexLocker locker(&_mutex);
	_watchers.removeOne(watcher);
}

bool FutureInterfaceBase::waitForSubTask(const std::shared_ptr<FutureInterfaceBase>& subTask)
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
#if 0
		subTask->waitForFinished();
#else
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
#endif
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

void FutureInterfaceBase::setProgressRange(int maximum)
{
    QMutexLocker locker(&_mutex);
    _progressMaximum = maximum;
    computeTotalProgress();
    sendCallOut(FutureWatcher::CallOutEvent::ProgressRange, maximum);
}

bool FutureInterfaceBase::setProgressValue(int value)
{
    QMutexLocker locker(&_mutex);
	_intermittentUpdateCounter = 0;

    if(value == _progressValue || isCanceled() || isFinished())
        return !isCanceled();

    _progressValue = value;
    computeTotalProgress();

    if(!_progressTime.isValid() || _progressValue == _progressMaximum || _progressTime.elapsed() >= (1000 / MaxProgressEmitsPerSecond)) {
		_progressTime.start();
		sendCallOut(FutureWatcher::CallOutEvent::ProgressValue, progressValue());
    }

    return !isCanceled();
}

bool FutureInterfaceBase::setProgressValueIntermittent(int progressValue, int updateEvery)
{
	if(_intermittentUpdateCounter == 0 || _intermittentUpdateCounter > updateEvery) {
		setProgressValue(progressValue);
	}
	_intermittentUpdateCounter++;
	return !isCanceled();
}

bool FutureInterfaceBase::incrementProgressValue(int increment)
{
    QMutexLocker locker(&_mutex);

    if(isCanceled() || isFinished())
        return !isCanceled();

    _progressValue += increment;
    computeTotalProgress();

    if(!_progressTime.isValid() || _progressValue == _progressMaximum || _progressTime.elapsed() >= (1000 / MaxProgressEmitsPerSecond)) {
		_progressTime.start();
		sendCallOut(FutureWatcher::CallOutEvent::ProgressValue, progressValue());
    }

    return !isCanceled();
}

void FutureInterfaceBase::computeTotalProgress()
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

void FutureInterfaceBase::beginProgressSubSteps(std::vector<int> weights)
{
    QMutexLocker locker(&_mutex);
    OVITO_ASSERT(std::accumulate(weights.cbegin(), weights.cend(), 0) > 0);
    subStepsStack.push_back(std::make_pair(0, std::move(weights)));
    _progressMaximum = 0;
    _progressValue = 0;
    computeTotalProgress();
}

void FutureInterfaceBase::nextProgressSubStep()
{
	QMutexLocker locker(&_mutex);
	OVITO_ASSERT(!subStepsStack.empty());
	OVITO_ASSERT(subStepsStack.back().first < subStepsStack.back().second.size() - 1);
	subStepsStack.back().first++;
    _progressMaximum = 0;
    _progressValue = 0;
    computeTotalProgress();
}

void FutureInterfaceBase::endProgressSubSteps()
{
	QMutexLocker locker(&_mutex);
	OVITO_ASSERT(!subStepsStack.empty());
	subStepsStack.pop_back();
    _progressMaximum = 0;
    _progressValue = 0;
    computeTotalProgress();
}

void FutureInterfaceBase::setProgressText(const QString& progressText)
{
    QMutexLocker locker(&_mutex);

    if(isCanceled() || isFinished())
        return;

    _progressText = progressText;
    sendCallOut(FutureWatcher::CallOutEvent::ProgressText, progressText);
}

void FutureWatcher::setFutureInterface(const std::shared_ptr<FutureInterfaceBase>& futureInterface, bool pendingAssignment)
{
	if(futureInterface == _futureInterface)
		return;

	if(_futureInterface) {
		_futureInterface->unregisterWatcher(this);
		if(pendingAssignment) {
	        _finished = false;
	        QCoreApplication::removePostedEvents(this);
		}
	}
	_futureInterface = futureInterface;
	if(_futureInterface)
		_futureInterface->registerWatcher(this);
}

void FutureWatcher::customEvent(QEvent* event)
{
	if(_futureInterface) {
    	OVITO_ASSERT(static_cast<CallOutEvent*>(event)->_source == _futureInterface.get());
		if(event->type() == (QEvent::Type)CallOutEvent::Started)
			Q_EMIT started();
		else if(event->type() == (QEvent::Type)CallOutEvent::Finished) {
			_finished = true;
			Q_EMIT finished();
		}
		else if(event->type() == (QEvent::Type)CallOutEvent::Canceled)
			Q_EMIT canceled();
		else if(event->type() == (QEvent::Type)CallOutEvent::ResultReady) {
			if(!_futureInterface->isCanceled()) {
				Q_EMIT resultReady();
			}
		}
		else if(event->type() == (QEvent::Type)CallOutEvent::ProgressValue) {
			if(!_futureInterface->isCanceled())
				Q_EMIT progressValueChanged(static_cast<CallOutEvent*>(event)->_value);
		}
		else if(event->type() == (QEvent::Type)CallOutEvent::ProgressText) {
			if(!_futureInterface->isCanceled())
				Q_EMIT progressTextChanged(static_cast<CallOutEvent*>(event)->_text);
		}
		else if(event->type() == (QEvent::Type)CallOutEvent::ProgressRange) {
			Q_EMIT progressRangeChanged(static_cast<CallOutEvent*>(event)->_value);
		}
	}
	QObject::customEvent(event);
}

void FutureWatcher::cancel()
{
	if(_futureInterface)
		_futureInterface->cancel();
}

bool FutureWatcher::isCanceled() const
{
	return _futureInterface->isCanceled();
}

bool FutureWatcher::isFinished() const
{
	return _finished;
}

int FutureWatcher::progressMaximum() const
{
	return _futureInterface->progressMaximum();
}

int FutureWatcher::progressValue() const
{
	return _futureInterface->progressValue();
}

int FutureWatcher::totalProgressMaximum() const
{
	return _futureInterface->totalProgressMaximum();
}

int FutureWatcher::totalProgressValue() const
{
	return _futureInterface->totalProgressValue();
}

QString FutureWatcher::progressText() const
{
	return _futureInterface->progressText();
}

void FutureWatcher::waitForFinished() const
{
	if(_futureInterface)
		_futureInterface->waitForFinished();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
