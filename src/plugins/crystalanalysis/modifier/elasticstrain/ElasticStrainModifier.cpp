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
#include <plugins/particles/objects/SimulationCellObject.h>
#include <plugins/crystalanalysis/objects/clusters/ClusterGraphObject.h>
#include <plugins/crystalanalysis/objects/patterns/StructurePattern.h>
#include "ElasticStrainModifier.h"
#include "ElasticStrainEngine.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(CrystalAnalysis, ElasticStrainModifier, StructureIdentificationModifier);
DEFINE_FLAGS_PROPERTY_FIELD(ElasticStrainModifier, _inputCrystalStructure, "CrystalStructure", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(ElasticStrainModifier, _calculateDeformationGradients, "CalculateDeformationGradients", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(ElasticStrainModifier, _calculateStrainTensors, "CalculateStrainTensors", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(ElasticStrainModifier, _latticeConstant, "LatticeConstant", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(ElasticStrainModifier, _caRatio, "CtoARatio", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(ElasticStrainModifier, _pushStrainTensorsForward, "PushStrainTensorsForward", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_REFERENCE_FIELD(ElasticStrainModifier, _patternCatalog, "PatternCatalog", PatternCatalog, PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);
SET_PROPERTY_FIELD_LABEL(ElasticStrainModifier, _inputCrystalStructure, "Input crystal structure");
SET_PROPERTY_FIELD_LABEL(ElasticStrainModifier, _calculateDeformationGradients, "Output deformation gradient tensors");
SET_PROPERTY_FIELD_LABEL(ElasticStrainModifier, _calculateStrainTensors, "Output strain tensors");
SET_PROPERTY_FIELD_LABEL(ElasticStrainModifier, _latticeConstant, "Lattice constant");
SET_PROPERTY_FIELD_LABEL(ElasticStrainModifier, _caRatio, "c/a ratio");
SET_PROPERTY_FIELD_LABEL(ElasticStrainModifier, _pushStrainTensorsForward, "Strain tensor in spatial frame (push-forward)");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ElasticStrainModifier, _latticeConstant, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ElasticStrainModifier, _caRatio, FloatParameterUnit, 0);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
ElasticStrainModifier::ElasticStrainModifier(DataSet* dataset) : StructureIdentificationModifier(dataset),
		_inputCrystalStructure(StructureAnalysis::LATTICE_FCC), _calculateDeformationGradients(false), _calculateStrainTensors(true),
		_latticeConstant(1), _caRatio(sqrt(8.0/3.0)), _pushStrainTensorsForward(true)
{
	INIT_PROPERTY_FIELD(ElasticStrainModifier::_inputCrystalStructure);
	INIT_PROPERTY_FIELD(ElasticStrainModifier::_patternCatalog);
	INIT_PROPERTY_FIELD(ElasticStrainModifier::_calculateDeformationGradients);
	INIT_PROPERTY_FIELD(ElasticStrainModifier::_calculateStrainTensors);
	INIT_PROPERTY_FIELD(ElasticStrainModifier::_latticeConstant);
	INIT_PROPERTY_FIELD(ElasticStrainModifier::_caRatio);
	INIT_PROPERTY_FIELD(ElasticStrainModifier::_pushStrainTensorsForward);

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
		OORef<StructurePattern> stype = _patternCatalog->structureById(id);
		if(!stype) {
			stype = new StructurePattern(dataset);
			stype->setId(id);
			stype->setStructureType(StructurePattern::Lattice);
			_patternCatalog->addPattern(stype);
		}
		stype->setName(ParticleTypeProperty::getPredefinedStructureTypeName(predefTypes[id]));
		stype->setColor(ParticleTypeProperty::getDefaultParticleColor(ParticleProperty::StructureTypeProperty, stype->name(), id));
		addStructureType(stype);
	}
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void ElasticStrainModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	StructureIdentificationModifier::propertyChanged(field);

	// Recompute results when the parameters have changed.
	if(field == PROPERTY_FIELD(ElasticStrainModifier::_inputCrystalStructure) ||
			field == PROPERTY_FIELD(ElasticStrainModifier::_calculateDeformationGradients) ||
			field == PROPERTY_FIELD(ElasticStrainModifier::_calculateStrainTensors) ||
			field == PROPERTY_FIELD(ElasticStrainModifier::_latticeConstant) ||
			field == PROPERTY_FIELD(ElasticStrainModifier::_caRatio) ||
			field == PROPERTY_FIELD(ElasticStrainModifier::_pushStrainTensorsForward))
		invalidateCachedResults();
}

/******************************************************************************
* Resets the modifier's result cache.
******************************************************************************/
void ElasticStrainModifier::invalidateCachedResults()
{
	StructureIdentificationModifier::invalidateCachedResults();
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
std::shared_ptr<AsynchronousParticleModifier::ComputeEngine> ElasticStrainModifier::createEngine(TimePoint time, TimeInterval validityInterval)
{
	// Get modifier inputs.
	ParticlePropertyObject* posProperty = expectStandardProperty(ParticleProperty::PositionProperty);
	SimulationCellObject* simCell = expectSimulationCell();

	// Build list of preferred crystal orientations.
	std::vector<Matrix3> preferredCrystalOrientations;
	if(inputCrystalStructure() == StructureAnalysis::LATTICE_FCC || inputCrystalStructure() == StructureAnalysis::LATTICE_BCC || inputCrystalStructure() == StructureAnalysis::LATTICE_CUBIC_DIAMOND) {
		preferredCrystalOrientations.push_back(Matrix3::Identity());
	}

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<ElasticStrainEngine>(validityInterval, posProperty->storage(),
			simCell->data(), inputCrystalStructure(), std::move(preferredCrystalOrientations),
			calculateDeformationGradients(), calculateStrainTensors(),
			latticeConstant(), axialRatio(), pushStrainTensorsForward());
}

/******************************************************************************
* Unpacks the results of the computation engine and stores them in the modifier.
******************************************************************************/
void ElasticStrainModifier::transferComputationResults(ComputeEngine* engine)
{
	StructureIdentificationModifier::transferComputationResults(engine);

	ElasticStrainEngine* eng = static_cast<ElasticStrainEngine*>(engine);
	_atomClusters = eng->atomClusters();
	_clusterGraph = eng->clusterGraph();
	_strainTensors = eng->strainTensors();
	_deformationGradients = eng->deformationGradients();
	_volumetricStrainValues = eng->volumetricStrains();
}

/******************************************************************************
* Lets the modifier insert the cached computation results into the
* modification pipeline.
******************************************************************************/
PipelineStatus ElasticStrainModifier::applyComputationResults(TimePoint time, TimeInterval& validityInterval)
{
	StructureIdentificationModifier::applyComputationResults(time, validityInterval);

	if(!_volumetricStrainValues)
		throwException(tr("No computation results available."));

	if(outputParticleCount() != _volumetricStrainValues->size())
		throwException(tr("The number of input particles has changed. The stored results have become invalid."));

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
	if(_atomClusters && _atomClusters->size() == outputParticleCount())
		outputStandardProperty(_atomClusters.data());

	if(calculateStrainTensors() && _strainTensors && _strainTensors->size() == outputParticleCount())
		outputStandardProperty(_strainTensors.data());

	if(calculateDeformationGradients() && _deformationGradients && _deformationGradients->size() == outputParticleCount())
		outputStandardProperty(_deformationGradients.data());

	if(_volumetricStrainValues && _volumetricStrainValues->size() == outputParticleCount())
		outputCustomProperty(_volumetricStrainValues.data());

	return PipelineStatus::Success;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace

