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


#include <gui/GUI.h>
#include <core/utilities/concurrent/TaskManager.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

class OVITO_GUI_EXPORT ProgressDialog : public QDialog
{
public:

	/// Constructor.
	ProgressDialog(MainWindow* mainWindow, const QString& dialogTitle = QString());

	/// Constructor.
	ProgressDialog(QWidget* parent, TaskManager& taskManager, const QString& dialogTitle = QString());

	/// ~Destructor.
	~ProgressDialog();

	/// Returns the TaskManager that manages the running task displayed in this progress dialog.
	TaskManager& taskManager() { return _taskManager; }

protected:

	/// Is called when the user tries to close the dialog.
	virtual void closeEvent(QCloseEvent* event) override;

	/// Is called when the user tries to close the dialog.
	virtual void reject() override;

private Q_SLOTS:

	/// Is called whenever one of the tasks was canceled.
	void onTaskCanceled();

private:

	/// The task manager.
	TaskManager& _taskManager;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


