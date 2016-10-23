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
#include <plugins/particles/objects/SurfaceMesh.h>
#include <core/scene/objects/geometry/TriMeshObject.h>
#include <core/scene/objects/geometry/TriMeshDisplay.h>
#include <plugins/particles/objects/SimulationCellObject.h>
#include <plugins/particles/objects/BondsObject.h>
#include <plugins/crystalanalysis/objects/dislocations/DislocationNetworkObject.h>
#include <plugins/crystalanalysis/objects/clusters/ClusterGraphObject.h>
#include <plugins/crystalanalysis/objects/patterns/StructurePattern.h>
#include "DislocationAnalysisModifier.h"
#include "DislocationAnalysisEngine.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(CrystalAnalysis, DislocationAnalysisModifier, StructureIdentificationModifier);
DEFINE_FLAGS_PROPERTY_FIELD(DislocationAnalysisModifier, _inputCrystalStructure, "CrystalStructure", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(DislocationAnalysisModifier, _maxTrialCircuitSize, "MaxTrialCircuitSize");
DEFINE_PROPERTY_FIELD(DislocationAnalysisModifier, _circuitStretchability, "CircuitStretchability");
DEFINE_PROPERTY_FIELD(DislocationAnalysisModifier, _outputInterfaceMesh, "OutputInterfaceMesh");
DEFINE_PROPERTY_FIELD(DislocationAnalysisModifier, _reconstructEdgeVectors, "ReconstructEdgeVectors");
DEFINE_PROPERTY_FIELD(DislocationAnalysisModifier, _onlyPerfectDislocations, "OnlyPerfectDislocations");
DEFINE_FLAGS_REFERENCE_FIELD(DislocationAnalysisModifier, _patternCatalog, "PatternCatalog", PatternCatalog, PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_REFERENCE_FIELD(DislocationAnalysisModifier, _dislocationDisplay, "DislocationDisplay", DislocationDisplay, PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_REFERENCE_FIELD(DislocationAnalysisModifier, _defectMeshDisplay, "DefectMeshDisplay", SurfaceMeshDisplay, PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_REFERENCE_FIELD(DislocationAnalysisModifier, _interfaceMeshDisplay, "InterfaceMeshDisplay", SurfaceMeshDisplay, PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_REFERENCE_FIELD(DislocationAnalysisModifier, _smoothDislocationsModifier, "SmoothDislocationsModifier", SmoothDislocationsModifier, PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_REFERENCE_FIELD(DislocationAnalysisModifier, _smoothSurfaceModifier, "SmoothSurfaceModifier", SmoothSurfaceModifier, PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);
SET_PROPERTY_FIELD_LABEL(DislocationAnalysisModifier, _inputCrystalStructure, "Input crystal structure");
SET_PROPERTY_FIELD_LABEL(DislocationAnalysisModifier, _maxTrialCircuitSize, "Trial circuit length");
SET_PROPERTY_FIELD_LABEL(DislocationAnalysisModifier, _circuitStretchability, "Circuit stretchability");
SET_PROPERTY_FIELD_LABEL(DislocationAnalysisModifier, _outputInterfaceMesh, "Output interface mesh");
SET_PROPERTY_FIELD_LABEL(DislocationAnalysisModifier, _reconstructEdgeVectors, "Reconstruct edge vectors");
SET_PROPERTY_FIELD_LABEL(DislocationAnalysisModifier, _onlyPerfectDislocations, "Generate perfect dislocations");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(DislocationAnalysisModifier, _maxTrialCircuitSize, IntegerParameterUnit, 3);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(DislocationAnalysisModifier, _circuitStretchability, IntegerParameterUnit, 0);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
DislocationAnalysisModifier::DislocationAnalysisModifier(DataSet* dataset) : StructureIdentificationModifier(dataset),
		_inputCrystalStructure(StructureAnalysis::LATTICE_FCC), _maxTrialCircuitSize(14), _circuitStretchability(9), _outputInterfaceMesh(false),
		_reconstructEdgeVectors(false), _onlyPerfectDislocations(false)
{
	INIT_PROPERTY_FIELD(DislocationAnalysisModifier::_inputCrystalStructure);
	INIT_PROPERTY_FIELD(DislocationAnalysisModifier::_maxTrialCircuitSize);
	INIT_PROPERTY_FIELD(DislocationAnalysisModifier::_circuitStretchability);
	INIT_PROPERTY_FIELD(DislocationAnalysisModifier::_outputInterfaceMesh);
	INIT_PROPERTY_FIELD(DislocationAnalysisModifier::_reconstructEdgeVectors);
	INIT_PROPERTY_FIELD(DislocationAnalysisModifier::_onlyPerfectDislocations);
	INIT_PROPERTY_FIELD(DislocationAnalysisModifier::_patternCatalog);
	INIT_PROPERTY_FIELD(DislocationAnalysisModifier::_dislocationDisplay);
	INIT_PROPERTY_FIELD(DislocationAnalysisModifier::_defectMeshDisplay);
	INIT_PROPERTY_FIELD(DislocationAnalysisModifier::_interfaceMeshDisplay);
	INIT_PROPERTY_FIELD(DislocationAnalysisModifier::_smoothDislocationsModifier);
	INIT_PROPERTY_FIELD(DislocationAnalysisModifier::_smoothSurfaceModifier);

	// Create the display objects.
	_dislocationDisplay = new DislocationDisplay(dataset);

	_defectMeshDisplay = new SurfaceMeshDisplay(dataset);
	_defectMeshDisplay->setShowCap(true);
	_defectMeshDisplay->setSmoothShading(true);
	_defectMeshDisplay->setCapTransparency(0.5);
	_defectMeshDisplay->setObjectTitle(tr("Defect mesh"));

	_interfaceMeshDisplay = new SurfaceMeshDisplay(dataset);
	_interfaceMeshDisplay->setShowCap(false);
	_interfaceMeshDisplay->setSmoothShading(false);
	_interfaceMeshDisplay->setCapTransparency(0.5);
	_interfaceMeshDisplay->setObjectTitle(tr("Interface mesh"));

	// Create the internal modifiers.
	_smoothDislocationsModifier = new SmoothDislocationsModifier(dataset);
	_smoothSurfaceModifier = new SmoothSurfaceModifier(dataset);

	// Create pattern catalog.
	_patternCatalog = new PatternCatalog(dataset);
	while(_patternCatalog->patterns().empty() == false)
		_patternCatalog->removePattern(0);

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

	// Create Burgers vector families.

	StructurePattern* fccPattern = _patternCatalog->structureById(StructureAnalysis::LATTICE_FCC);
	fccPattern->setSymmetryType(StructurePattern::CubicSymmetry);
	fccPattern->setShortName(QStringLiteral("fcc"));
	fccPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/2<110> (Perfect)"), Vector3(1.0f/2.0f, 1.0f/2.0f, 0.0f), Color(0.2f,0.2f,1)));
	fccPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/6<112> (Shockley)"), Vector3(1.0f/6.0f, 1.0f/6.0f, 2.0f/6.0f), Color(0,1,0)));
	fccPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/6<110> (Stair-rod)"), Vector3(1.0f/6.0f, 1.0f/6.0f, 0.0f/6.0f), Color(1,0,1)));
	fccPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/3<001> (Hirth)"), Vector3(1.0f/3.0f, 0.0f, 0.0f), Color(1,1,0)));
	fccPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/3<111> (Frank)"), Vector3(1.0f/3.0f, 1.0f/3.0f, 1.0f/3.0f), Color(0,1,1)));

	StructurePattern* bccPattern = _patternCatalog->structureById(StructureAnalysis::LATTICE_BCC);
	bccPattern->setSymmetryType(StructurePattern::CubicSymmetry);
	bccPattern->setShortName(QStringLiteral("bcc"));
	bccPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/2<111>"), Vector3(1.0f/2.0f, 1.0f/2.0f, 1.0f/2.0f), Color(0,1,0)));
	bccPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("<100>"), Vector3(1.0f, 0.0f, 0.0f), Color(1, 0.3f, 0.8f)));
	bccPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("<110>"), Vector3(1.0f, 1.0f, 0.0f), Color(0.2f, 0.5f, 1.0f)));

	StructurePattern* hcpPattern = _patternCatalog->structureById(StructureAnalysis::LATTICE_HCP);
	hcpPattern->setShortName(QStringLiteral("hcp"));
	hcpPattern->setSymmetryType(StructurePattern::HexagonalSymmetry);
	hcpPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/3<1-210>"), Vector3(sqrt(0.5f), 0.0f, 0.0f), Color(0,1,0)));
	hcpPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("<0001>"), Vector3(0.0f, 0.0f, sqrt(4.0f/3.0f)), Color(0.2f,0.2f,1)));
	hcpPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("<1-100>"), Vector3(0.0f, sqrt(3.0f/2.0f), 0.0f), Color(1,0,1)));
	hcpPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/3<1-100>"), Vector3(0.0f, sqrt(3.0f/2.0f)/3.0f, 0.0f), Color(1,0.5f,0)));
	hcpPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/3<1-213>"), Vector3(sqrt(0.5f), 0.0f, sqrt(4.0f/3.0f)), Color(1,1,0)));

	StructurePattern* cubicDiaPattern = _patternCatalog->structureById(StructureAnalysis::LATTICE_CUBIC_DIAMOND);
	cubicDiaPattern->setShortName(QStringLiteral("diamond"));
	cubicDiaPattern->setSymmetryType(StructurePattern::CubicSymmetry);
	cubicDiaPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/2<110>"), Vector3(1.0f/2.0f, 1.0f/2.0f, 0.0f), Color(0.2f,0.2f,1)));
	cubicDiaPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/6<112>"), Vector3(1.0f/6.0f, 1.0f/6.0f, 2.0f/6.0f), Color(0,1,0)));
	cubicDiaPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/6<110>"), Vector3(1.0f/6.0f, 1.0f/6.0f, 0.0f), Color(1,0,1)));
	cubicDiaPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/3<111>"), Vector3(1.0f/3.0f, 1.0f/3.0f, 1.0f/3.0f), Color(0,1,1)));

	StructurePattern* hexDiaPattern = _patternCatalog->structureById(StructureAnalysis::LATTICE_HEX_DIAMOND);
	hexDiaPattern->setShortName(QStringLiteral("hex_diamond"));
	hexDiaPattern->setSymmetryType(StructurePattern::HexagonalSymmetry);
	hexDiaPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/3<1-210>"), Vector3(sqrt(0.5f), 0.0f, 0.0f), Color(0,1,0)));
	hexDiaPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("<0001>"), Vector3(0.0f, 0.0f, sqrt(4.0f/3.0f)), Color(0.2f,0.2f,1)));
	hexDiaPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("<1-100>"), Vector3(0.0f, sqrt(3.0f/2.0f), 0.0f), Color(1,0,1)));
	hexDiaPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/3<1-100>"), Vector3(0.0f, sqrt(3.0f/2.0f)/3.0f, 0.0f), Color(1,0.5f,0)));
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void DislocationAnalysisModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	StructureIdentificationModifier::propertyChanged(field);

	// Recompute results when the parameters have changed.
	if(field == PROPERTY_FIELD(DislocationAnalysisModifier::_inputCrystalStructure)
			|| field == PROPERTY_FIELD(DislocationAnalysisModifier::_maxTrialCircuitSize)
			|| field == PROPERTY_FIELD(DislocationAnalysisModifier::_circuitStretchability)
			|| field == PROPERTY_FIELD(DislocationAnalysisModifier::_outputInterfaceMesh)
			|| field == PROPERTY_FIELD(DislocationAnalysisModifier::_reconstructEdgeVectors)
			|| field == PROPERTY_FIELD(DislocationAnalysisModifier::_onlyPerfectDislocations))
		invalidateCachedResults();
}

