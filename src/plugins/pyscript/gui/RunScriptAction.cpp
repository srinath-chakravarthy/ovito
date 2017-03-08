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

#include <plugins/pyscript/PyScript.h>
#include <core/dataset/DataSetContainer.h>
#include <gui/utilities/concurrent/ProgressDialog.h>
#include <gui/actions/ActionManager.h>
#include <gui/dialogs/HistoryFileDialog.h>
#include <gui/mainwin/MainWindow.h>
#include <plugins/pyscript/engine/ScriptEngine.h>
#include "RunScriptAction.h"

namespace PyScript {

IMPLEMENT_OVITO_OBJECT(RunScriptAction, GuiAutoStartObject);

/******************************************************************************
* Is called when a new main window is created.
******************************************************************************/
void RunScriptAction::registerActions(ActionManager& actionManager)
{
	// Register an action, which allows the user to run a Python script file.
	QAction* runScriptFileAction = actionManager.createCommandAction(ACTION_SCRIPTING_RUN_FILE, tr("Run Script File..."));

	connect(runScriptFileAction, &QAction::triggered, [&actionManager]() {
		// Let the user select a script file on disk.
		HistoryFileDialog dlg("ScriptFile", actionManager.mainWindow(), tr("Run Script File"), QString(), tr("Python scripts (*.py)"));
		if(dlg.exec() != QDialog::Accepted)
			return;
		QString scriptFile = dlg.selectedFiles().front();
		DataSet* dataset = actionManager.mainWindow()->datasetContainer().currentSet();

		// Execute the script file.
		// Keep undo records so that script actions can be undone.
		dataset->undoStack().beginCompoundOperation(tr("Script actions"));
		try {
			// Show a progress dialog while script is running.
			ProgressDialog progressDialog(actionManager.mainWindow(), tr("Script execution"));	

			ScriptEngine engine(dataset, progressDialog.taskManager(), true);

			engine.executeFile(scriptFile);
		}
		catch(const Exception& ex) {
			ex.reportError();
		}
		dataset->undoStack().endCompoundOperation();
	});
}

};
