///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2016) Alexander Stukowski
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
#include <core/utilities/concurrent/TaskManager.h>
#include <core/utilities/concurrent/PromiseWatcher.h>
#include "TaskDisplayWidget.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/******************************************************************************
* Constructs the widget and associates it with the main window.
******************************************************************************/
TaskDisplayWidget::TaskDisplayWidget(MainWindow* mainWindow) : QWidget(nullptr), _mainWindow(mainWindow)
{
	setVisible(false);

	QHBoxLayout* progressWidgetLayout = new QHBoxLayout(this);
	progressWidgetLayout->setContentsMargins(0,0,0,0);
	progressWidgetLayout->setSpacing(0);
	_progressTextDisplay = new QLabel();
	_progressTextDisplay->setLineWidth(0);
	_progressTextDisplay->setAlignment(Qt::Alignment(Qt::AlignRight | Qt::AlignVCenter));
	_progressTextDisplay->setAutoFillBackground(true);
	_progressTextDisplay->setMargin(2);
	_progressBar = new QProgressBar(this);
	_cancelTaskButton = new QToolButton(this);
	_cancelTaskButton->setText(tr("Cancel"));
	QIcon cancelIcon(":/gui/mainwin/process-stop-16.png");
	cancelIcon.addFile(":/gui/mainwin/process-stop-22.png");
	_cancelTaskButton->setIcon(cancelIcon);
	progressWidgetLayout->addWidget(_progressBar);
	progressWidgetLayout->addWidget(_cancelTaskButton);
	setMinimumHeight(_progressTextDisplay->minimumSizeHint().height());

	connect(_cancelTaskButton, &QAbstractButton::clicked, &mainWindow->datasetContainer().taskManager(), &TaskManager::cancelAll);
	connect(&mainWindow->datasetContainer().taskManager(), &TaskManager::taskStarted, this, &TaskDisplayWidget::taskStarted);
	connect(&mainWindow->datasetContainer().taskManager(), &TaskManager::taskFinished, this, &TaskDisplayWidget::taskFinished);
}

/******************************************************************************
* Is called when a task has started to run.
******************************************************************************/
void TaskDisplayWidget::taskStarted(PromiseWatcher* taskWatcher)
{
	// Show progress indicator only if the task doesn't finish within 200 milliseconds.
	if(isHidden())
		QTimer::singleShot(200, this, SLOT(showIndicator()));
	else
		updateIndicator();

	connect(taskWatcher, &PromiseWatcher::progressRangeChanged, this, &TaskDisplayWidget::taskProgressChanged);
	connect(taskWatcher, &PromiseWatcher::progressValueChanged, this, &TaskDisplayWidget::taskProgressChanged);
	connect(taskWatcher, &PromiseWatcher::progressTextChanged, this, &TaskDisplayWidget::taskProgressChanged);
}

/******************************************************************************
* Is called when a task has finished.
******************************************************************************/
void TaskDisplayWidget::taskFinished(PromiseWatcher* taskWatcher)
{
	updateIndicator();
}

/******************************************************************************
* Is called when the progress of a task has changed
******************************************************************************/
void TaskDisplayWidget::taskProgressChanged()
{
	const TaskManager& taskManager = _mainWindow->datasetContainer().taskManager();
	if(taskManager.runningTasks().empty() == false)
		updateIndicator();
}

/******************************************************************************
* Shows the progress indicator widget.
******************************************************************************/
void TaskDisplayWidget::showIndicator()
{
	const TaskManager& taskManager = _mainWindow->datasetContainer().taskManager();
	if(isHidden() && taskManager.runningTasks().empty() == false) {
		_mainWindow->statusBar()->addWidget(_progressTextDisplay, 1);
		show();
		_progressTextDisplay->show();
		updateIndicator();
	}
}

/******************************************************************************
* Shows or hides the progress indicator widgets and updates the displayed information.
******************************************************************************/
void TaskDisplayWidget::updateIndicator()
{
	if(isHidden())
		return;

	const TaskManager& taskManager = _mainWindow->datasetContainer().taskManager();
	if(taskManager.runningTasks().empty()) {
		hide();
		_mainWindow->statusBar()->removeWidget(_progressTextDisplay);
	}
	else {
		for(auto iter = taskManager.runningTasks().crbegin(); iter != taskManager.runningTasks().crend(); iter++) {
			PromiseWatcher* watcher = *iter;
			if(watcher->progressMaximum() != 0 || watcher->progressText().isEmpty() == false) {
				_progressBar->setRange(0, watcher->progressMaximum());
				_progressBar->setValue(watcher->progressValue());
				_progressTextDisplay->setText(watcher->progressText());
				show();
				break;
			}
		}
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
