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

#include "GrainSegmentationModifier.h"

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/particles/objects/SimulationCellObject.h>
#include <plugins/crystalanalysis/objects/clusters/ClusterGraphObject.h>
#include <plugins/crystalanalysis/objects/patterns/StructurePattern.h>
#include <plugins/particles/objects/BondsDisplay.h>
#include <ptm/index_ptm.h>
#include "../grains/GrainSegmentationEngine.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(GrainSegmentationModifier, StructureIdentificationModifier);
DEFINE_FLAGS_PROPERTY_FIELD(GrainSegmentationModifier, inputCrystalStructure, "CrystalStructure", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(GrainSegmentationModifier, rmsdCutoff, "RMSDCutoff");
DEFINE_FLAGS_PROPERTY_FIELD(GrainSegmentationModifier, misorientationThreshold, "MisorientationThreshold", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(GrainSegmentationModifier, minGrainAtomCount, "MinGrainAtomCount", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_REFERENCE_FIELD(GrainSegmentationModifier, patternCatalog, "PatternCatalog", PatternCatalog, PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(GrainSegmentationModifier, smoothingLevel, "SmoothingLevel", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(GrainSegmentationModifier, probeSphereRadius, "Radius", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(GrainSegmentationModifier, outputLocalOrientations, "OutputLocalOrientations");
DEFINE_FLAGS_REFERENCE_FIELD(GrainSegmentationModifier, meshDisplay, "MeshDisplay", PartitionMeshDisplay, PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_REFERENCE_FIELD(GrainSegmentationModifier, bondsDisplay, "BondsDisplay", BondsDisplay, PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);

DEFINE_PROPERTY_FIELD(GrainSegmentationModifier, onlySelectedParticles, "OnlySelectedParticles");
DEFINE_PROPERTY_FIELD(GrainSegmentationModifier, outputPartitionMesh, "OutputPartitionMesh");
//DEFINE_FLAGS_PROPERTY_FIELD(GrainSegmentationModifier, numOrientationSmoothingIterations, "NumOrientationSmoothingIterations", PROPERTY_FIELD_MEMORIZE);
//DEFINE_FLAGS_PROPERTY_FIELD(GrainSegmentationModifier, orientationSmoothingWeight, "OrientationSmoothingWeight", PROPERTY_FIELD_MEMORIZE);
SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier, inputCrystalStructure, "Input crystal structure");
SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier, rmsdCutoff, "RMSD cutoff");
SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier, misorientationThreshold, "Misorientation angle threshold");
SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier, minGrainAtomCount, "Minimum grain size (# of atoms)");
SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier, smoothingLevel, "Smoothing level");
SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier, probeSphereRadius, "Probe sphere radius");
SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier, meshDisplay, "Surface mesh display");
SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier, onlySelectedParticles, "Use only selected particles");
SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier, outputPartitionMesh, "Generate mesh");
SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier, outputLocalOrientations, "Output local orientations");
SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier, bondsDisplay, "Bonds display");
//SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier, numOrientationSmoothingIterations, "Orientation smoothing iterations");
//SET_PROPERTY_FIELD_LABEL(GrainSegmentationModifier, orientationSmoothingWeight, "Orientation smoothing weight");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(GrainSegmentationModifier, rmsdCutoff, FloatParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(GrainSegmentationModifier, misorientationThreshold, AngleParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(GrainSegmentationModifier, probeSphereRadius, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(GrainSegmentationModifier, minGrainAtomCount, IntegerParameterUnit, 1);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(GrainSegmentationModifier, smoothingLevel, IntegerParameterUnit, 0);
//SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(GrainSegmentationModifier, numOrientationSmoothingIterations, IntegerParameterUnit, 0);
//SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(GrainSegmentationModifier, orientationSmoothingWeight, FloatParameterUnit, 0);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
GrainSegmentationModifier::GrainSegmentationModifier(DataSet* dataset) : StructureIdentificationModifier(dataset),
		_inputCrystalStructure(GrainSegmentationEngine::FCC),
		_rmsdCutoff(0.1),
                _misorientationThreshold(4.0 * FLOATTYPE_PI / 180.0),
		_rmsdHistogramBinSize(0),
		_minGrainAtomCount(10),
		_smoothingLevel(8),
		_probeSphereRadius(4),
		_onlySelectedParticles(false),
		_outputPartitionMesh(false),
		_outputLocalOrientations(false)
