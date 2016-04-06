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

#ifndef __OVITO_FUTURE_INTERFACE_H
#define __OVITO_FUTURE_INTERFACE_H

#include <core/Core.h>
#include "FutureWatcher.h"

#include <QWaitCondition>
#include <QElapsedTimer>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

template<typename R> class Future;						// Defined in Future.h

class OVITO_CORE_EXPORT FutureInterfaceBase
{
public:

	enum State {
		NoState    = 0,
		Running    = (1<<0),
		Started    = (1<<1),
		Canceled   = (1<<2),
		Finished   = (1<<3),
		ResultSet  = (1<<4)
	};

	virtual ~FutureInterfaceBase();

	bool isCanceled() const { return (_state & Canceled); }
    int progressMaximum() const { return _progressMaximum; }
    int totalProgressMaximum() const { return _totalProgressMaximum; }
    int progressValue() const { return _progressValue; }
    int totalProgressValue() const { return _totalProgressValue; }
    QString progressText() const { return _progressText; }

    void setProgressRange(int maximum);
    bool setProgressValue(int progressValue);
    bool setProgressValueIntermittent(int progressValue, int updateEvery = 2000);
    bool incrementProgressValue(int increment = 1);
    void setProgressText(const QString& progressText);
    void beginProgressSubSteps(std::vector<int> weights);
    void beginProgressSubSteps(int numSteps) { beginProgressSubSteps(std::vector<int>(numSteps, 1)); }
    void nextProgressSubStep();
    void endProgressSubSteps();

	template<typename RS>
	bool waitForSubTask(Future<RS>& subFuture) {
		return waitForSubTask(subFuture.getinterface());
	}
	bool waitForSubTask(const std::shared_ptr<FutureInterfaceBase>& subTask);

	virtual void cancel();

    bool reportStarted();

    void reportFinished();

    void reportException();
    void reportException(std::exception_ptr ex);

    void waitForFinished();

protected:

	FutureInterfaceBase(State initialState = NoState) :
		_subTask(nullptr), _state(initialState), _progressValue(0), _progressMaximum(0),
		_totalProgressValue(0), _totalProgressMaximum(0), _intermittentUpdateCounter(0) {
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
		_progressTime.invalidate();
#endif
	}

	bool isRunning() const { return (_state & Running); }
	bool isStarted() const { return (_state & Started); }
	bool isFinished() const { return (_state & Finished); }
	bool isResultSet() const { return (_state & ResultSet); }

    void reportResultReady();

    void reportCanceled() {
    	cancel();
    }

    void throwPossibleException() {
    	if(_exceptionStore)
    		std::rethrow_exception(_exceptionStore);
    }

    void waitForResult();

    void sendCallOut(FutureWatcher::CallOutEvent::CallOutType type) {
    	Q_FOREACH(FutureWatcher* watcher, _watchers)
    		watcher->postCallOutEvent(type, this);
    }

    void sendCallOut(FutureWatcher::CallOutEvent::CallOutType type, int value) {
    	Q_FOREACH(FutureWatcher* watcher, _watchers)
    		watcher->postCallOutEvent(type, value, this);
    }

    void sendCallOut(FutureWatcher::CallOutEvent::CallOutType type, const QString& text) {
    	Q_FOREACH(FutureWatcher* watcher, _watchers)
    		watcher->postCallOutEvent(type, text, this);
    }

    void registerWatcher(FutureWatcher* watcher);
    void unregisterWatcher(FutureWatcher* watcher);

    void computeTotalProgress();

	virtual void tryToRunImmediately() {}

	FutureInterfaceBase* _subTask;
	QList<FutureWatcher*> _watchers;
	QMutex _mutex;
	State _state;
	QWaitCondition _waitCondition;
	std::exception_ptr _exceptionStore;
	int _totalProgressValue;
	int _totalProgressMaximum;
    int _progressValue;
    int _progressMaximum;
    int _intermittentUpdateCounter;
    QString _progressText;
    QElapsedTimer _progressTime;
    std::vector<std::pair<int, std::vector<int>>> subStepsStack;

	friend class FutureWatcher;
	friend class TaskManager;
	friend class FutureBase;
	template<typename R2> friend class Future;
};

template<typename R>
class FutureInterface : public FutureInterfaceBase
{
public:

	FutureInterface() {}

	void setResult(const R& value) {
		QMutexLocker locker(&_mutex);
		if(isCanceled() || isFinished())
			return;
		_result = value;
		_state = State(_state | ResultSet);
		reportResultReady();
	}

	void setResult(R&& value) {
		QMutexLocker locker(&_mutex);
		if(isCanceled() || isFinished())
			return;
		_result = std::move(value);
		_state = State(_state | ResultSet);
		reportResultReady();
	}

private:

	R _result;

	template<typename R2, typename Function> friend class Task;
	template<typename R2> friend class Future;
};

template<>
class FutureInterface<void> : public FutureInterfaceBase
{
public:
	FutureInterface() {}

	template<typename R2, typename Function> friend class Task;
	template<typename R2> friend class Future;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace

#endif // __OVITO_FUTURE_INTERFACE_H
