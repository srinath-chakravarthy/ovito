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
#include <plugins/particles/modifier/coloring/ColorLegendOverlay.h>
#include <gui/properties/BooleanGroupBoxParameterUI.h>
#include <gui/properties/StringParameterUI.h>
#include <gui/properties/ColorParameterUI.h>
#include <gui/properties/FontParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/Vector3ParameterUI.h>
#include <gui/properties/VariantComboBoxParameterUI.h>
#include <gui/properties/CustomParameterUI.h>
#include <gui/viewport/input/ViewportInputManager.h>
#include <gui/viewport/overlay/MoveOverlayInputMode.h>
#include <gui/actions/ViewportModeAction.h>
#include <core/scene/SceneRoot.h>
#include <core/scene/pipeline/PipelineObject.h>
#include <core/scene/pipeline/ModifierApplication.h>
#include "ColorLegendOverlayEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Coloring) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(ParticlesGui, ColorLegendOverlayEditor, PropertiesEditor);
SET_OVITO_OBJECT_EDITOR(ColorLegendOverlay, ColorLegendOverlayEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void ColorLegendOverlayEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Color legend"), rolloutParams);

    // Create the rollout contents.
	QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1, 1);
	int row = 0;

	// This widget displays the list of available ColorCodingModifiers in the current scene.
	class ModifierComboBox : public QComboBox {
	public:
		/// Initializes the widget.
		ModifierComboBox(QWidget* parent = nullptr) : QComboBox(parent), _overlay(nullptr) {}

		/// Sets the overlay being edited.
		void setOverlay(ColorLegendOverlay* overlay) { _overlay = overlay; }

		/// Is called just before the drop-down box is activated.
		virtual void showPopup() override {
			clear();
			if(_overlay) {
				// Find all ColorCodingModifiers in the scene. For this we have to visit all
				// object nodes and iterate over their modification pipelines.
				_overlay->dataset()->sceneRoot()->visitObjectNodes([this](ObjectNode* node) {
					DataObject* obj = node->dataProvider();
					while(obj) {
						if(PipelineObject* pipeline = dynamic_object_cast<PipelineObject>(obj)) {
							for(ModifierApplication* modApp : pipeline->modifierApplications()) {
								if(ColorCodingModifier* mod = dynamic_object_cast<ColorCodingModifier>(modApp->modifier())) {
									addItem(mod->sourceProperty().nameWithComponent(), QVariant::fromValue(mod));
								}
							}
							obj = pipeline->sourceObject();
						}
						else break;
					}
					return true;
				});
				setCurrentIndex(findData(QVariant::fromValue(_overlay->modifier())));
			}
			if(count() == 0) addItem(tr("<none>"));
			QComboBox::showPopup();
		}

	private:
		ColorLegendOverlay* _overlay;
	};

	ModifierComboBox* modifierComboBox = new ModifierComboBox();
	CustomParameterUI* modifierPUI = new CustomParameterUI(this, "modifier", modifierComboBox,
			[modifierComboBox](const QVariant& value) {
				modifierComboBox->clear();
				ColorCodingModifier* mod = dynamic_object_cast<ColorCodingModifier>(value.value<ColorCodingModifier*>());
				if(mod)
					modifierComboBox->addItem(mod->sourceProperty().nameWithComponent(), QVariant::fromValue(mod));
				else
					modifierComboBox->addItem(tr("<none>"));
				modifierComboBox->setCurrentIndex(0);
			},
			[modifierComboBox]() {
				return modifierComboBox->currentData();
			},
			[modifierComboBox](RefTarget* editObject) {
				modifierComboBox->setOverlay(dynamic_object_cast<ColorLegendOverlay>(editObject));
			});
	connect(modifierComboBox, (void (QComboBox::*)(int))&QComboBox::activated, modifierPUI, &CustomParameterUI::updatePropertyValue);
	layout->addWidget(new QLabel(tr("Source modifier:")), row, 0);
	layout->addWidget(modifierPUI->widget(), row++, 1);

	QGroupBox* positionBox = new QGroupBox(tr("Position"));
	layout->addWidget(positionBox, row++, 0, 1, 2);
	QGridLayout* sublayout = new QGridLayout(positionBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);
	sublayout->setColumnStretch(1, 1);

	VariantComboBoxParameterUI* alignmentPUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::_alignment));
	sublayout->addWidget(alignmentPUI->comboBox(), 0, 0);
	alignmentPUI->comboBox()->addItem(tr("Top"), QVariant::fromValue((int)(Qt::AlignTop | Qt::AlignHCenter)));
	alignmentPUI->comboBox()->addItem(tr("Top left"), QVariant::fromValue((int)(Qt::AlignTop | Qt::AlignLeft)));
	alignmentPUI->comboBox()->addItem(tr("Top right"), QVariant::fromValue((int)(Qt::AlignTop | Qt::AlignRight)));
	alignmentPUI->comboBox()->addItem(tr("Bottom"), QVariant::fromValue((int)(Qt::AlignBottom | Qt::AlignHCenter)));
	alignmentPUI->comboBox()->addItem(tr("Bottom left"), QVariant::fromValue((int)(Qt::AlignBottom | Qt::AlignLeft)));
	alignmentPUI->comboBox()->addItem(tr("Bottom right"), QVariant::fromValue((int)(Qt::AlignBottom | Qt::AlignRight)));
	alignmentPUI->comboBox()->addItem(tr("Left"), QVariant::fromValue((int)(Qt::AlignVCenter | Qt::AlignLeft)));
	alignmentPUI->comboBox()->addItem(tr("Right"), QVariant::fromValue((int)(Qt::AlignVCenter | Qt::AlignRight)));

	VariantComboBoxParameterUI* orientationPUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::_orientation));
	sublayout->addWidget(orientationPUI->comboBox(), 0, 1);
	orientationPUI->comboBox()->addItem(tr("Vertical"), QVariant::fromValue((int)Qt::Vertical));
	orientationPUI->comboBox()->addItem(tr("Horizontal"), QVariant::fromValue((int)Qt::Horizontal));

	FloatParameterUI* offsetXPUI = new FloatParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::_offsetX));
	sublayout->addWidget(offsetXPUI->label(), 1, 0);
	sublayout->addLayout(offsetXPUI->createFieldLayout(), 1, 1);

	FloatParameterUI* offsetYPUI = new FloatParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::_offsetY));
	sublayout->addWidget(offsetYPUI->label(), 2, 0);
	sublayout->addLayout(offsetYPUI->createFieldLayout(), 2, 1);

	ViewportInputMode* moveOverlayMode = new MoveOverlayInputMode(this);
	ViewportModeAction* moveOverlayAction = new ViewportModeAction(mainWindow(), tr("Move using mouse"), this, moveOverlayMode);
	sublayout->addWidget(moveOverlayAction->createPushButton(), 3, 0, 1, 2);

	QGroupBox* sizeBox = new QGroupBox(tr("Size"));
	layout->addWidget(sizeBox, row++, 0, 1, 2);
	sublayout = new QGridLayout(sizeBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);
	sublayout->setColumnStretch(1, 1);

	FloatParameterUI* sizePUI = new FloatParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::_legendSize));
	sublayout->addWidget(sizePUI->label(), 0, 0);
	sublayout->addLayout(sizePUI->createFieldLayout(), 0, 1);
	sizePUI->setMinValue(0);

	FloatParameterUI* aspectRatioPUI = new FloatParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::_aspectRatio));
	sublayout->addWidget(aspectRatioPUI->label(), 1, 0);
	sublayout->addLayout(aspectRatioPUI->createFieldLayout(), 1, 1);
	aspectRatioPUI->setMinValue(1.0);

	QGroupBox* labelBox = new QGroupBox(tr("Labels"));
	layout->addWidget(labelBox, row++, 0, 1, 2);
	sublayout = new QGridLayout(labelBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);
	sublayout->setColumnStretch(1, 3);
	sublayout->setColumnStretch(2, 1);

	StringParameterUI* titlePUI = new StringParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::_title));
	sublayout->addWidget(new QLabel(tr("Custom title:")), 0, 0);
	sublayout->addWidget(titlePUI->textBox(), 0, 1, 1, 2);

	StringParameterUI* label1PUI = new StringParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::_label1));
	sublayout->addWidget(new QLabel(tr("Custom label 1:")), 1, 0);
	sublayout->addWidget(label1PUI->textBox(), 1, 1, 1, 2);

	StringParameterUI* label2PUI = new StringParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::_label2));
	sublayout->addWidget(new QLabel(tr("Custom label 2:")), 2, 0);
	sublayout->addWidget(label2PUI->textBox(), 2, 1, 1, 2);

	StringParameterUI* valueFormatStringPUI = new StringParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::_valueFormatString));
	sublayout->addWidget(new QLabel(tr("Format string:")), 3, 0);
	sublayout->addWidget(valueFormatStringPUI->textBox(), 3, 1, 1, 2);

	FloatParameterUI* fontSizePUI = new FloatParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::_fontSize));
	sublayout->addWidget(new QLabel(tr("Text size/color:")), 4, 0);
	sublayout->addLayout(fontSizePUI->createFieldLayout(), 4, 1);
	fontSizePUI->setMinValue(0);

	ColorParameterUI* textColorPUI = new ColorParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::_textColor));
	sublayout->addWidget(textColorPUI->colorPicker(), 4, 2);

	FontParameterUI* labelFontPUI = new FontParameterUI(this, PROPERTY_FIELD(ColorLegendOverlay::_font));
	sublayout->addWidget(labelFontPUI->label(), 5, 0);
	sublayout->addWidget(labelFontPUI->fontPicker(), 5, 1, 1, 2);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
