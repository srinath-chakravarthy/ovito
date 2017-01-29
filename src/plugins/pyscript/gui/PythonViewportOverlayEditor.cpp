///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2014) Alexander Stukowski
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
#include <plugins/pyscript/extensions/PythonViewportOverlay.h>
#include <gui/mainwin/MainWindow.h>
#include "PythonViewportOverlayEditor.h"
#include "ObjectScriptEditor.h"

namespace PyScript { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(PythonViewportOverlayEditor, PropertiesEditor);
SET_OVITO_OBJECT_EDITOR(PythonViewportOverlay, PythonViewportOverlayEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void PythonViewportOverlayEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Python script"), rolloutParams, "viewport_overlays.python_script.html");

    // Create the rollout contents.
	QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	_editScriptButton = new QPushButton(tr("Edit script..."));
	layout->addWidget(_editScriptButton, 0, 0);
	connect(_editScriptButton, &QPushButton::clicked, this, &PythonViewportOverlayEditor::onOpenEditor);

	layout->addWidget(new QLabel(tr("Script output:")), 1, 0);
	_outputDisplay = new QTextEdit();
	_outputDisplay->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
	_outputDisplay->setReadOnly(true);
	_outputDisplay->setLineWrapMode(QTextEdit::NoWrap);
	layout->addWidget(_outputDisplay, 2, 0);

	connect(this, &PropertiesEditor::contentsChanged, this, &PythonViewportOverlayEditor::onContentsChanged);
}

/******************************************************************************
* Is called when the current edit object has generated a change
* event or if a new object has been loaded into editor.
******************************************************************************/
void PythonViewportOverlayEditor::onContentsChanged(RefTarget* editObject)
{
	PythonViewportOverlay* overlay = static_object_cast<PythonViewportOverlay>(editObject);
	if(overlay) {
		_editScriptButton->setEnabled(true);
		_outputDisplay->setText(overlay->scriptOutput());
	}
	else {
		_editScriptButton->setEnabled(false);
		_outputDisplay->clear();
	}
}

/******************************************************************************
* Is called when the user presses the 'Apply' button to commit the Python script.
******************************************************************************/
void PythonViewportOverlayEditor::onOpenEditor()
{
	PythonViewportOverlay* overlay = static_object_cast<PythonViewportOverlay>(editObject());
	if(!overlay) return;

	class OverlayScriptEditor : public ObjectScriptEditor {
	public:
		OverlayScriptEditor(QWidget* parentWidget, RefTarget* scriptableObject) : ObjectScriptEditor(parentWidget, scriptableObject) {}
	protected:
		/// Obtains the current script from the owner object.
		virtual const QString& getObjectScript(RefTarget* obj) const override {
			return static_object_cast<PythonViewportOverlay>(obj)->script();
		}

		/// Obtains the script output cached by the owner object.
		virtual const QString& getOutputText(RefTarget* obj) const override {
			return static_object_cast<PythonViewportOverlay>(obj)->scriptOutput();
		}

		/// Sets the current script of the owner object.
		virtual void setObjectScript(RefTarget* obj, const QString& script) const override {
			UndoableTransaction::handleExceptions(obj->dataset()->undoStack(), tr("Commit script"), [obj, &script]() {
				static_object_cast<PythonViewportOverlay>(obj)->setScript(script);
			});
		}
	};

	// First check if there is already an open editor.
	if(ObjectScriptEditor* editor = ObjectScriptEditor::findEditorForObject(overlay)) {
		editor->show();
		editor->activateWindow();
		return;
	}

	OverlayScriptEditor* editor = new OverlayScriptEditor(mainWindow(), overlay);
	editor->show();
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool PythonViewportOverlayEditor::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	if(source == editObject() && event->type() == ReferenceEvent::ObjectStatusChanged) {
		if(PythonViewportOverlay* overlay = static_object_cast<PythonViewportOverlay>(editObject()))
			_outputDisplay->setText(overlay->scriptOutput());
	}
	return PropertiesEditor::referenceEvent(source, event);
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
