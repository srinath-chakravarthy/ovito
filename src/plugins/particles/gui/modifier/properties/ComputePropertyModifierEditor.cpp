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
#include <plugins/particles/modifier/properties/ComputePropertyModifier.h>
#include <plugins/particles/gui/util/ParticlePropertyParameterUI.h>
#include <gui/properties/BooleanGroupBoxParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/properties/StringParameterUI.h>
#include <gui/properties/VariantComboBoxParameterUI.h>
#include <gui/widgets/general/AutocompleteLineEdit.h>
#include <core/animation/AnimationSettings.h>
#include <core/scene/pipeline/PipelineObject.h>
#include "ComputePropertyModifierEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Properties) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(ParticlesGui, ComputePropertyModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(ComputePropertyModifier, ComputePropertyModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void ComputePropertyModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	rollout = createRollout(tr("Compute property"), rolloutParams, "particles.modifiers.compute_property.html");

    // Create the rollout contents.
	QVBoxLayout* mainLayout = new QVBoxLayout(rollout);
	mainLayout->setContentsMargins(4,4,4,4);

	QGroupBox* propertiesGroupBox = new QGroupBox(tr("Output property"), rollout);
	mainLayout->addWidget(propertiesGroupBox);
	QVBoxLayout* propertiesLayout = new QVBoxLayout(propertiesGroupBox);
	propertiesLayout->setContentsMargins(6,6,6,6);
	propertiesLayout->setSpacing(4);

	// Output property
	ParticlePropertyParameterUI* outputPropertyUI = new ParticlePropertyParameterUI(this, PROPERTY_FIELD(ComputePropertyModifier::_outputProperty), false, false);
	propertiesLayout->addWidget(outputPropertyUI->comboBox());

	// Create the check box for the selection flag.
	BooleanParameterUI* selectionFlagUI = new BooleanParameterUI(this, PROPERTY_FIELD(ComputePropertyModifier::_onlySelectedParticles));
	propertiesLayout->addWidget(selectionFlagUI->checkBox());

	expressionsGroupBox = new QGroupBox(tr("Expression"));
	mainLayout->addWidget(expressionsGroupBox);
	expressionsLayout = new QGridLayout(expressionsGroupBox);
	expressionsLayout->setContentsMargins(4,4,4,4);
	expressionsLayout->setSpacing(1);
	expressionsLayout->setColumnStretch(1,1);

	// Status label.
	mainLayout->addWidget(statusLabel());

    // Neighbor mode panel.
	QWidget* neighorRollout = createRollout(tr("Neighbor particles"), rolloutParams.after(rollout), "particles.modifiers.compute_property.html");

	mainLayout = new QVBoxLayout(neighorRollout);
	mainLayout->setContentsMargins(4,4,4,4);

	BooleanGroupBoxParameterUI* neighborModeUI = new BooleanGroupBoxParameterUI(this, PROPERTY_FIELD(ComputePropertyModifier::_neighborModeEnabled));
	mainLayout->addWidget(neighborModeUI->groupBox());

	QGridLayout* gridlayout = new QGridLayout(neighborModeUI->childContainer());
	gridlayout->setContentsMargins(4,4,4,4);
	gridlayout->setColumnStretch(1, 1);
	gridlayout->setRowStretch(1, 1);

	// Cutoff parameter.
	FloatParameterUI* cutoffRadiusUI = new FloatParameterUI(this, PROPERTY_FIELD(ComputePropertyModifier::_cutoff));
	gridlayout->addWidget(cutoffRadiusUI->label(), 0, 0);
	gridlayout->addLayout(cutoffRadiusUI->createFieldLayout(), 0, 1);

	neighborExpressionsGroupBox = new QGroupBox(tr("Neighbor expression"));
	gridlayout->addWidget(neighborExpressionsGroupBox, 1, 0, 1, 2);
	neighborExpressionsLayout = new QGridLayout(neighborExpressionsGroupBox);
	neighborExpressionsLayout->setContentsMargins(4,4,4,4);
	neighborExpressionsLayout->setSpacing(1);
	neighborExpressionsLayout->setColumnStretch(1,1);

	QWidget* variablesRollout = createRollout(tr("Variables"), rolloutParams.after(neighorRollout), "particles.modifiers.compute_property.html");
    QVBoxLayout* variablesLayout = new QVBoxLayout(variablesRollout);
    variablesLayout->setContentsMargins(4,4,4,4);
    variableNamesDisplay = new QLabel();
	variableNamesDisplay->setWordWrap(true);
	variableNamesDisplay->setTextInteractionFlags(Qt::TextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard | Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard));
	variablesLayout->addWidget(variableNamesDisplay);

	// Update input variables list if another modifier has been loaded into the editor.
	connect(this, &ComputePropertyModifierEditor::contentsReplaced, this, &ComputePropertyModifierEditor::updateEditorFields);
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool ComputePropertyModifierEditor::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	if(source == editObject() && (event->type() == ReferenceEvent::TargetChanged || event->type() == ReferenceEvent::ObjectStatusChanged)) {
		if(!editorUpdatePending) {
			editorUpdatePending = true;
			QMetaObject::invokeMethod(this, "updateEditorFields", Qt::QueuedConnection, Q_ARG(bool, event->type() == ReferenceEvent::TargetChanged));
		}
	}
	return ParticleModifierEditor::referenceEvent(source, event);
}