/******************************************************************************
* Handles reference events sent by reference targets of this object.
******************************************************************************/
bool DislocationAnalysisModifier::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	// Do not propagate messages from the attached display objects or the pattern catalog.
	if(source == defectMeshDisplay() || source == interfaceMeshDisplay() || source == dislocationDisplay())
		return false;

	return StructureIdentificationModifier::referenceEvent(source, event);
}

/******************************************************************************
* Resets the modifier's result cache.
******************************************************************************/
void DislocationAnalysisModifier::invalidateCachedResults()
{
	StructureIdentificationModifier::invalidateCachedResults();
	_defectMesh.reset();
	_interfaceMesh.reset();
	_atomClusters.reset();
	_clusterGraph.reset();
	_dislocationNetwork.reset();
	_unassignedEdges.reset();
	_segmentCounts.clear();
	_dislocationLengths.clear();
	_planarDefects.reset();
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
std::shared_ptr<AsynchronousParticleModifier::ComputeEngine> DislocationAnalysisModifier::createEngine(TimePoint time, TimeInterval validityInterval)
{
	// Get modifier inputs.
	ParticlePropertyObject* posProperty = expectStandardProperty(ParticleProperty::PositionProperty);
	SimulationCellObject* simCell = expectSimulationCell();

	// Get particle selection.
	ParticleProperty* selectionProperty = nullptr;
	if(onlySelectedParticles())
		selectionProperty = expectStandardProperty(ParticleProperty::SelectionProperty)->storage();

	// Build list of preferred crystal orientations.
	std::vector<Matrix3> preferredCrystalOrientations;
	if(inputCrystalStructure() == StructureAnalysis::LATTICE_FCC || inputCrystalStructure() == StructureAnalysis::LATTICE_BCC || inputCrystalStructure() == StructureAnalysis::LATTICE_CUBIC_DIAMOND) {
		preferredCrystalOrientations.push_back(Matrix3::Identity());
	}

	// Get cluster property.
	ParticlePropertyObject* clusterProperty = inputStandardProperty(ParticleProperty::ClusterProperty);

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<DislocationAnalysisEngine>(validityInterval, posProperty->storage(),
			simCell->data(), inputCrystalStructure(), maxTrialCircuitSize(), circuitStretchability(),
			reconstructEdgeVectors(), selectionProperty,
			clusterProperty ? clusterProperty->storage() : nullptr, std::move(preferredCrystalOrientations),
			onlyPerfectDislocations());
}

/******************************************************************************
* Unpacks the results of the computation engine and stores them in the modifier.
******************************************************************************/
void DislocationAnalysisModifier::transferComputationResults(ComputeEngine* engine)
{
	StructureIdentificationModifier::transferComputationResults(engine);

	DislocationAnalysisEngine* eng = static_cast<DislocationAnalysisEngine*>(engine);
	_defectMesh = eng->defectMesh();
	_isGoodEverywhere = eng->isGoodEverywhere();
	_isBadEverywhere = eng->isBadEverywhere();
	_atomClusters = eng->atomClusters();
	_clusterGraph = eng->clusterGraph();
	_dislocationNetwork = eng->dislocationNetwork();
	if(outputInterfaceMesh()) {
		_interfaceMesh = new HalfEdgeMesh<>();
		_interfaceMesh->copyFrom(eng->interfaceMesh());
	}
	else _interfaceMesh.reset();
	_simCell = eng->cell();
	_unassignedEdges = eng->elasticMapping().unassignedEdges();
	_segmentCounts.clear();
	_dislocationLengths.clear();

#if 0
	_planarDefects = eng->planarDefectIdentification().planarDefects();
#endif
}

/******************************************************************************
* Lets the modifier insert the cached computation results into the
* modification pipeline.
******************************************************************************/
PipelineStatus DislocationAnalysisModifier::applyComputationResults(TimePoint time, TimeInterval& validityInterval)
{
	StructureIdentificationModifier::applyComputationResults(time, validityInterval);

	if(!_dislocationNetwork)
		throwException(tr("No computation results available."));

	// Output defect mesh.
	OORef<SurfaceMesh> defectMeshObj(new SurfaceMesh(dataset(), _defectMesh.data()));
	defectMeshObj->setCompletelySolid(_isBadEverywhere);
	if(smoothSurfaceModifier() && smoothSurfaceModifier()->isEnabled() && smoothSurfaceModifier()->smoothingLevel() > 0)
		defectMeshObj->smoothMesh(_simCell, smoothSurfaceModifier()->smoothingLevel());
	defectMeshObj->setDisplayObject(_defectMeshDisplay);
	output().addObject(defectMeshObj);

	// Output interface mesh.
	if(_interfaceMesh) {
		OORef<SurfaceMesh> interfaceMeshObj(new SurfaceMesh(dataset(), _interfaceMesh.data()));
		interfaceMeshObj->setCompletelySolid(_isBadEverywhere);
		interfaceMeshObj->setDisplayObject(_interfaceMeshDisplay);
		output().addObject(interfaceMeshObj);
	}

	// Output cluster graph.
	OORef<ClusterGraphObject> clusterGraphObj(new ClusterGraphObject(dataset(), _clusterGraph.data()));
	if(ClusterGraphObject* oldClusterGraph = output().findObject<ClusterGraphObject>())
		output().removeObject(oldClusterGraph);
	output().addObject(clusterGraphObj);

	// Output dislocations.
	OORef<DislocationNetworkObject> dislocationsObj(new DislocationNetworkObject(dataset(), _dislocationNetwork.data()));
	dislocationsObj->setDisplayObject(_dislocationDisplay);
	if(smoothDislocationsModifier() && smoothDislocationsModifier()->isEnabled())
		smoothDislocationsModifier()->smoothDislocationLines(dislocationsObj);
	output().addObject(dislocationsObj);

	// Classify, count and measure length of dislocation segments.
	_segmentCounts.clear();
	_dislocationLengths.clear();
	FloatType totalLineLength = 0;
	int totalSegmentCount = 0;
	for(DislocationSegment* segment : dislocationsObj->storage()->segments()) {
		FloatType len = segment->calculateLength();
		totalLineLength += len;
		totalSegmentCount++;

		Cluster* cluster = segment->burgersVector.cluster();
		OVITO_ASSERT(cluster != nullptr);
		StructurePattern* pattern = _patternCatalog->structureById(cluster->structure);
		if(pattern == nullptr) continue;
		BurgersVectorFamily* family = pattern->defaultBurgersVectorFamily();
		for(BurgersVectorFamily* f : pattern->burgersVectorFamilies()) {
			if(f->isMember(segment->burgersVector.localVec(), pattern)) {
				family = f;
				break;
			}
		}
		_segmentCounts[family]++;
		_dislocationLengths[family] += len;
	}

	// Output pattern catalog.
	if(_patternCatalog) {
		if(PatternCatalog* oldCatalog = output().findObject<PatternCatalog>())
			output().removeObject(oldCatalog);
		output().addObject(_patternCatalog);
	}

	// Output particle properties.
	if(_atomClusters)
		outputStandardProperty(_atomClusters.data());

	// Output planar defects.
	if(_planarDefects) {
		OORef<TriMeshObject> triMeshObj(new TriMeshObject(dataset()));
		triMeshObj->mesh() = _planarDefects->mesh();
		triMeshObj->setDisplayObject(new TriMeshDisplay(dataset()));
		output().addObject(triMeshObj);

		OORef<TriMeshObject> triMeshObj2(new TriMeshObject(dataset()));
		triMeshObj2->mesh() = _planarDefects->grainBoundaryMesh();
		triMeshObj2->setDisplayObject(new TriMeshDisplay(dataset()));
		output().addObject(triMeshObj2);
	}

	if(_unassignedEdges) {
		OORef<BondsObject> bondsObj(new BondsObject(dataset(), _unassignedEdges.data()));
		output().addObject(bondsObj);
	}

	output().attributes().insert(QStringLiteral("DislocationAnalysis.total_line_length"), QVariant::fromValue(totalLineLength));

	if(totalSegmentCount == 0)
		return PipelineStatus(PipelineStatus::Success, tr("No dislocations found"));
	else
		return PipelineStatus(PipelineStatus::Success, tr("Found %1 dislocation segments\nTotal line length: %2").arg(totalSegmentCount).arg(totalLineLength));
}

}	// End of namespace
}	// End of namespace
}	// End of namespace

