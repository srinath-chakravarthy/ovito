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

#include <plugins/particles/gui/ParticlesGui.h>
#include <plugins/particles/modifier/selection/SelectExpressionModifier.h>
#include <gui/properties/StringParameterUI.h>
#include <gui/widgets/general/AutocompleteTextEdit.h>
#include "SelectExpressionModifierEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Selection) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(SelectExpressionModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(SelectExpressionModifier, SelectExpressionModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void SelectExpressionModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	QWidget* rollout = createRollout(tr("Expression select"), rolloutParams, "particles.modifiers.expression_select.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(0);

	layout->addWidget(new QLabel(tr("Boolean expression:")));
	StringParameterUI* expressionUI = new StringParameterUI(this, PROPERTY_FIELD(SelectExpressionModifier::_expression));
	expressionEdit = new AutocompleteTextEdit();
	expressionUI->setTextBox(expressionEdit);
	layout->addWidget(expressionUI->textBox());

	// Status label.
	layout->addSpacing(12);
	layout->addWidget(statusLabel());

	QWidget* variablesRollout = createRollout(tr("Variables"), rolloutParams.after(rollout), "particles.modifiers.expression_select.html");
    QVBoxLayout* variablesLayout = new QVBoxLayout(variablesRollout);
    variablesLayout->setContentsMargins(4,4,4,4);
	variableNamesList = new QLabel();
	variableNamesList->setWordWrap(true);
	variableNamesList->setTextInteractionFlags(Qt::TextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard | Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard));
	variablesLayout->addWidget(variableNamesList);

	// Update input variables list if another modifier has been loaded into the editor.
	connect(this, &SelectExpressionModifierEditor::contentsReplaced, this, &SelectExpressionModifierEditor::updateEditorFields);
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool SelectExpressionModifierEditor::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	if(source == editObject() && event->type() == ReferenceEvent::TargetChanged) {
		updateEditorFields();
	}
	return ParticleModifierEditor::referenceEvent(source, event);
}

/******************************************************************************
* Updates the enabled/disabled status of the editor's controls.
******************************************************************************/
void SelectExpressionModifierEditor::updateEditorFields()
{
	SelectExpressionModifier* mod = static_object_cast<SelectExpressionModifier>(editObject());
	if(!mod) return;

	variableNamesList->setText(mod->inputVariableTable() + QStringLiteral("<p></p>"));
	expressionEdit->setWordList(mod->inputVariableNames());
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