//		_numOrientationSmoothingIterations(4),
		//_orientationSmoothingWeight(0.5)
{
	INIT_PROPERTY_FIELD(inputCrystalStructure);
	INIT_PROPERTY_FIELD(rmsdCutoff);
	INIT_PROPERTY_FIELD(misorientationThreshold);
	INIT_PROPERTY_FIELD(minGrainAtomCount);
	INIT_PROPERTY_FIELD(patternCatalog);
	INIT_PROPERTY_FIELD(smoothingLevel);
	INIT_PROPERTY_FIELD(probeSphereRadius);
	INIT_PROPERTY_FIELD(meshDisplay);
	INIT_PROPERTY_FIELD(onlySelectedParticles);
	INIT_PROPERTY_FIELD(outputLocalOrientations);
	//INIT_PROPERTY_FIELD(numOrientationSmoothingIterations);
	//INIT_PROPERTY_FIELD(orientationSmoothingWeight);
	INIT_PROPERTY_FIELD(outputPartitionMesh);
	INIT_PROPERTY_FIELD(bondsDisplay);

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
	OVITO_STATIC_ASSERT(sizeof(predefTypes)/sizeof(predefTypes[0]) == GrainSegmentationEngine::NUM_STRUCTURE_TYPES);
	for(int id = 0; id < GrainSegmentationEngine::NUM_STRUCTURE_TYPES; id++) {
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
	_bondsDisplay->setEnabled(false);
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void GrainSegmentationModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	StructureIdentificationModifier::propertyChanged(field);

	// Recompute results when the parameters have changed.
	if(field == PROPERTY_FIELD(inputCrystalStructure) ||
			field == PROPERTY_FIELD(rmsdCutoff) ||
			field == PROPERTY_FIELD(misorientationThreshold) ||
			field == PROPERTY_FIELD(minGrainAtomCount) ||
			field == PROPERTY_FIELD(smoothingLevel) ||
			field == PROPERTY_FIELD(probeSphereRadius) ||
			field == PROPERTY_FIELD(onlySelectedParticles) ||
			field == PROPERTY_FIELD(outputLocalOrientations) ||
			//field == PROPERTY_FIELD(numOrientationSmoothingIterations) ||
			//field == PROPERTY_FIELD(orientationSmoothingWeight) ||
			field == PROPERTY_FIELD(outputPartitionMesh))
		invalidateCachedResults();
}

/******************************************************************************
* Handles reference events sent by reference targets of this object.
******************************************************************************/
bool GrainSegmentationModifier::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	// Do not propagate messages from the attached display object.
	if(source == meshDisplay() || source == bondsDisplay())
		return false;

	return StructureIdentificationModifier::referenceEvent(source, event);
}

/******************************************************************************
* Resets the modifier's result cache.
******************************************************************************/
void GrainSegmentationModifier::invalidateCachedResults()
{
	StructureIdentificationModifier::invalidateCachedResults();
	_partitionMesh.reset();
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
std::shared_ptr<AsynchronousParticleModifier::ComputeEngine> GrainSegmentationModifier::createEngine(TimePoint time, TimeInterval validityInterval)
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

	// Determine the list of structure types the PTM should search for.
	QVector<bool> typesToIdentify(GrainSegmentationEngine::NUM_STRUCTURE_TYPES, false);
	typesToIdentify[inputCrystalStructure()] = true;

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<GrainSegmentationEngine>(validityInterval, posProperty->storage(),
			simCell->data(), typesToIdentify, selectionProperty,
			inputCrystalStructure(), rmsdCutoff(),
			_numOrientationSmoothingIterations, _orientationSmoothingWeight, misorientationThreshold(),
			minGrainAtomCount(), outputPartitionMesh() ? probeSphereRadius() : 0, smoothingLevel());
}

/******************************************************************************
* Unpacks the results of the computation engine and stores them in the modifier.
******************************************************************************/
void GrainSegmentationModifier::transferComputationResults(ComputeEngine* engine)
{
	StructureIdentificationModifier::transferComputationResults(engine);

	GrainSegmentationEngine* eng = static_cast<GrainSegmentationEngine*>(engine);
	_atomClusters = eng->atomClusters();
        _clusterGraph = eng->outputClusterGraph();
        _partitionMesh = eng->mesh();
        _spaceFillingRegion = eng->spaceFillingGrain();
        _symmetryorient = eng->symmetry();

        // Copy RMDS histogram data.
        _rmsdHistogramData = eng->rmsdHistogramData();
	_rmsdHistogramBinSize = eng->rmsdHistogramBinSize();

	if(outputLocalOrientations())
		_localOrientations = eng->localOrientations();
	else
		_localOrientations.reset();

#ifdef OVITO_DEBUG
	_latticeNeighborBonds = eng->latticeNeighborBonds();
	_neighborDisorientationAngles = eng->neighborDisorientationAngles();
	_defectDistances = eng->defectDistances();
	_defectDistanceBasins = eng->defectDistanceBasins();
#endif
}

/******************************************************************************
* Lets the modifier insert the cached computation results into the
* modification pipeline.
******************************************************************************/
PipelineStatus GrainSegmentationModifier::applyComputationResults(TimePoint time, TimeInterval& validityInterval)
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
        
        if (_symmetryorient){
          outputStandardProperty(_symmetryorient.data());
        }
        
        if(outputLocalOrientations() && _localOrientations) outputStandardProperty(_localOrientations.data());
        if(_defectDistances) outputCustomProperty(_defectDistances.data());
        if(_defectDistanceBasins) outputCustomProperty(_defectDistanceBasins.data());

	// Output lattice neighbor bonds.
	if(_latticeNeighborBonds) {
		std::vector<BondProperty*> bondProperties;
		if(_neighborDisorientationAngles && _neighborDisorientationAngles->size() == _latticeNeighborBonds->size())
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

	return PipelineStatus(PipelineStatus::Success, tr("Found %1 grains").arg(_clusterGraph ? (_clusterGraph->clusters().size() - 1) : 0));
}

}	// End of namespace
}	// End of namespace
}	// End of namespace

