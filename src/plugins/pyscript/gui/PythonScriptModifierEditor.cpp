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
#include <gui/mainwin/MainWindow.h>
#include <gui/actions/ActionManager.h>
#include <gui/properties/StringParameterUI.h>
#include <plugins/pyscript/extensions/PythonScriptModifier.h>
#include "PythonScriptModifierEditor.h"
#include "ObjectScriptEditor.h"

namespace PyScript { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(PythonScriptModifierEditor, PropertiesEditor);
SET_OVITO_OBJECT_EDITOR(PythonScriptModifier, PythonScriptModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void PythonScriptModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Python script"), rolloutParams, "particles.modifiers.python_script.html");

    // Create the rollout contents.
	QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	int row = 0;

	QHBoxLayout* sublayout = new QHBoxLayout();
	sublayout->setContentsMargins(0,0,0,0);
	sublayout->setSpacing(10);

	StringParameterUI* namePUI = new StringParameterUI(this, PROPERTY_FIELD(Modifier::_title));
	layout->addWidget(new QLabel(tr("User-defined modifier name:")), row++, 0);
	static_cast<QLineEdit*>(namePUI->textBox())->setPlaceholderText(PythonScriptModifier::OOType.displayName());
	sublayout->addWidget(namePUI->textBox(), 1);
	layout->addLayout(sublayout, row++, 0);

	QToolButton* savePresetButton = new QToolButton();
	savePresetButton->setDefaultAction(mainWindow()->actionManager()->getAction(ACTION_MODIFIER_CREATE_PRESET));
	sublayout->addWidget(savePresetButton);

	_editScriptButton = new QPushButton(tr("Edit script..."));
	layout->addWidget(_editScriptButton, row++, 0);
	connect(_editScriptButton, &QPushButton::clicked, this, &PythonScriptModifierEditor::onOpenEditor);

	layout->addWidget(new QLabel(tr("Script output:")), row++, 0);
	_outputDisplay = new QTextEdit();
	_outputDisplay->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
	_outputDisplay->setReadOnly(true);
	_outputDisplay->setLineWrapMode(QTextEdit::NoWrap);
	layout->addWidget(_outputDisplay, row++, 0);

	connect(this, &PropertiesEditor::contentsChanged, this, &PythonScriptModifierEditor::onContentsChanged);
}

/******************************************************************************
* Is called when the current edit object has generated a change
* event or if a new object has been loaded into editor.
******************************************************************************/
void PythonScriptModifierEditor::onContentsChanged(RefTarget* editObject)
{
	PythonScriptModifier* modifier = static_object_cast<PythonScriptModifier>(editObject);
	if(modifier) {
		_editScriptButton->setEnabled(true);
		_outputDisplay->setText(modifier->scriptLogOutput());
	}
	else {
		_editScriptButton->setEnabled(false);
		_outputDisplay->clear();
	}
}

/******************************************************************************
* Is called when the user presses the 'Apply' button to commit the Python script.
******************************************************************************/
void PythonScriptModifierEditor::onOpenEditor()
{
	PythonScriptModifier* modifier = static_object_cast<PythonScriptModifier>(editObject());
	if(!modifier) return;

	class ScriptEditor : public ObjectScriptEditor {
	public:
		ScriptEditor(QWidget* parentWidget, RefTarget* scriptableObject) : ObjectScriptEditor(parentWidget, scriptableObject) {}
	protected:
		/// Obtains the current script from the owner object.
		virtual const QString& getObjectScript(RefTarget* obj) const override {
			return static_object_cast<PythonScriptModifier>(obj)->script();
		}

		/// Obtains the script output cached by the owner object.
		virtual const QString& getOutputText(RefTarget* obj) const override {
			return static_object_cast<PythonScriptModifier>(obj)->scriptLogOutput();
		}

		/// Sets the current script of the owner object.
		virtual void setObjectScript(RefTarget* obj, const QString& script) const override {
			UndoableTransaction::handleExceptions(obj->dataset()->undoStack(), tr("Commit script"), [obj, &script]() {
				static_object_cast<PythonScriptModifier>(obj)->setScript(script);
			});
		}
	};

	// First check if there is already an open editor.
	if(ObjectScriptEditor* editor = ObjectScriptEditor::findEditorForObject(modifier)) {
		editor->show();
		editor->activateWindow();
		return;
	}

	ScriptEditor* editor = new ScriptEditor(mainWindow(), modifier);
	editor->show();
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool PythonScriptModifierEditor::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	if(source == editObject() && event->type() == ReferenceEvent::ObjectStatusChanged) {
		if(PythonScriptModifier* modifier = static_object_cast<PythonScriptModifier>(editObject()))
			_outputDisplay->setText(modifier->scriptLogOutput());
	}
	return PropertiesEditor::referenceEvent(source, event);
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
