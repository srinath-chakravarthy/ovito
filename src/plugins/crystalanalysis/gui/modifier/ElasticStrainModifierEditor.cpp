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
#include <plugins/crystalanalysis/modifier/elasticstrain/ElasticStrainModifier.h>
#include <plugins/particles/gui/modifier/analysis/StructureListParameterUI.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/properties/BooleanRadioButtonParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/VariantComboBoxParameterUI.h>
#include <gui/properties/SubObjectParameterUI.h>
#include "ElasticStrainModifierEditor.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_OBJECT(CrystalAnalysisGui, ElasticStrainModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(ElasticStrainModifier, ElasticStrainModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void ElasticStrainModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create the rollout.
	QWidget* rollout = createRollout(tr("Elastic strain calculation"), rolloutParams, "particles.modifiers.elastic_strain.html");

    QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(6);

	QGroupBox* structureBox = new QGroupBox(tr("Input crystal"));
	layout->addWidget(structureBox);
	QGridLayout* sublayout1 = new QGridLayout(structureBox);
	sublayout1->setContentsMargins(4,4,4,4);
	sublayout1->setSpacing(4);
	sublayout1->setColumnStretch(1,1);
	VariantComboBoxParameterUI* crystalStructureUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(ElasticStrainModifier::_inputCrystalStructure));

	crystalStructureUI->comboBox()->addItem(tr("Face-centered cubic (FCC)"), QVariant::fromValue((int)StructureAnalysis::LATTICE_FCC));
	crystalStructureUI->comboBox()->addItem(tr("Hexagonal close-packed (HCP)"), QVariant::fromValue((int)StructureAnalysis::LATTICE_HCP));
	crystalStructureUI->comboBox()->addItem(tr("Body-centered cubic (BCC)"), QVariant::fromValue((int)StructureAnalysis::LATTICE_BCC));
	crystalStructureUI->comboBox()->addItem(tr("Diamond cubic / Zinc blende"), QVariant::fromValue((int)StructureAnalysis::LATTICE_CUBIC_DIAMOND));
	crystalStructureUI->comboBox()->addItem(tr("Diamond hexagonal / Wurtzite"), QVariant::fromValue((int)StructureAnalysis::LATTICE_HEX_DIAMOND));
	sublayout1->addWidget(crystalStructureUI->comboBox(), 0, 0, 1, 2);

	FloatParameterUI* latticeConstantUI = new FloatParameterUI(this, PROPERTY_FIELD(ElasticStrainModifier::_latticeConstant));
	sublayout1->addWidget(latticeConstantUI->label(), 1, 0);
	sublayout1->addLayout(latticeConstantUI->createFieldLayout(), 1, 1);

	_caRatioUI = new FloatParameterUI(this, PROPERTY_FIELD(ElasticStrainModifier::_caRatio));
	sublayout1->addWidget(_caRatioUI->label(), 2, 0);
	sublayout1->addLayout(_caRatioUI->createFieldLayout(), 2, 1);

	QGroupBox* outputParamsBox = new QGroupBox(tr("Output settings"));
	layout->addWidget(outputParamsBox);
	QGridLayout* sublayout2 = new QGridLayout(outputParamsBox);
	sublayout2->setContentsMargins(4,4,4,4);
	sublayout2->setSpacing(4);
	sublayout2->setColumnStretch(1, 1);
	sublayout2->setColumnMinimumWidth(0, 12);

	BooleanParameterUI* outputStrainTensorsUI = new BooleanParameterUI(this, PROPERTY_FIELD(ElasticStrainModifier::_calculateStrainTensors));
	sublayout2->addWidget(outputStrainTensorsUI->checkBox(), 0, 0, 1, 2);

	BooleanRadioButtonParameterUI* pushStrainTensorsForwardUI = new BooleanRadioButtonParameterUI(this, PROPERTY_FIELD(ElasticStrainModifier::_pushStrainTensorsForward));
	pushStrainTensorsForwardUI->buttonTrue()->setText(tr("in spatial frame"));
	pushStrainTensorsForwardUI->buttonFalse()->setText(tr("in lattice frame"));
	sublayout2->addWidget(pushStrainTensorsForwardUI->buttonTrue(), 1, 1);
	sublayout2->addWidget(pushStrainTensorsForwardUI->buttonFalse(), 2, 1);

	pushStrainTensorsForwardUI->setEnabled(false);
	connect(outputStrainTensorsUI->checkBox(), &QCheckBox::toggled, pushStrainTensorsForwardUI, &BooleanRadioButtonParameterUI::setEnabled);

	BooleanParameterUI* outputDeformationGradientsUI = new BooleanParameterUI(this, PROPERTY_FIELD(ElasticStrainModifier::_calculateDeformationGradients));
	sublayout2->addWidget(outputDeformationGradientsUI->checkBox(), 3, 0, 1, 2);

	// Status label.
	layout->addWidget(statusLabel());

	// Structure list.
	StructureListParameterUI* structureTypesPUI = new StructureListParameterUI(this);
	layout->addSpacing(10);
	layout->addWidget(structureTypesPUI->tableWidget());

	connect(this, &PropertiesEditor::contentsChanged, this, &ElasticStrainModifierEditor::modifierChanged);
}

/******************************************************************************
* Is called each time the parameters of the modifier have changed.
******************************************************************************/
void ElasticStrainModifierEditor::modifierChanged(RefTarget* editObject)
{
	ElasticStrainModifier* modifier = static_object_cast<ElasticStrainModifier>(editObject);
	_caRatioUI->setEnabled(modifier &&
			(modifier->inputCrystalStructure() == StructureAnalysis::LATTICE_HCP ||
			 modifier->inputCrystalStructure() == StructureAnalysis::LATTICE_HEX_DIAMOND));
}

}	// End of namespace
}	// End of namespace
}	// End of namespace

