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

#include <gui/GUI.h>
#include <gui/mainwin/MainWindow.h>
#include <gui/widgets/general/ElidedTextLabel.h>
#include "ProgressDialog.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

/******************************************************************************
* Initializes the dialog window.
******************************************************************************/
ProgressDialog::ProgressDialog(MainWindow* mainWindow, const QString& dialogTitle) : ProgressDialog(mainWindow, mainWindow->datasetContainer().taskManager(), dialogTitle)
{
}

/******************************************************************************
* Initializes the dialog window.
******************************************************************************/
ProgressDialog::ProgressDialog(QWidget* parent, TaskManager& taskManager, const QString& dialogTitle) : QDialog(parent), _taskManager(taskManager)
{
	setWindowModality(Qt::WindowModal);
	setWindowTitle(dialogTitle);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addStretch(1);

	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
	layout->addWidget(buttonBox);

	// Cancel all currently running tasks when user presses the cancel button.
	connect(buttonBox, &QDialogButtonBox::rejected, &taskManager, &TaskManager::cancelAll);

	// Helper function that sets up the UI widgets in the dialog for a newly started task.
	auto createUIForTask = [this, layout](PromiseWatcher* taskWatcher) {
		QLabel* statusLabel = new QLabel(taskWatcher->progressText());
		statusLabel->setMaximumWidth(400);
		QProgressBar* progressBar = new QProgressBar();
		progressBar->setMaximum(taskWatcher->progressMaximum());
		progressBar->setValue(taskWatcher->progressValue());
		if(statusLabel->text().isEmpty()) {
			statusLabel->hide();
			progressBar->hide();
		}
		layout->insertWidget(layout->count() - 2, statusLabel);
		layout->insertWidget(layout->count() - 2, progressBar);
		connect(taskWatcher, &PromiseWatcher::progressRangeChanged, progressBar, &QProgressBar::setMaximum);
		connect(taskWatcher, &PromiseWatcher::progressValueChanged, progressBar, &QProgressBar::setValue);
		connect(taskWatcher, &PromiseWatcher::progressTextChanged, statusLabel, &QLabel::setText);
		connect(taskWatcher, &PromiseWatcher::progressTextChanged, statusLabel, [statusLabel, progressBar](const QString& text) {
			statusLabel->setVisible(!text.isEmpty());
			progressBar->setVisible(!text.isEmpty());
		});
		
		// Remove progress display when this task finished.
		connect(taskWatcher, &PromiseWatcher::finished, progressBar, &QObject::deleteLater);
		connect(taskWatcher, &PromiseWatcher::finished, statusLabel, &QObject::deleteLater);
	};

	// Create UI for every running task.
	for(PromiseWatcher* watcher : taskManager.runningTasks()) {
		createUIForTask(watcher);
	}

	// Create a separate progress bar for every new active task.
	connect(&taskManager, &TaskManager::taskStarted, this, createUIForTask);

	// Show the dialog with a short delay.
	// This prevents the dialog from showing up for short tasks that terminate very quickly.
	QTimer::singleShot(100, this, &QDialog::show);

	// Activate local event handling to keep the dialog responsive.
	taskManager.startLocalEventHandling();
}

/******************************************************************************
* Destructor
******************************************************************************/
ProgressDialog::~ProgressDialog()
{
	_taskManager.stopLocalEventHandling();
}

/******************************************************************************
* Is called whenever one of the tasks was canceled.
******************************************************************************/
void ProgressDialog::onTaskCanceled()
{
	// Cancel all tasks when one of the active tasks has been canceled.
	taskManager().cancelAll();
}

/******************************************************************************
* Is called when the user tries to close the dialog.
******************************************************************************/
void ProgressDialog::closeEvent(QCloseEvent* event)
{
	taskManager().cancelAll();
	if(event->spontaneous())
		event->ignore();
	QDialog::closeEvent(event);
}

/******************************************************************************
* Is called when the user tries to close the dialog.
******************************************************************************/
void ProgressDialog::reject()
{
	taskManager().cancelAll();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
