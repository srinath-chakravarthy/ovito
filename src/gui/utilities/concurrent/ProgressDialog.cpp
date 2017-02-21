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
#include "ProgressDialog.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Concurrency)

/******************************************************************************
* Initializes the dialog window.
******************************************************************************/
ProgressDialog::ProgressDialog(QWidget* parent) : QDialog(parent) 
{
	setWindowModality(Qt::WindowModal);

	QVBoxLayout* layout = new QVBoxLayout(this);
	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
	layout->addWidget(buttonBox);

	// Cancel all running tasks when user presses the cancel button.
	connect(buttonBox, &QDialogButtonBox::rejected, &taskManager(), &TaskManager::cancelAll);

	// Create a separate progress bar for every active task.
	connect(&taskManager(), &TaskManager::taskStarted, this, [this, layout](FutureWatcher* taskWatcher) {
		QLabel* statusLabel = new QLabel();
		QProgressBar* progressBar = new QProgressBar();
		layout->insertWidget(layout->count() - 1, statusLabel);
		layout->insertWidget(layout->count() - 1, progressBar);
		connect(taskWatcher, &FutureWatcher::progressRangeChanged, progressBar, &QProgressBar::setMaximum);
		connect(taskWatcher, &FutureWatcher::progressValueChanged, progressBar, &QProgressBar::setValue);
		connect(taskWatcher, &FutureWatcher::progressTextChanged, statusLabel, &QLabel::setText);

		// Remove progress display when task finished.
		connect(taskWatcher, &FutureWatcher::finished, progressBar, &QObject::deleteLater);
		connect(taskWatcher, &FutureWatcher::finished, statusLabel, &QObject::deleteLater);
	});

	show();
}

/******************************************************************************
* Is called whenever one of the tasks was canceled.
******************************************************************************/
void ProgressDialog::onTaskCanceled()
{
	// Cancel all tasks when one of the active tasks has been canceled.
	taskManager().cancelAll();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
