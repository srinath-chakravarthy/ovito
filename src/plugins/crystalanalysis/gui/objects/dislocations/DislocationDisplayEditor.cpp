///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2015) Alexander Stukowski
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

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/crystalanalysis/objects/dislocations/DislocationDisplay.h>
#include <gui/properties/VariantComboBoxParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/ColorParameterUI.h>
#include <gui/properties/BooleanGroupBoxParameterUI.h>
#include <gui/properties/BooleanParameterUI.h>
#include "DislocationDisplayEditor.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_OBJECT(CrystalAnalysisGui, DislocationDisplayEditor, PropertiesEditor);
SET_OVITO_OBJECT_EDITOR(DislocationDisplay, DislocationDisplayEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void DislocationDisplayEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Dislocation display"), rolloutParams);

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QGroupBox* linesGroupBox = new QGroupBox(tr("Dislocation lines"));
	QGridLayout* sublayout = new QGridLayout(linesGroupBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);
	sublayout->setColumnStretch(1, 1);
	layout->addWidget(linesGroupBox);

	// Shading mode.
	VariantComboBoxParameterUI* shadingModeUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(DislocationDisplay::_shadingMode));
	shadingModeUI->comboBox()->addItem(tr("Normal"), qVariantFromValue(ArrowPrimitive::NormalShading));
	shadingModeUI->comboBox()->addItem(tr("Flat"), qVariantFromValue(ArrowPrimitive::FlatShading));
	sublayout->addWidget(new QLabel(tr("Shading mode:")), 0, 0);
	sublayout->addWidget(shadingModeUI->comboBox(), 0, 1);

	// Line width parameter.
	FloatParameterUI* lineWidthUI = new FloatParameterUI(this, PROPERTY_FIELD(DislocationDisplay::_lineWidth));
	sublayout->addWidget(lineWidthUI->label(), 1, 0);
	sublayout->addLayout(lineWidthUI->createFieldLayout(), 1, 1);

	// Show Burgers vectors.
	BooleanGroupBoxParameterUI* showBurgersVectorsGroupUI = new BooleanGroupBoxParameterUI(this, PROPERTY_FIELD(DislocationDisplay::_showBurgersVectors));
	showBurgersVectorsGroupUI->groupBox()->setTitle(tr("Burgers vectors"));
	sublayout = new QGridLayout(showBurgersVectorsGroupUI->childContainer());
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);
	sublayout->setColumnStretch(1, 1);
	layout->addWidget(showBurgersVectorsGroupUI->groupBox());

	// Arrow scaling.
	FloatParameterUI* burgersVectorScalingUI = new FloatParameterUI(this, PROPERTY_FIELD(DislocationDisplay::_burgersVectorScaling));
	sublayout->addWidget(new QLabel(tr("Scaling factor:")), 0, 0);
	sublayout->addLayout(burgersVectorScalingUI->createFieldLayout(), 0, 1);

	// Arrow width.
	FloatParameterUI* burgersVectorWidthUI = new FloatParameterUI(this, PROPERTY_FIELD(DislocationDisplay::_burgersVectorWidth));
	sublayout->addWidget(new QLabel(tr("Width:")), 1, 0);
	sublayout->addLayout(burgersVectorWidthUI->createFieldLayout(), 1, 1);

	// Arrow color.
	ColorParameterUI* burgersVectorColorUI = new ColorParameterUI(this, PROPERTY_FIELD(DislocationDisplay::_burgersVectorColor));
	sublayout->addWidget(new QLabel(tr("Color:")), 2, 0);
	sublayout->addWidget(burgersVectorColorUI->colorPicker(), 2, 1);

	// Show line directions.
	BooleanParameterUI* showLineDirectionsUI = new BooleanParameterUI(this, PROPERTY_FIELD(DislocationDisplay::_showLineDirections));
	layout->addWidget(showLineDirectionsUI->checkBox());

	// Indicate dislocation character.
	sublayout = new QGridLayout();
	sublayout->setContentsMargins(0,0,0,0);
	sublayout->setSpacing(0);
	BooleanParameterUI* indicateDislocationCharacterUI = new BooleanParameterUI(this, PROPERTY_FIELD(DislocationDisplay::_indicateDislocationCharacter));
	sublayout->addWidget(indicateDislocationCharacterUI->checkBox(), 0, 0);
	sublayout->addWidget(new QLabel(tr("<p> (<font color=\"#FF0000\">screw</font>/<font color=\"#00DD00\">edge</font>)</p>")), 0, 1);
	layout->addLayout(sublayout);
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
