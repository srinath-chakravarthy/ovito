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


#include <plugins/pyscript/PyScript.h>
#include <core/reference/RefTargetListener.h>
#include <gui/GUI.h>

class QsciScintilla;

namespace PyScript {

using namespace Ovito;

/**
 * \brief A script editor UI component.
 */
class ObjectScriptEditor : public QMainWindow
{
	Q_OBJECT

public:

	/// Constructor.
	ObjectScriptEditor(QWidget* parentWidget, RefTarget* scriptableObject);

	/// Returns an existing editor window for the given object if there is one.
	static ObjectScriptEditor* findEditorForObject(RefTarget* scriptableObject);	

public Q_SLOTS:

	/// Compiles/runs the current script.
	void onCommitScript();

	/// Lets the user load a script file into the editor.
	void onLoadScriptFromFile();

	/// Lets the user save the current script to a file.
	void onSaveScriptToFile();

	/// Returns the path where the current script is stored.
	//const QString& filePath() const { return _filePath; }

	/// Sets the path where current script is stored.
	//void setFilePath(const QString& path) { _filePath = path; }

protected Q_SLOTS:

	/// Is called when the scriptable object generates an event.
	void onNotificationEvent(ReferenceEvent* event);

	/// Replaces the editor contents with the script from the owning object.
	void updateEditorContents();

	/// Replaces the output window contents with the script output cached by the owning object.
	void updateOutputWindow();

protected:

	/// Obtains the current script from the owner object.
	virtual const QString& getObjectScript(RefTarget* obj) const = 0;

	/// Obtains the script output cached by the owner object.
	virtual const QString& getOutputText(RefTarget* obj) const = 0;

	/// Sets the current script of the owner object.
	virtual void setObjectScript(RefTarget* obj, const QString& script) const = 0;

	/// Is called when the user closes the window.
	virtual void closeEvent(QCloseEvent* event) override;

	/// Is called when the window is shown.
	virtual void showEvent(QShowEvent* event) override;

	/// The main text editor component.
	QsciScintilla* _codeEditor;

	/// The text box that displays the script's output.
	QsciScintilla* _outputWindow;

	/// The object to which the script belongs that is opened in the editor.
	RefTargetListener<RefTarget> _scriptableObject;

	/// The action that undoes the last edit operation.
	QAction* _undoAction;

	/// The action that redoes the last undone edit operation.
	QAction* _redoAction;

	/// The file path where the current script is stored.
	//QString _filePath;
};

};


