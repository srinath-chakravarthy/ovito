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
#include <gui/properties/StringParameterUI.h>
#include <gui/properties/ColorParameterUI.h>
#include <gui/properties/FontParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/VariantComboBoxParameterUI.h>
#include <gui/widgets/general/AutocompleteTextEdit.h>
#include <gui/actions/ViewportModeAction.h>
#include <gui/viewport/overlay/MoveOverlayInputMode.h>
#include <core/viewport/overlay/TextLabelOverlay.h>
#include <core/animation/AnimationSettings.h>
#include "TextLabelOverlayEditor.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(View) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(Gui, TextLabelOverlayEditor, PropertiesEditor);
SET_OVITO_OBJECT_EDITOR(TextLabelOverlay, TextLabelOverlayEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void TextLabelOverlayEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Text label"), rolloutParams);

    // Create the rollout contents.
	QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1, 1);

	// Label text.
	StringParameterUI* labelTextPUI = new StringParameterUI(this, PROPERTY_FIELD(TextLabelOverlay::_labelText));
	layout->addWidget(new QLabel(tr("Text:")), 0, 0);
	_textEdit = new AutocompleteTextEdit();
	labelTextPUI->setTextBox(_textEdit);
	layout->addWidget(labelTextPUI->textBox(), 0, 1);

	VariantComboBoxParameterUI* alignmentPUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(TextLabelOverlay::_alignment));
	layout->addWidget(new QLabel(tr("Position:")), 1, 0);
	layout->addWidget(alignmentPUI->comboBox(), 1, 1);
	alignmentPUI->comboBox()->addItem(tr("Top"), QVariant::fromValue((int)(Qt::AlignTop | Qt::AlignHCenter)));
	alignmentPUI->comboBox()->addItem(tr("Top left"), QVariant::fromValue((int)(Qt::AlignTop | Qt::AlignLeft)));
	alignmentPUI->comboBox()->addItem(tr("Top right"), QVariant::fromValue((int)(Qt::AlignTop | Qt::AlignRight)));
	alignmentPUI->comboBox()->addItem(tr("Bottom"), QVariant::fromValue((int)(Qt::AlignBottom | Qt::AlignHCenter)));
	alignmentPUI->comboBox()->addItem(tr("Bottom left"), QVariant::fromValue((int)(Qt::AlignBottom | Qt::AlignLeft)));
	alignmentPUI->comboBox()->addItem(tr("Bottom right"), QVariant::fromValue((int)(Qt::AlignBottom | Qt::AlignRight)));
	alignmentPUI->comboBox()->addItem(tr("Left"), QVariant::fromValue((int)(Qt::AlignVCenter | Qt::AlignLeft)));
	alignmentPUI->comboBox()->addItem(tr("Right"), QVariant::fromValue((int)(Qt::AlignVCenter | Qt::AlignRight)));

	FloatParameterUI* offsetXPUI = new FloatParameterUI(this, PROPERTY_FIELD(TextLabelOverlay::_offsetX));
	layout->addWidget(offsetXPUI->label(), 2, 0);
	layout->addLayout(offsetXPUI->createFieldLayout(), 2, 1);

	FloatParameterUI* offsetYPUI = new FloatParameterUI(this, PROPERTY_FIELD(TextLabelOverlay::_offsetY));
	layout->addWidget(offsetYPUI->label(), 3, 0);
	layout->addLayout(offsetYPUI->createFieldLayout(), 3, 1);

	ViewportInputMode* moveOverlayMode = new MoveOverlayInputMode(this);
	ViewportModeAction* moveOverlayAction = new ViewportModeAction(mainWindow(), tr("Move using mouse"), this, moveOverlayMode);
	layout->addWidget(moveOverlayAction->createPushButton(), 4, 1);

	FloatParameterUI* fontSizePUI = new FloatParameterUI(this, PROPERTY_FIELD(TextLabelOverlay::_fontSize));
	layout->addWidget(fontSizePUI->label(), 5, 0);
	layout->addLayout(fontSizePUI->createFieldLayout(), 5, 1);

	FontParameterUI* labelFontPUI = new FontParameterUI(this, PROPERTY_FIELD(TextLabelOverlay::_font));
	layout->addWidget(labelFontPUI->label(), 6, 0);
	layout->addWidget(labelFontPUI->fontPicker(), 6, 1);

	// Text color.
	ColorParameterUI* textColorPUI = new ColorParameterUI(this, PROPERTY_FIELD(TextLabelOverlay::_textColor));
	layout->addWidget(new QLabel(tr("Text color:")), 7, 0);
	layout->addWidget(textColorPUI->colorPicker(), 7, 1);

	QWidget* variablesRollout = createRollout(tr("Variables"), rolloutParams.after(rollout));
    QVBoxLayout* variablesLayout = new QVBoxLayout(variablesRollout);
    variablesLayout->setContentsMargins(4,4,4,4);
    _attributeNamesList = new QLabel();
    _attributeNamesList->setWordWrap(true);
    _attributeNamesList->setTextInteractionFlags(Qt::TextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard | Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard));
	variablesLayout->addWidget(_attributeNamesList);

	// Update input variables list if another modifier has been loaded into the editor.
	connect(this, &TextLabelOverlayEditor::contentsReplaced, this, &TextLabelOverlayEditor::updateEditorFields);
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool TextLabelOverlayEditor::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	if(source == editObject() && event->type() == ReferenceEvent::TargetChanged) {
		updateEditorFields();
	}
	return PropertiesEditor::referenceEvent(source, event);
}

/******************************************************************************
* Updates the enabled/disabled status of the editor's controls.
******************************************************************************/
void TextLabelOverlayEditor::updateEditorFields()
{
	TextLabelOverlay* overlay = static_object_cast<TextLabelOverlay>(editObject());
	if(!overlay) return;

	QString str;
	QStringList variableNames;
	if(ObjectNode* node = overlay->sourceNode()) {
		const PipelineFlowState& flowState = node->evalPipeline(node->dataset()->animationSettings()->time());
		str.append(tr("<p>Dynamic variables that can be referenced in the label text:</b><ul>"));
		for(const QString& attrName : flowState.attributes().keys()) {
			str.append(QStringLiteral("<li>[%1]</li>").arg(attrName));
			variableNames.push_back(QStringLiteral("[") + attrName + QStringLiteral("]"));
		}
		str.append(QStringLiteral("</ul></p><p></p>"));
	}

	_attributeNamesList->setText(str);
	_textEdit->setWordList(variableNames);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
