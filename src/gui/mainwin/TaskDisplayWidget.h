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

#pragma once


#include <gui/GUI.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * \brief Displays the running tasks in the status bar of the main window.
 */
class TaskDisplayWidget : public QWidget
{
	Q_OBJECT

public:

	/// Constructs the widget and associates it with the main window.
	TaskDisplayWidget(MainWindow* mainWindow);

private Q_SLOTS:

	/// \brief Is called when a task has started to run.
	void taskStarted(PromiseWatcher* taskWatcher);

	/// \brief Is called when a task has finished.
	void taskFinished(PromiseWatcher* taskWatcher);

	/// \brief Is called when the progress or status of a task has changed.
	void taskProgressChanged();

	/// \brief Shows the progress indicator widgets.
	void showIndicator();

	/// \brief Updates the displayed information in the indicator widget.
	void updateIndicator();

private:
	
	/// The window this display widget is associated with.
	MainWindow* _mainWindow;

	/// The progress bar widget.
	QProgressBar* _progressBar;

	/// The button that lets the user cancel running tasks.
	QAbstractButton* _cancelTaskButton;

	/// The label that displays the current progress text.
	QLabel* _progressTextDisplay;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


