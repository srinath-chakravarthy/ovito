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
#include <gui/properties/CustomParameterUI.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/widgets/general/AutocompleteTextEdit.h>
#include <gui/actions/ViewportModeAction.h>
#include <gui/viewport/overlay/MoveOverlayInputMode.h>
#include <core/viewport/overlay/TextLabelOverlay.h>
#include <core/animation/AnimationSettings.h>
#include <core/scene/SceneRoot.h>
#include <core/scene/ObjectNode.h>
#include "TextLabelOverlayEditor.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(View) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(TextLabelOverlayEditor, PropertiesEditor);
SET_OVITO_OBJECT_EDITOR(TextLabelOverlay, TextLabelOverlayEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void TextLabelOverlayEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Text label"), rolloutParams, "viewport_overlays.text_label.html");

    // Create the rollout contents.
	QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1, 3);
	layout->setColumnStretch(2, 1);

	// This widget displays the list of available ObjectNodes in the current scene.
	class ObjectNodeComboBox : public QComboBox {
	public:
		/// Initializes the widget.
		ObjectNodeComboBox(QWidget* parent = nullptr) : QComboBox(parent), _overlay(nullptr) {}

		/// Sets the overlay being edited.
		void setOverlay(TextLabelOverlay* overlay) { _overlay = overlay; }

		/// Is called just before the drop-down box is activated.
		virtual void showPopup() override {
			clear();
			if(_overlay) {
				// Find all ObjectNodes in the scene.
				_overlay->dataset()->sceneRoot()->visitObjectNodes([this](ObjectNode* node) {
					addItem(node->objectTitle(), QVariant::fromValue(node));
					return true;
				});
				setCurrentIndex(findData(QVariant::fromValue(_overlay->sourceNode())));
			}
			if(count() == 0) addItem(tr("<none>"));
			QComboBox::showPopup();
		}

	private:
		TextLabelOverlay* _overlay;
	};

	ObjectNodeComboBox* nodeComboBox = new ObjectNodeComboBox();
	CustomParameterUI* sourcePUI = new CustomParameterUI(this, "sourceNode", nodeComboBox,
			[nodeComboBox](const QVariant& value) {
				nodeComboBox->clear();
				ObjectNode* node = dynamic_object_cast<ObjectNode>(value.value<ObjectNode*>());
				if(node) {
					nodeComboBox->addItem(node->objectTitle(), QVariant::fromValue(node));
				}
				else {
					nodeComboBox->addItem(tr("<none>"));
				}
				nodeComboBox->setCurrentIndex(0);
			},
			[nodeComboBox]() {
				return nodeComboBox->currentData();
			},
			[nodeComboBox](RefTarget* editObject) {
				nodeComboBox->setOverlay(dynamic_object_cast<TextLabelOverlay>(editObject));
			});
	connect(nodeComboBox, (void (QComboBox::*)(int))&QComboBox::activated, sourcePUI, &CustomParameterUI::updatePropertyValue);
	layout->addWidget(new QLabel(tr("Data source:")), 0, 0);
	layout->addWidget(sourcePUI->widget(), 0, 1, 1, 2);

	// Label text.
	StringParameterUI* labelTextPUI = new StringParameterUI(this, PROPERTY_FIELD(TextLabelOverlay::labelText));
	layout->addWidget(new QLabel(tr("Text:")), 1, 0);
	_textEdit = new AutocompleteTextEdit();
	labelTextPUI->setTextBox(_textEdit);
	layout->addWidget(labelTextPUI->textBox(), 1, 1, 1, 2);

	VariantComboBoxParameterUI* alignmentPUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(TextLabelOverlay::alignment));
	layout->addWidget(new QLabel(tr("Position:")), 2, 0);
	layout->addWidget(alignmentPUI->comboBox(), 2, 1);
	alignmentPUI->comboBox()->addItem(tr("Top"), QVariant::fromValue((int)(Qt::AlignTop | Qt::AlignHCenter)));
	alignmentPUI->comboBox()->addItem(tr("Top left"), QVariant::fromValue((int)(Qt::AlignTop | Qt::AlignLeft)));
	alignmentPUI->comboBox()->addItem(tr("Top right"), QVariant::fromValue((int)(Qt::AlignTop | Qt::AlignRight)));
	alignmentPUI->comboBox()->addItem(tr("Bottom"), QVariant::fromValue((int)(Qt::AlignBottom | Qt::AlignHCenter)));
	alignmentPUI->comboBox()->addItem(tr("Bottom left"), QVariant::fromValue((int)(Qt::AlignBottom | Qt::AlignLeft)));
	alignmentPUI->comboBox()->addItem(tr("Bottom right"), QVariant::fromValue((int)(Qt::AlignBottom | Qt::AlignRight)));
	alignmentPUI->comboBox()->addItem(tr("Left"), QVariant::fromValue((int)(Qt::AlignVCenter | Qt::AlignLeft)));
	alignmentPUI->comboBox()->addItem(tr("Right"), QVariant::fromValue((int)(Qt::AlignVCenter | Qt::AlignRight)));

	FloatParameterUI* offsetXPUI = new FloatParameterUI(this, PROPERTY_FIELD(TextLabelOverlay::offsetX));
	layout->addWidget(offsetXPUI->label(), 3, 0);
	layout->addLayout(offsetXPUI->createFieldLayout(), 3, 1, 1, 2);

	FloatParameterUI* offsetYPUI = new FloatParameterUI(this, PROPERTY_FIELD(TextLabelOverlay::offsetY));
	layout->addWidget(offsetYPUI->label(), 4, 0);
	layout->addLayout(offsetYPUI->createFieldLayout(), 4, 1, 1, 2);

	ViewportInputMode* moveOverlayMode = new MoveOverlayInputMode(this);
	ViewportModeAction* moveOverlayAction = new ViewportModeAction(mainWindow(), tr("Move using mouse"), this, moveOverlayMode);
	layout->addWidget(moveOverlayAction->createPushButton(), 5, 1, 1, 2);

	FloatParameterUI* fontSizePUI = new FloatParameterUI(this, PROPERTY_FIELD(TextLabelOverlay::fontSize));
	layout->addWidget(new QLabel(tr("Text size/color:")), 6, 0);
	layout->addLayout(fontSizePUI->createFieldLayout(), 6, 1);

	// Text color.
	ColorParameterUI* textColorPUI = new ColorParameterUI(this, PROPERTY_FIELD(TextLabelOverlay::textColor));
	layout->addWidget(textColorPUI->colorPicker(), 6, 2);

	BooleanParameterUI* outlineEnabledPUI = new BooleanParameterUI(this, PROPERTY_FIELD(TextLabelOverlay::outlineEnabled));
	layout->addWidget(outlineEnabledPUI->checkBox(), 7, 1);

	ColorParameterUI* outlineColorPUI = new ColorParameterUI(this, PROPERTY_FIELD(TextLabelOverlay::outlineColor));
	layout->addWidget(outlineColorPUI->colorPicker(), 7, 2);

	FontParameterUI* labelFontPUI = new FontParameterUI(this, PROPERTY_FIELD(TextLabelOverlay::font));
	layout->addWidget(labelFontPUI->label(), 8, 0);
	layout->addWidget(labelFontPUI->fontPicker(), 8, 1, 1, 2);

	QWidget* variablesRollout = createRollout(tr("Variables"), rolloutParams.after(rollout), "viewport_overlays.text_label.html");
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
		const PipelineFlowState& flowState = node->evaluatePipelineImmediately(PipelineEvalRequest(node->dataset()->animationSettings()->time(), false));
		str.append(tr("<p>Dynamic variables that can be referenced in the label text:</b><ul>"));
		for(const QString& attrName : flowState.attributes().keys()) {
			str.append(QStringLiteral("<li>[%1]</li>").arg(attrName.toHtmlEscaped()));
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
