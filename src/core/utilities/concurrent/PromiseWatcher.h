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

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

class FutureBase;
class PromiseBase;

/// A pointer to the promise base class.
using PromiseBasePtr = std::shared_ptr<PromiseBase>;

/******************************************************************************
* A utility class that integrates the Promise class into the Qt signal/slots
* framework. It allows to have signals generated when the state of a Promise
* changes.
******************************************************************************/
class OVITO_CORE_EXPORT PromiseWatcher : public QObject
{
public:

	/// Constructor that creates a watcher that is not associated with 
	/// a Promise yet.
	PromiseWatcher(QObject* parent = nullptr) : QObject(parent) {}

	/// Destructor.
	virtual ~PromiseWatcher() {
		setPromise(nullptr, false);
	}

	/// Associates this object with the Promise of the given Future.
	void setFuture(const FutureBase& future);

	/// Associates this object with the given Promise.
	void setPromise(const PromiseBasePtr& promise) {
		setPromise(promise, true);
	}

	/// Dissociates this object from its Promise.
	void unsetPromise() {
		setPromise(nullptr, true);
	}

	/// Returns true if the Promise monitored by this object has been canceled.
	bool isCanceled() const;

	/// Returns true if the Promise monitored by this object has reached the 'finished' state.
	bool isFinished() const;

	/// Returns the maximum value for the progress of the Promise monitored by this object.
	int progressMaximum() const;

	/// Returns the current value for the progress of the Promise monitored by this object.
	int progressValue() const;

	/// Returns the status text of the Promise monitored by this object.
	QString progressText() const;

	/// Blocks execution until the Promise monitored by this object has reached the 'finished' state.
	void waitForFinished() const;

	/// Returns the Promise monitored by this object.
	const PromiseBasePtr& promise() const { return _promise; }

protected:

	void setPromise(const PromiseBasePtr& promise, bool pendingAssignment);

Q_SIGNALS:

	void canceled();
	void finished();
	void started();
	void resultReady();
	void progressRangeChanged(int maximum);
	void progressValueChanged(int progressValue);
	void progressTextChanged(const QString& progressText);

public Q_SLOTS:

	void cancel();

private Q_SLOTS:

	void promiseCanceled();
	void promiseFinished();
	void promiseStarted();
	void promiseResultReady();
	void promiseProgressRangeChanged(int maximum);
	void promiseProgressValueChanged(int progressValue);
	void promiseProgressTextChanged(QString progressText);

private:

	/// The Promise being monitored.
	PromiseBasePtr _promise;

	/// Indicates that the promise has reached the 'finished' state.
    bool _finished = false;

	Q_OBJECT

	friend class PromiseBase;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


