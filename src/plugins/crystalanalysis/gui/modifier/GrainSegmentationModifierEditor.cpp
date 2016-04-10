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

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/crystalanalysis/modifier/grains/GrainSegmentationModifier.h>
#include <plugins/particles/gui/modifier/analysis/StructureListParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/IntegerParameterUI.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/properties/VariantComboBoxParameterUI.h>
#include <gui/properties/SubObjectParameterUI.h>
#include <gui/properties/BooleanGroupBoxParameterUI.h>
#include "GrainSegmentationModifierEditor.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_OBJECT(CrystalAnalysisGui, GrainSegmentationModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(GrainSegmentationModifier, GrainSegmentationModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void GrainSegmentationModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create the rollout.
	QWidget* rollout = createRollout(tr("Grain segmentation"), rolloutParams);

    QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(6);

	QGroupBox* structureBox = new QGroupBox(tr("Input crystal"));
	layout->addWidget(structureBox);
	QGridLayout* sublayout1 = new QGridLayout(structureBox);
	sublayout1->setContentsMargins(4,4,4,4);
	sublayout1->setSpacing(4);
	sublayout1->setColumnStretch(1,1);
	VariantComboBoxParameterUI* crystalStructureUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(GrainSegmentationModifier::_inputCrystalStructure));

	crystalStructureUI->comboBox()->addItem(tr("Face-centered cubic (FCC)"), QVariant::fromValue((int)StructureAnalysis::LATTICE_FCC));
	crystalStructureUI->comboBox()->addItem(tr("Hexagonal close-packed (HCP)"), QVariant::fromValue((int)StructureAnalysis::LATTICE_HCP));
	crystalStructureUI->comboBox()->addItem(tr("Body-centered cubic (BCC)"), QVariant::fromValue((int)StructureAnalysis::LATTICE_BCC));
	crystalStructureUI->comboBox()->addItem(tr("Diamond cubic / Zinc blende"), QVariant::fromValue((int)StructureAnalysis::LATTICE_CUBIC_DIAMOND));
	crystalStructureUI->comboBox()->addItem(tr("Diamond hexagonal / Wurtzite"), QVariant::fromValue((int)StructureAnalysis::LATTICE_HEX_DIAMOND));
	sublayout1->addWidget(crystalStructureUI->comboBox(), 0, 0, 1, 2);

	BooleanParameterUI* onlySelectedUI = new BooleanParameterUI(this, PROPERTY_FIELD(GrainSegmentationModifier::_onlySelectedParticles));
	sublayout1->addWidget(onlySelectedUI->checkBox(), 1, 0, 1, 2);

	QGroupBox* paramsBox = new QGroupBox(tr("Parameters"));
	layout->addWidget(paramsBox);
	QGridLayout* sublayout2 = new QGridLayout(paramsBox);
	sublayout2->setContentsMargins(4,4,4,4);
	sublayout2->setSpacing(4);
	sublayout2->setColumnStretch(1, 1);

	FloatParameterUI* misorientationThresholdUI = new FloatParameterUI(this, PROPERTY_FIELD(GrainSegmentationModifier::_misorientationThreshold));
	sublayout2->addWidget(misorientationThresholdUI->label(), 0, 0);
	sublayout2->addLayout(misorientationThresholdUI->createFieldLayout(), 0, 1);
	misorientationThresholdUI->setMinValue(0);

	FloatParameterUI* fluctuationToleranceUI = new FloatParameterUI(this, PROPERTY_FIELD(GrainSegmentationModifier::_fluctuationTolerance));
	sublayout2->addWidget(fluctuationToleranceUI->label(), 1, 0);
	sublayout2->addLayout(fluctuationToleranceUI->createFieldLayout(), 1, 1);
	fluctuationToleranceUI->setMinValue(0);

	IntegerParameterUI* minGrainAtomCountUI = new IntegerParameterUI(this, PROPERTY_FIELD(GrainSegmentationModifier::_minGrainAtomCount));
	sublayout2->addWidget(minGrainAtomCountUI->label(), 2, 0);
	sublayout2->addLayout(minGrainAtomCountUI->createFieldLayout(), 2, 1);
	minGrainAtomCountUI->setMinValue(0);

	BooleanGroupBoxParameterUI* generateMeshUI = new BooleanGroupBoxParameterUI(this, PROPERTY_FIELD(GrainSegmentationModifier::_outputPartitionMesh));
	generateMeshUI->groupBox()->setTitle(tr("Generate boundary mesh"));
	sublayout2 = new QGridLayout(generateMeshUI->childContainer());
	sublayout2->setContentsMargins(4,4,4,4);
	sublayout2->setColumnStretch(1, 1);
	layout->addWidget(generateMeshUI->groupBox());

	FloatParameterUI* radiusUI = new FloatParameterUI(this, PROPERTY_FIELD(GrainSegmentationModifier::_probeSphereRadius));
	sublayout2->addWidget(radiusUI->label(), 0, 0);
	sublayout2->addLayout(radiusUI->createFieldLayout(), 0, 1);
	radiusUI->setMinValue(0);

	IntegerParameterUI* smoothingLevelUI = new IntegerParameterUI(this, PROPERTY_FIELD(GrainSegmentationModifier::_smoothingLevel));
	sublayout2->addWidget(smoothingLevelUI->label(), 1, 0);
	sublayout2->addLayout(smoothingLevelUI->createFieldLayout(), 1, 1);
	smoothingLevelUI->setMinValue(0);

	// Status label.
	layout->addWidget(statusLabel());

	// Structure list.
	StructureListParameterUI* structureTypesPUI = new StructureListParameterUI(this);
	layout->addSpacing(10);
	layout->addWidget(structureTypesPUI->tableWidget());

	// Open a sub-editor for the mesh display object.
	new SubObjectParameterUI(this, PROPERTY_FIELD(GrainSegmentationModifier::_meshDisplay), rolloutParams.after(rollout));
}

}	// End of namespace
}	// End of namespace
}	// End of namespace

