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
#include <core/gui/properties/IntegerParameterUI.h>
#include <core/gui/properties/FloatParameterUI.h>
#include <core/gui/properties/BooleanParameterUI.h>
#include <core/gui/properties/SubObjectParameterUI.h>
#include <plugins/particles/objects/SimulationCellObject.h>
#include <plugins/crystalanalysis/objects/dislocations/DislocationNetworkObject.h>
#include <plugins/crystalanalysis/objects/clusters/ClusterGraphObject.h>
#include "DislocationAnalysisModifier.h"
#include "DislocationAnalysisEngine.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(CrystalAnalysis, DislocationAnalysisModifier, AsynchronousParticleModifier);
IMPLEMENT_OVITO_OBJECT(CrystalAnalysis, DislocationAnalysisModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(DislocationAnalysisModifier, DislocationAnalysisModifierEditor);
DEFINE_FLAGS_REFERENCE_FIELD(DislocationAnalysisModifier, _dislocationDisplay, "DislocationDisplay", DislocationDisplay, PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_REFERENCE_FIELD(DislocationAnalysisModifier, _defectMeshDisplay, "DefectMeshDisplay", SurfaceMeshDisplay, PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_REFERENCE_FIELD(DislocationAnalysisModifier, _interfaceMeshDisplay, "InterfaceMeshDisplay", SurfaceMeshDisplay, PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_REFERENCE_FIELD(DislocationAnalysisModifier, _smoothDislocationsModifier, "SmoothDislocationsModifier", SmoothDislocationsModifier, PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_REFERENCE_FIELD(DislocationAnalysisModifier, _smoothSurfaceModifier, "SmoothSurfaceModifier", SmoothSurfaceModifier, PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
DislocationAnalysisModifier::DislocationAnalysisModifier(DataSet* dataset) : AsynchronousParticleModifier(dataset)
{
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
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void DislocationAnalysisModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	AsynchronousParticleModifier::propertyChanged(field);
}

/******************************************************************************
* Handles reference events sent by reference targets of this object.
******************************************************************************/
bool DislocationAnalysisModifier::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	// Do not propagate messages from the attached display objects.
	if(source == defectMeshDisplay() || source == interfaceMeshDisplay() || source == dislocationDisplay())
		return false;

	return AsynchronousParticleModifier::referenceEvent(source, event);
}

/******************************************************************************
* Resets the modifier's result cache.
******************************************************************************/
void DislocationAnalysisModifier::invalidateCachedResults()
{
	AsynchronousParticleModifier::invalidateCachedResults();
	_defectMesh.reset();
	_atomStructures.reset();
	_atomClusters.reset();
	_clusterGraph.reset();
	_dislocationNetwork.reset();
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
std::shared_ptr<AsynchronousParticleModifier::ComputeEngine> DislocationAnalysisModifier::createEngine(TimePoint time, TimeInterval validityInterval)
{
	// Get modifier inputs.
	ParticlePropertyObject* posProperty = expectStandardProperty(ParticleProperty::PositionProperty);
	SimulationCellObject* simCell = expectSimulationCell();

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<DislocationAnalysisEngine>(validityInterval, posProperty->storage(),
			simCell->data());
}

/******************************************************************************
* Unpacks the results of the computation engine and stores them in the modifier.
******************************************************************************/
void DislocationAnalysisModifier::transferComputationResults(ComputeEngine* engine)
{
	DislocationAnalysisEngine* eng = static_cast<DislocationAnalysisEngine*>(engine);
	_defectMesh = eng->defectMesh();
	_isGoodEverywhere = eng->isGoodEverywhere();
	_isBadEverywhere = eng->isBadEverywhere();
	_atomStructures = eng->structureTypes();
	_atomClusters = eng->atomClusters();
	_clusterGraph = eng->clusterGraph();
	_dislocationNetwork = eng->dislocationNetwork();
	_interfaceMesh = new HalfEdgeMesh<>();
	_interfaceMesh->copyFrom(eng->interfaceMesh());
	_simCell = eng->simulationCell();
}

/******************************************************************************
* Lets the modifier insert the cached computation results into the
* modification pipeline.
******************************************************************************/
PipelineStatus DislocationAnalysisModifier::applyComputationResults(TimePoint time, TimeInterval& validityInterval)
{
	if(!_defectMesh)
		throw Exception(tr("No computation results available."));

	// Output defect mesh.
	OORef<SurfaceMesh> defectMeshObj(new SurfaceMesh(dataset(), _defectMesh.data()));
	defectMeshObj->setCompletelySolid(_isGoodEverywhere);
	if(smoothSurfaceModifier() && smoothSurfaceModifier()->isEnabled() && smoothSurfaceModifier()->smoothingLevel() > 0)
		defectMeshObj->smoothMesh(_simCell, smoothSurfaceModifier()->smoothingLevel());
	defectMeshObj->setDisplayObject(_defectMeshDisplay);
	output().addObject(defectMeshObj);

	// Output interface mesh.
	OORef<SurfaceMesh> interfaceMeshObj(new SurfaceMesh(dataset(), _interfaceMesh.data()));
	interfaceMeshObj->setCompletelySolid(_isGoodEverywhere);
	interfaceMeshObj->setDisplayObject(_interfaceMeshDisplay);
	output().addObject(interfaceMeshObj);

	// Output cluster graph.
	OORef<ClusterGraphObject> clusterGraphObj(new ClusterGraphObject(dataset(), _clusterGraph.data()));
	output().addObject(clusterGraphObj);

	// Output dislocations.
	OORef<DislocationNetworkObject> dislocationsObj(new DislocationNetworkObject(dataset(), _dislocationNetwork.data()));
	dislocationsObj->setDisplayObject(_dislocationDisplay);
	if(smoothDislocationsModifier() && smoothDislocationsModifier()->isEnabled())
		smoothDislocationsModifier()->smoothDislocationLines(dislocationsObj);
	output().addObject(dislocationsObj);

	// Output particle properties.
	if(_atomStructures)
		outputStandardProperty(_atomStructures.data());
	if(_atomClusters)
		outputStandardProperty(_atomClusters.data());

	return PipelineStatus(PipelineStatus::Success);
}

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void DislocationAnalysisModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create the rollout.
	QWidget* rollout = createRollout(tr("Dislocation analysis"), rolloutParams);

    QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(6);
	layout->setColumnStretch(1, 1);

	// Status label.
	layout->setRowMinimumHeight(3, 10);
	layout->addWidget(statusLabel(), 4, 0, 1, 2);
	statusLabel()->setMinimumHeight(100);

	// Open a sub-editor for the mesh display object.
	new SubObjectParameterUI(this, PROPERTY_FIELD(DislocationAnalysisModifier::_defectMeshDisplay), rolloutParams.after(rollout));

	// Open a sub-editor for the internal surface smoothing modifier.
	new SubObjectParameterUI(this, PROPERTY_FIELD(DislocationAnalysisModifier::_smoothSurfaceModifier), rolloutParams.after(rollout).setTitle(tr("Post-processing")));

	// Open a sub-editor for the dislocation display object.
	new SubObjectParameterUI(this, PROPERTY_FIELD(DislocationAnalysisModifier::_dislocationDisplay), rolloutParams.after(rollout));

	// Open a sub-editor for the internal line smoothing modifier.
	new SubObjectParameterUI(this, PROPERTY_FIELD(DislocationAnalysisModifier::_smoothDislocationsModifier), rolloutParams.after(rollout).setTitle(tr("Post-processing")));
}

}	// End of namespace
}	// End of namespace
}	// End of namespace

