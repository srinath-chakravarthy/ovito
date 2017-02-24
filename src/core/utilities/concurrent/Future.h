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

#pragma once


#include <core/Core.h>
#include "Promise.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

/******************************************************************************
* Generic base class for futures, which implements the basic state management,
* progress reporting, and event processing.
******************************************************************************/
class FutureBase 
{
public:

	/// Returns true if the Promise associated with this Future has been canceled.
	bool isCanceled() const { return promise()->isCanceled(); }

	/// Returns true if the Promise associated with this Future has been completed.
	bool isFinished() const { return promise()->isFinished(); }

	/// Cancels the Promise associated with this Future.
	void cancel() { promise()->cancel(); }

	/// Blocks execution until the Promise associated with this future has been completed.
	void waitForFinished() const {
		promise()->waitForFinished();
	}

	/// Returns true if this Future is associated with a valid Promise.
	bool isValid() const { return (bool)_promise; }

	/// Dissociates this Future from its Promise.
	void reset() { _promise.reset(); }

	/// Returns the Promise associated with this Future. 
	/// Make sure it has a Promise before calling this function.
	const PromiseBasePtr& promise() const {
		OVITO_ASSERT(isValid());
		return _promise;
	}

protected:

	/// Default constructor, which creates a Future with a Promise.
	FutureBase() {}

	/// Constructor that creates a Future associated with a Promise.
	explicit FutureBase(const PromiseBasePtr& p) : _promise(p) {}
	
	/// The Promise associated with this Future.
	PromiseBasePtr _promise;

	template<typename R2, typename Function> friend class Task;
	template<typename R2> friend class Promise;
	friend class PromiseBase;
	friend class PromiseWatcher;
	friend class TaskManager;
};

/******************************************************************************
* A future that provides access to the value computed by a Promise.
******************************************************************************/
template<typename R>
class Future : public FutureBase 
{
public:
	typedef Promise<R> PromiseType;
	typedef PromisePtr<R> PromisePtrType;

	/// Default constructor that coonstructs an invalid Future that is not associated with any Promise.
	Future() {}

	/// Constructor that constructs a Future that is associated with the given Promise.
	explicit Future(const PromisePtrType& p) : FutureBase(p) {}

	/// Creates a new Future and its associated Promise. The Promise is not started yet.
	static Future createWithPromise() {
		return Future(std::make_shared<PromiseType>());
	}

	/// Creates an already completed Future with a result value that is immediately available.
	static Future createImmediate(const R& result, const QString& statusText = QString()) {
		auto promise = std::make_shared<PromiseType>();
		promise->setStarted();
		if(statusText.isEmpty() == false)
			promise->setProgressText(statusText);
		promise->setResult(result);
		promise->setFinished();
		return Future(promise);
	}

	/// Creates an already completed Future with a result value that is immediately available.
	static Future createImmediate(R&& result, const QString& statusText = QString()) {
		auto promise = std::make_shared<PromiseType>();
		promise->setStarted();
		if(statusText.isEmpty() == false)
			promise->setProgressText(statusText);
		promise->setResult(std::move(result));
		promise->setFinished();
		return Future(promise);
	}

	/// Creates a completed Future that is in the 'exception' state.
	static Future createFailed(const Exception& ex) {
		auto promise = std::make_shared<PromiseType>();
		promise->setStarted();
		promise->setException(std::make_exception_ptr(ex));
		promise->setFinished();
		return Future(promise);
	}

	/// Creates a Future without results that is in the canceled state.
	static Future createCanceled() {
		auto promise = std::make_shared<PromiseType>();
		promise->setStarted();
		promise->cancel();
		promise->setFinished();
		return Future(promise);
	}

	/// Returns the results computed by the associated Promise.
	/// Blocks execution until the results become available.
	/// Throws an exception if an error occurred while the promise was computing
	/// the results, or if the promise has been canceled.
	const R& result() const {
		promise()->waitForResult();
		return static_cast<Promise<R>*>(promise().get())->_result;
	}

	/// Returns the Promise associated with this Future. 
	/// Make sure it has a Promise before calling this function.
	PromisePtrType promise() const {
		return std::static_pointer_cast<PromiseType>(FutureBase::promise());
	}
};

/******************************************************************************
* A future without a result.
******************************************************************************/
template<>
class Future<void> : public FutureBase 
{
public:
	typedef Promise<void> PromiseType;
	typedef PromisePtr<void> PromisePtrType;

	/// Default constructor that coonstructs an invalid Future that is not associated with any Promise.
	Future() {}

	/// Constructor that constructs a Future that is associated with the given Promise.
	explicit Future(const PromisePtrType& p) : FutureBase(p) {}

	/// Creates a new Future and its associated Promise. The Promise is not started yet.
	static Future createWithPromise() {
		return Future(std::make_shared<PromiseType>());
	}

	/// Creates a Future that is already complete.
	static Future createImmediate(const QString& statusText = QString()) {
		auto promise = std::make_shared<PromiseType>();
		promise->setStarted();
		if(statusText.isEmpty() == false)
			promise->setProgressText(statusText);
		promise->setFinished();
		return Future(promise);
	}

	/// Create a Future without results that is in the canceled state.
	static Future createCanceled() {
		auto promise = std::make_shared<PromiseType>();
		promise->setStarted();
		promise->cancel();
		promise->setFinished();
		return Future(promise);
	}

	/// Blocks execution until the Promise associated with this Future has finished.
	/// Throws an exception if an error occurred while the promise was running or if the promise has been canceled.
	void result() const {
		promise()->waitForResult();
	}

	/// Returns the Promise associated with this Future. 
	/// Make sure it has a Promise before calling this function.
	PromisePtrType promise() const {
		return std::static_pointer_cast<PromiseType>(FutureBase::promise());
	}
};

/// Part of PromiseWatcher implementation.
inline void PromiseWatcher::setFuture(const FutureBase& future)
{
	setPromise(future.promise());
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


