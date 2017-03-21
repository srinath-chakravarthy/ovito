///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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
#include <gui/properties/BooleanParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/Vector3ParameterUI.h>
#include "VRSettingsObject.h"
#include "VRSettingsObjectEditor.h"

namespace VRPlugin {

IMPLEMENT_OVITO_OBJECT(VRSettingsObjectEditor, PropertiesEditor);
SET_OVITO_OBJECT_EDITOR(VRSettingsObject, VRSettingsObjectEditor);

/******************************************************************************
* Creates the UI controls for the editor.
******************************************************************************/
void VRSettingsObjectEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create the rollout.
	QWidget* rollout = createRollout(tr("Settings"), rolloutParams);
	QVBoxLayout* mainLayout = new QVBoxLayout(rollout);
	mainLayout->setContentsMargins(4,4,4,4);

	// Model size group box.
	QGroupBox* sizeGroupBox = new QGroupBox(tr("Model size"));
	mainLayout->addWidget(sizeGroupBox);
	QGridLayout* layout = new QGridLayout(sizeGroupBox);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(2);
	layout->setColumnStretch(0, 1);
	int row = 0;

	// Apparent model size.
	layout->addWidget(new QLabel(tr("Apparent size:")), row, 0);
	QLabel* modelSizeLabel = new QLabel();
	layout->addWidget(modelSizeLabel, row++, 1);
	connect(this, &PropertiesEditor::contentsChanged, this, [modelSizeLabel](RefTarget* editObject) {
		if(editObject) {
			Vector3 modelSize = static_object_cast<VRSettingsObject>(editObject)->apparentModelSize();
			modelSizeLabel->setText(tr("%1 x %2 x %3 m").arg(modelSize.x(), 0, 'f', 2).arg(modelSize.y(), 0, 'f', 2).arg(modelSize.z(), 0, 'f', 2));
		}
		else modelSizeLabel->setText(QString());
	});

	// Scale factor
	FloatParameterUI* scaleFactorUI = new FloatParameterUI(this, PROPERTY_FIELD(VRSettingsObject::scaleFactor));
	layout->addWidget(scaleFactorUI->label(), row, 0);
	layout->addLayout(scaleFactorUI->createFieldLayout(), row++, 1);	

	// Model transformation group box.
	QGroupBox* transformationGroupBox = new QGroupBox(tr("Model transformation"));
	mainLayout->addWidget(transformationGroupBox);
	layout = new QGridLayout(transformationGroupBox);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(2);
	layout->setColumnStretch(0, 1);
	row = 0;

	// Rotation
	FloatParameterUI* rotationUI = new FloatParameterUI(this, PROPERTY_FIELD(VRSettingsObject::rotationZ));
	layout->addWidget(rotationUI->label(), row, 0);
	layout->addLayout(rotationUI->createFieldLayout(), row++, 1);

	// Translation
	Vector3ParameterUI* translationXUI = new Vector3ParameterUI(this, PROPERTY_FIELD(VRSettingsObject::translation), 0);
	layout->addWidget(translationXUI->label(), row, 0);
	layout->addLayout(translationXUI->createFieldLayout(), row++, 1);
	Vector3ParameterUI* translationYUI = new Vector3ParameterUI(this, PROPERTY_FIELD(VRSettingsObject::translation), 1);
	layout->addWidget(translationYUI->label(), row, 0);
	layout->addLayout(translationYUI->createFieldLayout(), row++, 1);
	Vector3ParameterUI* translationZUI = new Vector3ParameterUI(this, PROPERTY_FIELD(VRSettingsObject::translation), 2);
	layout->addWidget(translationZUI->label(), row, 0);
	layout->addLayout(translationZUI->createFieldLayout(), row++, 1);

	// Model center
	Vector3ParameterUI* modelCenterXUI = new Vector3ParameterUI(this, PROPERTY_FIELD(VRSettingsObject::modelCenter), 0);
	layout->addWidget(modelCenterXUI->label(), row, 0);
	layout->addLayout(modelCenterXUI->createFieldLayout(), row++, 1);
	Vector3ParameterUI* modelCenterYUI = new Vector3ParameterUI(this, PROPERTY_FIELD(VRSettingsObject::modelCenter), 1);
	layout->addWidget(modelCenterYUI->label(), row, 0);
	layout->addLayout(modelCenterYUI->createFieldLayout(), row++, 1);
	Vector3ParameterUI* modelCenterZUI = new Vector3ParameterUI(this, PROPERTY_FIELD(VRSettingsObject::modelCenter), 2);
	layout->addWidget(modelCenterZUI->label(), row, 0);
	layout->addLayout(modelCenterZUI->createFieldLayout(), row++, 1);

	// Recenter action.
	QPushButton* recenterBtn = new QPushButton(tr("Reset"), rollout);
	connect(recenterBtn, &QPushButton::clicked, this, [this]() {
		VRSettingsObject* settings = static_object_cast<VRSettingsObject>(editObject());
		if(settings) settings->recenter();
	});
	layout->addWidget(recenterBtn, row++, 0, 1, 2);

	// Navigation mode group box.
	QGroupBox* navigationGroupBox = new QGroupBox(tr("Navigation"));
	mainLayout->addWidget(navigationGroupBox);
	layout = new QGridLayout(navigationGroupBox);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(2);
	layout->setColumnStretch(0, 1);
	row = 0;

	// Speed
	FloatParameterUI* speedUI = new FloatParameterUI(this, PROPERTY_FIELD(VRSettingsObject::movementSpeed));
	layout->addWidget(speedUI->label(), row, 0);
	layout->addLayout(speedUI->createFieldLayout(), row++, 1);

	// Flying mode
	BooleanParameterUI* flyingModeUI = new BooleanParameterUI(this, PROPERTY_FIELD(VRSettingsObject::flyingMode));
	layout->addWidget(flyingModeUI->checkBox(), row++, 0, 1, 2);

	// Show floor
	BooleanParameterUI* showFloorUI = new BooleanParameterUI(this, PROPERTY_FIELD(VRSettingsObject::showFloor));
	layout->addWidget(showFloorUI->checkBox(), row++, 0, 1, 2);

	// Performance group box.
	QGroupBox* performanceGroupBox = new QGroupBox(tr("Performance"));
	mainLayout->addWidget(performanceGroupBox);
	layout = new QGridLayout(performanceGroupBox);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(2);
	layout->setColumnStretch(0, 1);
	row = 0;

	// Supersampling	
	BooleanParameterUI* supersamplingEnabledUI = new BooleanParameterUI(this, PROPERTY_FIELD(VRSettingsObject::supersamplingEnabled));
	layout->addWidget(supersamplingEnabledUI->checkBox(), row++, 0, 1, 2);

    // Disable viewport rendering.
    QCheckBox* disableViewportsBox = new QCheckBox(tr("Disable main viewports"));
    connect(disableViewportsBox, &QCheckBox::toggled, this, &VRSettingsObjectEditor::disableViewportRendering);
    layout->addWidget(disableViewportsBox, row++, 0, 1, 2);	
}

}	// End of namespace