/******************************************************************************
* Updates the enabled/disabled status of the editor's controls.
******************************************************************************/
void ComputePropertyModifierEditor::updateEditorFields(bool updateExpressions)
{
	editorUpdatePending = false;

	ComputePropertyModifier* mod = static_object_cast<ComputePropertyModifier>(editObject());
	if(!mod) return;

	const QStringList& expr = mod->expressions();
	expressionsGroupBox->setTitle((expr.size() <= 1) ? tr("Expression") : tr("Expressions"));
	while(expr.size() > expressionBoxes.size()) {
		QLabel* label = new QLabel();
		AutocompleteLineEdit* edit = new AutocompleteLineEdit();
		expressionsLayout->addWidget(label, expressionBoxes.size(), 0);
		expressionsLayout->addWidget(edit, expressionBoxes.size(), 1);
		expressionBoxes.push_back(edit);
		expressionBoxLabels.push_back(label);
		connect(edit, &AutocompleteLineEdit::editingFinished, this, &ComputePropertyModifierEditor::onExpressionEditingFinished);
	}
	while(expr.size() < expressionBoxes.size()) {
		delete expressionBoxes.takeLast();
		delete expressionBoxLabels.takeLast();
	}
	OVITO_ASSERT(expressionBoxes.size() == expr.size());
	OVITO_ASSERT(expressionBoxLabels.size() == expr.size());

	const QStringList& neighExpr = mod->neighborExpressions();
	neighborExpressionsGroupBox->setTitle((neighExpr.size() <= 1) ? tr("Neighbor expression") : tr("Neighbor expressions"));
	while(neighExpr.size() > neighborExpressionBoxes.size()) {
		QLabel* label = new QLabel();
		AutocompleteLineEdit* edit = new AutocompleteLineEdit();
		neighborExpressionsLayout->addWidget(label, neighborExpressionBoxes.size(), 0);
		neighborExpressionsLayout->addWidget(edit, neighborExpressionBoxes.size(), 1);
		neighborExpressionBoxes.push_back(edit);
		neighborExpressionBoxLabels.push_back(label);
		connect(edit, &AutocompleteLineEdit::editingFinished, this, &ComputePropertyModifierEditor::onExpressionEditingFinished);
	}
	while(neighExpr.size() < neighborExpressionBoxes.size()) {
		delete neighborExpressionBoxes.takeLast();
		delete neighborExpressionBoxLabels.takeLast();
	}
	OVITO_ASSERT(neighborExpressionBoxes.size() == neighExpr.size());
	OVITO_ASSERT(neighborExpressionBoxLabels.size() == neighExpr.size());

	QStringList standardPropertyComponentNames;
	if(mod->outputProperty().type() != ParticleProperty::UserProperty) {
		standardPropertyComponentNames = ParticleProperty::standardPropertyComponentNames(mod->outputProperty().type());
	}

	QStringList inputVariableNames = mod->inputVariableNames();
	if(mod->neighborModeEnabled()) {
		inputVariableNames.append(QStringLiteral("Cutoff"));
		inputVariableNames.append(QStringLiteral("NumNeighbors"));
	}
	for(int i = 0; i < expr.size(); i++) {
		if(updateExpressions)
			expressionBoxes[i]->setText(expr[i]);
		expressionBoxes[i]->setWordList(inputVariableNames);
		if(expr.size() == 1)
			expressionBoxLabels[i]->hide();
		else {
			if(i < standardPropertyComponentNames.size())
				expressionBoxLabels[i]->setText(tr("%1:").arg(standardPropertyComponentNames[i]));
			else
				expressionBoxLabels[i]->setText(tr("%1:").arg(i+1));
			expressionBoxLabels[i]->show();
		}
	}
	if(mod->neighborModeEnabled()) {
		inputVariableNames.append(QStringLiteral("Distance"));
		inputVariableNames.append(QStringLiteral("Delta.X"));
		inputVariableNames.append(QStringLiteral("Delta.Y"));
		inputVariableNames.append(QStringLiteral("Delta.Z"));
	}
	for(int i = 0; i < neighExpr.size(); i++) {
		if(updateExpressions)
			neighborExpressionBoxes[i]->setText(neighExpr[i]);
		neighborExpressionBoxes[i]->setWordList(inputVariableNames);
		if(expr.size() == 1)
			neighborExpressionBoxLabels[i]->hide();
		else {
			if(i < standardPropertyComponentNames.size())
				neighborExpressionBoxLabels[i]->setText(tr("%1:").arg(standardPropertyComponentNames[i]));
			else
				neighborExpressionBoxLabels[i]->setText(tr("%1:").arg(i+1));
			neighborExpressionBoxLabels[i]->show();
		}
	}

	QString variableList = mod->inputVariableTable();
	if(mod->neighborModeEnabled()) {
		variableList.append(QStringLiteral("<p><b>Neighbor parameters:</b><ul>"));
		variableList.append(QStringLiteral("<li>Cutoff (<i style=\"color: #555;\">radius</i>)</li>"));
		variableList.append(QStringLiteral("<li>NumNeighbors (<i style=\"color: #555;\">of central particle</i>)</li>"));
		variableList.append(QStringLiteral("<li>Distance (<i style=\"color: #555;\">from central particle</i>)</li>"));
		variableList.append(QStringLiteral("<li>Delta.X (<i style=\"color: #555;\">neighbor vector</i>)</li>"));
		variableList.append(QStringLiteral("<li>Delta.Y (<i style=\"color: #555;\">neighbor vector</i>)</li>"));
		variableList.append(QStringLiteral("<li>Delta.X (<i style=\"color: #555;\">neighbor vector</i>)</li>"));
		variableList.append(QStringLiteral("</ul></p>"));
	}
	variableList.append(QStringLiteral("<p></p>"));
	variableNamesDisplay->setText(variableList);

	neighborExpressionsGroupBox->updateGeometry();
	container()->updateRolloutsLater();
}

/******************************************************************************
* Is called when the user has typed in an expression.
******************************************************************************/
void ComputePropertyModifierEditor::onExpressionEditingFinished()
{
	ComputePropertyModifier* mod = static_object_cast<ComputePropertyModifier>(editObject());
	AutocompleteLineEdit* edit = (AutocompleteLineEdit*)sender();
	int index = expressionBoxes.indexOf(edit);
	if(index >= 0) {
		undoableTransaction(tr("Change expression"), [mod, edit, index]() {
			QStringList expr = mod->expressions();
			expr[index] = edit->text();
			mod->setExpressions(expr);
		});
	}
	else {
		int index = neighborExpressionBoxes.indexOf(edit);
		OVITO_ASSERT(index >= 0);
		undoableTransaction(tr("Change neighbor function"), [mod, edit, index]() {
			QStringList expr = mod->neighborExpressions();
			expr[index] = edit->text();
			mod->setNeighborExpressions(expr);
		});
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
