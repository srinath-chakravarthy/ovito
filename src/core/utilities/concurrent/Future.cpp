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
#include "moc_FutureWatcher.cpp"

namespace Ovito {

enum {
    MaxProgressEmitsPerSecond = 20
};

void FutureInterfaceBase::cancel()
{
	//qDebug() << "BEG FutureInterfaceBase::cancel() this=" << this << "thread=" << QThread::currentThread() << "text=" << progressText();
	QMutexLocker locker(&_mutex);
	if(isCanceled()) {
	//	qDebug() << "ALREADY CANCELED FutureInterfaceBase::cancel() this=" << this << "thread=" << QThread::currentThread() << "text=" << progressText();
		return;
	}

	if(_subTask) {
	//	qDebug() << "Canceling subtask";
		_subTask->cancel();
	}

	_state = State(_state | Canceled);
	_waitCondition.wakeAll();
	sendCallOut(FutureWatcher::CallOutEvent::Canceled);
	//qDebug() << "END FutureInterfaceBase::cancel() this=" << this << "thread=" << QThread::currentThread() << "text=" << progressText();
}

bool FutureInterfaceBase::reportStarted()
{
	//qDebug() << "BEG FutureInterfaceBase::reportStarted() this=" << this << "thread=" << QThread::currentThread() << "text=" << progressText();
	//qDebug() << "THREAD COUNT=" << QThreadPool::globalInstance()->activeThreadCount();
    QMutexLocker locker(&_mutex);
    bool isAlreadyStarted = isStarted();
    OVITO_ASSERT(!isFinished() || isRunning());
    if(isAlreadyStarted)
        return false;
    _state = State(Started | Running);
    sendCallOut(FutureWatcher::CallOutEvent::Started);
	//qDebug() << "END FutureInterfaceBase::reportStarted() this=" << this << "thread=" << QThread::currentThread() << "text=" << progressText();
	return true;
}

void FutureInterfaceBase::reportFinished()
{
	//qDebug() << "BEG FutureInterfaceBase::reportFinished() this=" << this << "thread=" << QThread::currentThread() << "text=" << progressText();
    QMutexLocker locker(&_mutex);
    OVITO_ASSERT(isStarted());
    if(!isFinished()) {
        _state = State((_state & ~Running) | Finished);
        _waitCondition.wakeAll();
        sendCallOut(FutureWatcher::CallOutEvent::Finished);
    }
	//qDebug() << "END FutureInterfaceBase::reportFinished() this=" << this << "thread=" << QThread::currentThread() << "text=" << progressText();
}

void FutureInterfaceBase::reportException()
{
	//qDebug() << "EXCEPTION FutureInterfaceBase::reportException() this=" << this << "thread=" << QThread::currentThread() << "text=" << progressText();
	QMutexLocker locker(&_mutex);
	if(isCanceled() || isFinished())
		return;

	_exceptionStore = std::current_exception();
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

void FutureInterfaceBase::tryToRunImmediately()
{
	if(_runnable)
		_runnable->run();
}

void FutureInterfaceBase::waitForResult()
{
	//qDebug() << "WAIT FOR RESULT FutureInterfaceBase::waitForResult() this=" << this << "thread=" << QThread::currentThread() << "text=" << progressText();
	throwPossibleException();

	QMutexLocker lock(&_mutex);
	if(!isRunning())
		return;

	lock.unlock();

	// To avoid deadlocks and reduce the number of threads used, try to
	// run the runnable in the current thread.
	tryToRunImmediately();

	lock.relock();
	if(!isRunning())
		return;

	while(isRunning() && isResultSet() == false)
		_waitCondition.wait(&_mutex);

	throwPossibleException();
}

void FutureInterfaceBase::waitForFinished()
{
	//qDebug() << "BEG FutureInterfaceBase::waitForFinished() this=" << this << "thread=" << QThread::currentThread() << "text=" << progressText();

	QMutexLocker lock(&_mutex);
    const bool alreadyFinished = !isRunning();
    lock.unlock();

    if(!alreadyFinished) {
    	tryToRunImmediately();
        lock.relock();
        while(isRunning())
            _waitCondition.wait(&_mutex);
    }

    throwPossibleException();

	//qDebug() << "END FutureInterfaceBase::waitForFinished() this=" << this << "thread=" << QThread::currentThread() << "text=" << progressText();
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

bool FutureInterfaceBase::waitForSubTask(FutureInterfaceBase* subTask)
{
	//qDebug() << "BEG FutureInterfaceBase::waitForSubTask() this=" << this << "thread=" << QThread::currentThread() << "text=" << progressText() << "subtask=" << subTask;
	QMutexLocker locker(&_mutex);
	this->_subTask = subTask;
	if(this->isCanceled()) subTask->cancel();
	locker.unlock();
	try {
		subTask->waitForFinished();
	}
	catch(...) {
		locker.relock();
		this->_subTask = nullptr;
		throw;
	}
	locker.relock();
	this->_subTask = nullptr;
	if(subTask->isCanceled()) {
		this->cancel();
		//qDebug() << "END FutureInterfaceBase::waitForSubTask() this=" << this << "thread=" << QThread::currentThread() << "text=" << progressText() << "subtask=" << subTask;
		return false;
	}
	//qDebug() << "END FutureInterfaceBase::waitForSubTask() this=" << this << "thread=" << QThread::currentThread() << "text=" << progressText() << "subtask=" << subTask;
	return true;
}

void FutureInterfaceBase::setProgressRange(int maximum)
{
    QMutexLocker locker(&_mutex);
    _progressMaximum = maximum;
    sendCallOut(FutureWatcher::CallOutEvent::ProgressRange, maximum);
}

void FutureInterfaceBase::setProgressValue(int value)
{
    QMutexLocker locker(&_mutex);

    if(value == _progressValue)
    	return;

    if(isCanceled() || isFinished())
        return;

    _progressValue = value;
    if(_progressTime.isValid() && _progressValue != _progressMaximum)
    	if(_progressTime.elapsed() < (1000 / MaxProgressEmitsPerSecond))
            return;

    _progressTime.start();
    sendCallOut(FutureWatcher::CallOutEvent::ProgressValue, value);
}

void FutureInterfaceBase::setProgressText(const QString& progressText)
{
    QMutexLocker locker(&_mutex);

    if(isCanceled() || isFinished())
        return;

    _progressText = progressText;
    sendCallOut(FutureWatcher::CallOutEvent::ProgressText, progressText);
}

bool FutureInterfaceBase::isProgressUpdateNeeded()
{
    QMutexLocker locker(&_mutex);
    return !_progressTime.isValid() || (_progressTime.elapsed() > (1000 / MaxProgressEmitsPerSecond));
}

void FutureWatcher::setFutureInterface(const std::shared_ptr<FutureInterfaceBase>& futureInterface, bool pendingAssignment)
{
	if(futureInterface == _futureInterface)
		return;

	if(_futureInterface) {
		if(pendingAssignment) {
	        _finished = false;
	        QCoreApplication::removePostedEvents(this);
		}
		_futureInterface->unregisterWatcher(this);
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

QString FutureWatcher::progressText() const
{
	return _futureInterface->progressText();
}

void FutureWatcher::waitForFinished() const
{
	if(_futureInterface)
		_futureInterface->waitForFinished();
}


};