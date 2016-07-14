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
#include <plugins/particles/objects/SimulationCellObject.h>
#include <plugins/crystalanalysis/objects/clusters/ClusterGraphObject.h>
#include <plugins/crystalanalysis/objects/patterns/StructurePattern.h>
#include <plugins/particles/objects/BondsDisplay.h>
#include "GrainSegmentationModifier2.h"
#include "GrainSegmentationEngine2.h"

#include <ptm/index_ptm.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(CrystalAnalysis, GrainSegmentationModifier2, StructureIdentificationModifier);
DEFINE_FLAGS_PROPERTY_FIELD(GrainSegmentationModifier2, _rmsdCutoff, "RMSDCutoff", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(GrainSegmentationModifier2, _misorientationThreshold, "MisorientationThreshold", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(GrainSegmentationModifier2, _minGrainAtomCount, "MinGrainAtomCount", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_REFERENCE_FIELD(GrainSegmentationModifier2, _patternCatalog, "PatternCatalog", PatternCatalog, PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(GrainSegmentationModifier2, _smoothingLevel, "SmoothingLevel", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(GrainSegmentationModifier2, _probeSphereRadius, "Radius", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(GrainSegmentationModifier2, _outputLocalOrientations, "OutputLocalOrientations");
DEFINE_FLAGS_REFERENCE_FIELD(GrainSegmentationModifier2, _meshDisplay, "MeshDisplay", PartitionMeshDisplay, PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_REFERENCE_FIELD(GrainSegmentationModifier2, _bondsDisplay, "BondsDisplay", BondsDisplay, PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(GrainSegmentationModifier2, _onlySelectedParticles, "OnlySelectedParticles");
DEFINE_PROPERTY_FIELD(GrainSegmentationModifier2, _outputPartitionMesh, "OutputPartitionMesh");
DEFINE_FLAGS_PROPERTY_FIELD(GrainSegmentationModifier2, _numOrientationSmoothingIterations, "NumOrientationSmoothingIterations", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(GrainSegmentationModifier2, _orientationSmoothingWeight, "OrientationSmoothingWeight", PROPERTY_FIELD_MEMORIZE);
SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier2, _rmsdCutoff, "RMSD cutoff");
SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier2, _misorientationThreshold, "Misorientation threshold");
SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier2, _minGrainAtomCount, "Minimum grain size");
SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier2, _smoothingLevel, "Smoothing level");
SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier2, _probeSphereRadius, "Probe sphere radius");
SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier2, _meshDisplay, "Surface mesh display");
SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier2, _onlySelectedParticles, "Use only selected particles");
SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier2, _outputPartitionMesh, "Generate mesh");
SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier2, _outputLocalOrientations, "Output local orientations");
SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier2, _bondsDisplay, "Bonds display");
SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier2, _numOrientationSmoothingIterations, "Number of smoothing iterations");
SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier2, _orientationSmoothingWeight, "Orientation smoothing weight");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(GrainSegmentationModifier2, _rmsdCutoff, FloatParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(GrainSegmentationModifier2, _misorientationThreshold, AngleParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(GrainSegmentationModifier2, _probeSphereRadius, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(GrainSegmentationModifier2, _minGrainAtomCount, IntegerParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(GrainSegmentationModifier2, _smoothingLevel, IntegerParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(GrainSegmentationModifier2, _numOrientationSmoothingIterations, IntegerParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(GrainSegmentationModifier2, _orientationSmoothingWeight, FloatParameterUnit, 0);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
GrainSegmentationModifier2::GrainSegmentationModifier2(DataSet* dataset) : StructureIdentificationModifier(dataset),
		_rmsdCutoff(0),
		_rmsdHistogramBinSize(0),
		_misorientationThreshold(3.0 * FLOATTYPE_PI / 180.0),
		_minGrainAtomCount(10),
		_smoothingLevel(8),
		_probeSphereRadius(4),
		_onlySelectedParticles(false),
		_outputPartitionMesh(false),
		_outputLocalOrientations(false),
		_numOrientationSmoothingIterations(1),
		_orientationSmoothingWeight(0.5)
{
	INIT_PROPERTY_FIELD(GrainSegmentationModifier2::_rmsdCutoff);
	INIT_PROPERTY_FIELD(GrainSegmentationModifier2::_misorientationThreshold);
	INIT_PROPERTY_FIELD(GrainSegmentationModifier2::_minGrainAtomCount);
	INIT_PROPERTY_FIELD(GrainSegmentationModifier2::_patternCatalog);
	INIT_PROPERTY_FIELD(GrainSegmentationModifier2::_smoothingLevel);
	INIT_PROPERTY_FIELD(GrainSegmentationModifier2::_probeSphereRadius);
	INIT_PROPERTY_FIELD(GrainSegmentationModifier2::_meshDisplay);
	INIT_PROPERTY_FIELD(GrainSegmentationModifier2::_onlySelectedParticles);
	INIT_PROPERTY_FIELD(GrainSegmentationModifier2::_outputLocalOrientations);
	INIT_PROPERTY_FIELD(GrainSegmentationModifier2::_numOrientationSmoothingIterations);
	INIT_PROPERTY_FIELD(GrainSegmentationModifier2::_orientationSmoothingWeight);
	INIT_PROPERTY_FIELD(GrainSegmentationModifier2::_outputPartitionMesh);
	INIT_PROPERTY_FIELD(GrainSegmentationModifier2::_bondsDisplay);

	// Create the display object.
	_meshDisplay = new PartitionMeshDisplay(dataset);

	// Create pattern catalog.
	_patternCatalog = new PatternCatalog(dataset);

	// Create the structure types.
	ParticleTypeProperty::PredefinedStructureType predefTypes[] = {
			ParticleTypeProperty::PredefinedStructureType::OTHER,
			ParticleTypeProperty::PredefinedStructureType::FCC,
			ParticleTypeProperty::PredefinedStructureType::HCP,
			ParticleTypeProperty::PredefinedStructureType::BCC,
			ParticleTypeProperty::PredefinedStructureType::ICO,
			ParticleTypeProperty::PredefinedStructureType::SC,
	};
	OVITO_STATIC_ASSERT(sizeof(predefTypes)/sizeof(predefTypes[0]) == GrainSegmentationEngine2::NUM_STRUCTURE_TYPES);
	for(int id = 0; id < GrainSegmentationEngine2::NUM_STRUCTURE_TYPES; id++) {
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

	// Create the display object for bonds rendering.
	_bondsDisplay = new BondsDisplay(dataset);
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void GrainSegmentationModifier2::propertyChanged(const PropertyFieldDescriptor& field)
{
	StructureIdentificationModifier::propertyChanged(field);

	// Recompute results when the parameters have changed.
	if(field == PROPERTY_FIELD(GrainSegmentationModifier2::_rmsdCutoff) ||
			field == PROPERTY_FIELD(GrainSegmentationModifier2::_misorientationThreshold) ||
			field == PROPERTY_FIELD(GrainSegmentationModifier2::_minGrainAtomCount) ||
			field == PROPERTY_FIELD(GrainSegmentationModifier2::_smoothingLevel) ||
			field == PROPERTY_FIELD(GrainSegmentationModifier2::_probeSphereRadius) ||
			field == PROPERTY_FIELD(GrainSegmentationModifier2::_onlySelectedParticles) ||
			field == PROPERTY_FIELD(GrainSegmentationModifier2::_outputLocalOrientations) ||
			field == PROPERTY_FIELD(GrainSegmentationModifier2::_numOrientationSmoothingIterations) ||
			field == PROPERTY_FIELD(GrainSegmentationModifier2::_orientationSmoothingWeight) ||
			field == PROPERTY_FIELD(GrainSegmentationModifier2::_outputPartitionMesh))
		invalidateCachedResults();
}

/******************************************************************************
* Handles reference events sent by reference targets of this object.
******************************************************************************/
bool GrainSegmentationModifier2::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	// Do not propagate messages from the attached display object.
	if(source == meshDisplay() || source == bondsDisplay())
		return false;

	return StructureIdentificationModifier::referenceEvent(source, event);
}

/******************************************************************************
* Resets the modifier's result cache.
******************************************************************************/
void GrainSegmentationModifier2::invalidateCachedResults()
{
	StructureIdentificationModifier::invalidateCachedResults();
	_partitionMesh.reset();
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
std::shared_ptr<AsynchronousParticleModifier::ComputeEngine> GrainSegmentationModifier2::createEngine(TimePoint time, TimeInterval validityInterval)
{
	// Get modifier inputs.
	ParticlePropertyObject* posProperty = expectStandardProperty(ParticleProperty::PositionProperty);
	SimulationCellObject* simCell = expectSimulationCell();

	// Get particle selection.
	ParticleProperty* selectionProperty = nullptr;
	if(onlySelectedParticles())
		selectionProperty = expectStandardProperty(ParticleProperty::SelectionProperty)->storage();

	// Initialize PTM library.
	ptm_initialize_global();

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<GrainSegmentationEngine2>(validityInterval, posProperty->storage(),
			simCell->data(), getTypesToIdentify(GrainSegmentationEngine2::NUM_STRUCTURE_TYPES), selectionProperty, rmsdCutoff(),
			_numOrientationSmoothingIterations, _orientationSmoothingWeight, misorientationThreshold(),
			minGrainAtomCount(), outputPartitionMesh() ? probeSphereRadius() : 0, smoothingLevel());
}

/******************************************************************************
* Unpacks the results of the computation engine and stores them in the modifier.
******************************************************************************/
void GrainSegmentationModifier2::transferComputationResults(ComputeEngine* engine)
{
	StructureIdentificationModifier::transferComputationResults(engine);

	GrainSegmentationEngine2* eng = static_cast<GrainSegmentationEngine2*>(engine);
	_atomClusters = eng->atomClusters();
	_clusterGraph = eng->outputClusterGraph();
	_partitionMesh = eng->mesh();
	_spaceFillingRegion = eng->spaceFillingGrain();

	// Copy RMDS histogram data.
	_rmsdHistogramData = eng->rmsdHistogramData();
	_rmsdHistogramBinSize = eng->rmsdHistogramBinSize();

	if(outputLocalOrientations())
		_localOrientations = eng->localOrientations();
	else
		_localOrientations.reset();

	_latticeNeighborBonds = eng->latticeNeighborBonds();
	_neighborDisorientationAngles= eng->neighborDisorientationAngles();
}

/******************************************************************************
* Lets the modifier insert the cached computation results into the
* modification pipeline.
******************************************************************************/
PipelineStatus GrainSegmentationModifier2::applyComputationResults(TimePoint time, TimeInterval& validityInterval)
{
	StructureIdentificationModifier::applyComputationResults(time, validityInterval);

	if(!_atomClusters)
		throwException(tr("No computation results available."));

	if(outputParticleCount() != _atomClusters->size())
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
	outputStandardProperty(_atomClusters.data());
	if(outputLocalOrientations() && _localOrientations) outputStandardProperty(_localOrientations.data());

	// Output lattice neighbor bonds.
	if(_latticeNeighborBonds) {
		std::vector<BondProperty*> bondProperties;
		if(_neighborDisorientationAngles)
			bondProperties.push_back(_neighborDisorientationAngles.data());
		addBonds(_latticeNeighborBonds.data(), bondsDisplay(), bondProperties);
	}

	if(_partitionMesh) {
		// Create the output data object for the partition mesh.
		OORef<PartitionMesh> meshObj(new PartitionMesh(dataset(), _partitionMesh.data()));
		meshObj->setSpaceFillingRegion(_spaceFillingRegion);
		meshObj->addDisplayObject(_meshDisplay);

		// Insert output object into the pipeline.
		output().addObject(meshObj);
	}

	return PipelineStatus::Success;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace

