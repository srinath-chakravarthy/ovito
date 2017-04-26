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

#pragma once


#include <core/Core.h>
#include "PromiseWatcher.h"

#include <QWaitCondition>
#include <QMutex>
#include <QMutexLocker>
#include <QElapsedTimer>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

template<typename R> class Future;						// Defined in Future.h

/******************************************************************************
* Generic base class for promises, which implements the basic state management,
* progress reporting, and event processing.
******************************************************************************/
class OVITO_CORE_EXPORT PromiseBase
{
public:

	/// The different states the promise can be in.
	/// Note that combinations are possible.
	enum State {
		NoState    = 0,
		Running    = (1<<0),
		Started    = (1<<1),
		Canceled   = (1<<2),
		Finished   = (1<<3),
		ResultSet  = (1<<4)
	};

	/// Destructor.
	virtual ~PromiseBase();

	/// Returns whether this promise has been canceled by a previous call to cancel().
	bool isCanceled() const { return (_state & Canceled); }

	/// Returns the maximum value for progress reporting. 
    int progressMaximum() const { return _progressMaximum; }

	/// Sets the current maximum value for progress reporting.
    void setProgressMaximum(int maximum);
    
	/// Returns the current progress value (in the range 0 to progressMaximum()).
	int progressValue() const { return _progressValue; }

	/// Sets the current progress value (must be in the range 0 to progressMaximum()).
	/// Returns false if the promise has been canceled.
    bool setProgressValue(int progressValue);

	/// Increments the progress value by 1.
	/// Returns false if the promise has been canceled.
    bool incrementProgressValue(int increment = 1);

	/// Sets the progress value of the promise but generates an update event only occasionally.
	/// Returns false if the promise has been canceled.
    bool setProgressValueIntermittent(int progressValue, int updateEvery = 2000);

	/// Return the current status text set for this promise.
    QString progressText() const { return _progressText; }

	/// Changes the status text of this promise.
	void setProgressText(const QString& progressText);
    
	// Progress reporting for tasks with sub-steps:

	/// Begins a sequence of sub-steps in the progress range of this promise.
	/// This is used for long and complex computations, which consist of several logical sub-steps, each with
	/// a separate progress range.
    void beginProgressSubSteps(std::vector<int> weights);

	/// Convenience version of the function above, which creates N substeps, all with the same weight.
    void beginProgressSubSteps(int nsteps) { beginProgressSubSteps(std::vector<int>(nsteps, 1)); }

	/// Changes to the next sub-step in the sequence started with beginProgressSubSteps().
    void nextProgressSubStep();

	/// Must be called at the end of a sub-step sequence started with beginProgressSubSteps().
    void endProgressSubSteps();

	/// Returns the maximum progress value that can be reached (taking into account sub-steps).
    int totalProgressMaximum() const { return _totalProgressMaximum; }

	/// Returns the current progress value (taking into account sub-steps).
    int totalProgressValue() const { return _totalProgressValue; }

	/// Blocks execution until the given promise is complete.
	/// Returns false if either the sub-promise or this promise have been canceled.
	bool waitForSubTask(const PromiseBasePtr& subTask);

	/// Blocks execution until the given future is complete.
	/// Returns false if either the future or this promise have been canceled.
	bool waitForSubTask(const FutureBase& subFuture);

	/// Cancels this promise.
	virtual void cancel();

	// Implementation details:

	/// This must be called after creating a promise to put it into the 'started' state.
	/// Returns false if the promise is or was already in the 'started' state.
    bool setStarted();

	/// This must be called after the promise has been fulfilled (even if an exception occurred).
    void setFinished();

	/// Sets the promise into the 'exception' state to signal that an exception has occurred 
	/// while trying to fulfill it. This method should be called from a catch(...) exception handler.
    void setException();

	/// Sets the promise into the 'exception' state to signal that an exception has occurred 
	/// while trying to fulfill it. 
    void setException(std::exception_ptr ex);

	/// Blocks execution until this promise has been fulfilled, canceled or an exception occurred.
    void waitForFinished();

protected:

	/// Constructor.
	PromiseBase(State initialState = NoState) : _state(initialState) {}

	/// Returns true if the promise is in the 'running' state.
	bool isRunning() const { return (_state & Running); }
	
	/// Returns true if the promise is in the 'started' state.
	bool isStarted() const { return (_state & Started); }

	/// Returns true if the promise is in the 'finished' state.
	bool isFinished() const { return (_state & Finished); }

	/// Returns true if a result value (or, alternatively, an exception) has been set for this promise.
	bool isResultSet() const { return (_state & ResultSet); }

	/// Signals the associated future that a result value is available.
    void setResultReady();

	/// Re-throws the exception stored in this promise if an exception was previously set via setException().
    void throwPossibleException() {
    	if(_exceptionStore)
    		std::rethrow_exception(_exceptionStore);
    }

	/// Blocks execution until a result value (or exception) has been set for this promise.
    void waitForResult();

    void registerWatcher(PromiseWatcher* watcher);
    void unregisterWatcher(PromiseWatcher* watcher);

    void computeTotalProgress();

	virtual void tryToRunImmediately() {}

	PromiseBase* _subTask = nullptr;
	QList<PromiseWatcher*> _watchers;
	QMutex _mutex;
	State _state;
	QWaitCondition _waitCondition;
	std::exception_ptr _exceptionStore;
	int _totalProgressValue = 0;
	int _totalProgressMaximum = 0;
    int _progressValue = 0;
    int _progressMaximum = 0;
    int _intermittentUpdateCounter = 0;
    QString _progressText;
    QElapsedTimer _progressTime;
    std::vector<std::pair<int, std::vector<int>>> subStepsStack;

	friend class PromiseWatcher;
	friend class TaskManager;
	friend class FutureBase;
	template<typename R2> friend class Future;
};

/******************************************************************************
* A promise that generates a result value of a specific type.
******************************************************************************/
template<typename R>
class Promise : public PromiseBase
{
public:

	/// Default constructor.
	Promise() {}

	/// Sets the result value of this promise. A copy of the provided value is stored in the promise.
	void setResult(const R& value) {
		QMutexLocker locker(&_mutex);
		if(isCanceled() || isFinished())
			return;
		_result = value;
		_state = State(_state | ResultSet);
		setResultReady();
	}

	/// Sets the result value of this promise. The provided value is moved into the promise.
	void setResult(R&& value) {
		QMutexLocker locker(&_mutex);
		if(isCanceled() || isFinished())
			return;
		_result = std::move(value);
		_state = State(_state | ResultSet);
		setResultReady();
	}

private:

	/// The result value stored by this promise.
	R _result;

	template<typename R2, typename Function> friend class Task;
	template<typename R2> friend class Future;
};

/******************************************************************************
* A promise that generates no result value.
******************************************************************************/
template<>
class Promise<void> : public PromiseBase
{
public:

	/// Default constructor.
	Promise() {}

	template<typename R2, typename Function> friend class Task;
	template<typename R2> friend class Future;
};

/// A pointer to a promise.
template<typename T>
using PromisePtr = std::shared_ptr<Promise<T>>;

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


