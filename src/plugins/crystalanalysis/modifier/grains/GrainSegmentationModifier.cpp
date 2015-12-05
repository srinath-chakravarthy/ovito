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
#include <core/gui/properties/FloatParameterUI.h>
#include <core/gui/properties/IntegerParameterUI.h>
#include <core/gui/properties/VariantComboBoxParameterUI.h>
#include <core/gui/properties/SubObjectParameterUI.h>
#include <plugins/particles/objects/SimulationCellObject.h>
#include <plugins/crystalanalysis/objects/clusters/ClusterGraphObject.h>
#include <plugins/crystalanalysis/objects/patterns/StructurePattern.h>
#include "GrainSegmentationModifier.h"
#include "GrainSegmentationEngine.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(CrystalAnalysis, GrainSegmentationModifier, StructureIdentificationModifier);
IMPLEMENT_OVITO_OBJECT(CrystalAnalysis, GrainSegmentationModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(GrainSegmentationModifier, GrainSegmentationModifierEditor);
DEFINE_FLAGS_PROPERTY_FIELD(GrainSegmentationModifier, _inputCrystalStructure, "CrystalStructure", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(GrainSegmentationModifier, _misorientationThreshold, "MisorientationThreshold", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(GrainSegmentationModifier, _fluctuationTolerance, "FluctuationTolerance", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(GrainSegmentationModifier, _minGrainAtomCount, "MinGrainAtomCount", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_REFERENCE_FIELD(GrainSegmentationModifier, _patternCatalog, "PatternCatalog", PatternCatalog, PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);
SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier, _inputCrystalStructure, "Input crystal structure");
SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier, _misorientationThreshold, "Misorientation threshold");
SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier, _fluctuationTolerance, "Tolerance");
SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier, _minGrainAtomCount, "Minimum grain size");
SET_PROPERTY_FIELD_UNITS(GrainSegmentationModifier, _misorientationThreshold, AngleParameterUnit);
SET_PROPERTY_FIELD_UNITS(GrainSegmentationModifier, _fluctuationTolerance, AngleParameterUnit);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
GrainSegmentationModifier::GrainSegmentationModifier(DataSet* dataset) : StructureIdentificationModifier(dataset),
		_inputCrystalStructure(StructureAnalysis::LATTICE_FCC),
		_misorientationThreshold(3.0 * FLOATTYPE_PI / 180.0),
		_fluctuationTolerance(2.0 * FLOATTYPE_PI / 180.0),
		_minGrainAtomCount(10)
{
	INIT_PROPERTY_FIELD(GrainSegmentationModifier::_inputCrystalStructure);
	INIT_PROPERTY_FIELD(GrainSegmentationModifier::_misorientationThreshold);
	INIT_PROPERTY_FIELD(GrainSegmentationModifier::_fluctuationTolerance);
	INIT_PROPERTY_FIELD(GrainSegmentationModifier::_minGrainAtomCount);
	INIT_PROPERTY_FIELD(GrainSegmentationModifier::_patternCatalog);

	// Create pattern catalog.
	_patternCatalog = new PatternCatalog(dataset);

	// Create the structure types.
	ParticleTypeProperty::PredefinedStructureType predefTypes[] = {
			ParticleTypeProperty::PredefinedStructureType::OTHER,
			ParticleTypeProperty::PredefinedStructureType::FCC,
			ParticleTypeProperty::PredefinedStructureType::HCP,
			ParticleTypeProperty::PredefinedStructureType::BCC,
			ParticleTypeProperty::PredefinedStructureType::CUBIC_DIAMOND,
			ParticleTypeProperty::PredefinedStructureType::HEX_DIAMOND
	};
	OVITO_STATIC_ASSERT(sizeof(predefTypes)/sizeof(predefTypes[0]) == StructureAnalysis::NUM_LATTICE_TYPES);
	for(int id = 0; id < StructureAnalysis::NUM_LATTICE_TYPES; id++) {
		OORef<StructurePattern> stype(new StructurePattern(dataset));
		stype->setId(id);
		stype->setName(ParticleTypeProperty::getPredefinedStructureTypeName(predefTypes[id]));
		stype->setColor(ParticleTypeProperty::getDefaultParticleColor(ParticleProperty::StructureTypeProperty, stype->name(), id));
		stype->setStructureType(StructurePattern::Lattice);
		addStructureType(stype);
		_patternCatalog->addPattern(stype);
	}
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void GrainSegmentationModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	StructureIdentificationModifier::propertyChanged(field);

	// Recompute results when the parameters have changed.
	if(field == PROPERTY_FIELD(GrainSegmentationModifier::_inputCrystalStructure) ||
			field == PROPERTY_FIELD(GrainSegmentationModifier::_misorientationThreshold) ||
			field == PROPERTY_FIELD(GrainSegmentationModifier::_fluctuationTolerance) ||
			field == PROPERTY_FIELD(GrainSegmentationModifier::_minGrainAtomCount))
		invalidateCachedResults();
}

/******************************************************************************
* Resets the modifier's result cache.
******************************************************************************/
void GrainSegmentationModifier::invalidateCachedResults()
{
	StructureIdentificationModifier::invalidateCachedResults();
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
std::shared_ptr<AsynchronousParticleModifier::ComputeEngine> GrainSegmentationModifier::createEngine(TimePoint time, TimeInterval validityInterval)
{
	// Get modifier inputs.
	ParticlePropertyObject* posProperty = expectStandardProperty(ParticleProperty::PositionProperty);
	SimulationCellObject* simCell = expectSimulationCell();

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<GrainSegmentationEngine>(validityInterval, posProperty->storage(),
			simCell->data(), inputCrystalStructure());
}

/******************************************************************************
* Unpacks the results of the computation engine and stores them in the modifier.
******************************************************************************/
void GrainSegmentationModifier::transferComputationResults(ComputeEngine* engine)
{
	StructureIdentificationModifier::transferComputationResults(engine);

	GrainSegmentationEngine* eng = static_cast<GrainSegmentationEngine*>(engine);
	_atomClusters = eng->atomClusters();
	_clusterGraph = eng->clusterGraph();
}

/******************************************************************************
* Lets the modifier insert the cached computation results into the
* modification pipeline.
******************************************************************************/
PipelineStatus GrainSegmentationModifier::applyComputationResults(TimePoint time, TimeInterval& validityInterval)
{
	StructureIdentificationModifier::applyComputationResults(time, validityInterval);

	if(!_atomClusters)
		throw Exception(tr("No computation results available."));

	if(outputParticleCount() != _atomClusters->size())
		throw Exception(tr("The number of input particles has changed. The stored results have become invalid."));

	if(_clusterGraph) {
		// Output cluster graph.
		OORef<ClusterGraphObject> clusterGraphObj(new ClusterGraphObject(dataset(), _clusterGraph.data()));
		output().addObject(clusterGraphObj);
	}

	// Output pattern catalog.
	if(_patternCatalog) {
		output().addObject(_patternCatalog);
	}

	// Output particle properties.
	outputStandardProperty(_atomClusters.data());

	return PipelineStatus::Success;
}

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

	// Status label.
	layout->addWidget(statusLabel());

	// Structure list.
	StructureListParameterUI* structureTypesPUI = new StructureListParameterUI(this);
	layout->addSpacing(10);
	layout->addWidget(structureTypesPUI->tableWidget());
}

}	// End of namespace
}	// End of namespace
}	// End of namespace

