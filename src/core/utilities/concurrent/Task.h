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
#include "Future.h"

#include <QRunnable>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

class AsynchronousTask : public Promise<void>, public QRunnable
{
public:

	/// Constructor.
	AsynchronousTask() {
		setAutoDelete(false);
	}

	/// This function must be implemented by subclasses to perform the actual task.
	virtual void perform() = 0;

private:

	/// Implementation of QRunnable.
	virtual void run() override {
		tryToRunImmediately();
	}

	/// Implementation of FutureInterface.
	virtual void tryToRunImmediately() override {
		if(!this->setStarted())
			return;
		try {
			perform();
		}
		catch(...) {
			this->setException();
		}
		this->setFinished();
	}
};

class OVITO_CORE_EXPORT SynchronousTask
{
public:

	/// Constructor.
	SynchronousTask(TaskManager& taskManager);

	/// Destructor.
	~SynchronousTask();

	/// Returns whether the operation has been canceled by the user.
	bool isCanceled() const;

	/// Cancels the operation.
	void cancel() { _promise->cancel(); }

	/// Sets the status text to be displayed.
	void setProgressText(const QString& text);

	/// Return the current status text.
	QString progressText() const { return _promise->progressText(); }

	/// Returns the highest value represented by the progress bar.
	int progressMaximum() const { return _promise->progressMaximum(); }

	/// Sets the highest value represented by the progress bar.
	void setProgressMaximum(int max) { _promise->setProgressMaximum(max); }

	/// Returns the value displayed by the progress bar.
	int progressValue() const { return _promise->progressValue(); }

	/// Sets the value displayed by the progress bar.
	void setProgressValue(int v);

	/// Returns the internal promise managed by this object.
	Promise<void>& promise() const { return *_promise.get(); }

private:

	PromisePtr<void> _promise;
	TaskManager& _taskManager;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


