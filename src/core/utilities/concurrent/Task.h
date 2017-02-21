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

#ifndef __OVITO_TASK_H
#define __OVITO_TASK_H

#include <core/Core.h>
#include "FutureInterface.h"
#include "Future.h"

#include <QRunnable>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

class AsynchronousTask : public FutureInterface<void>, public QRunnable
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
		if(!this->reportStarted())
			return;
		try {
			perform();
		}
		catch(...) {
			this->reportException();
		}
		this->reportFinished();
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
	bool wasCanceled() const { return _futureInterface->isCanceled(); }

	/// Cancels the operation.
	void cancel() { _futureInterface->cancel(); }

	/// Sets the status text to be displayed.
	void setStatusText(const QString& text);

	/// Return the current status text.
	QString statusText() const { return _futureInterface->progressText(); }

	/// Returns the highest value represented by the progress bar.
	int maximum() const { return _futureInterface->progressMaximum(); }

	/// Sets the highest value represented by the progress bar.
	void setMaximum(int max) { _futureInterface->setProgressRange(max); }

	/// Returns the value displayed by the progress bar.
	int value() const { return _futureInterface->progressValue(); }

	/// Sets the value displayed by the progress bar.
	void setValue(int v);

private:

	std::shared_ptr<FutureInterface<void>> _futureInterface;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace

#endif // __OVITO_TASK_H
