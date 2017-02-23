///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2014) Alexander Stukowski
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
#include <plugins/particles/objects/SimulationCellObject.h>
#include <plugins/crystalanalysis/util/DelaunayTessellation.h>
#include <plugins/crystalanalysis/util/ManifoldConstructionHelper.h>
#include "ConstructSurfaceModifier.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(ConstructSurfaceModifier, AsynchronousParticleModifier);
DEFINE_FLAGS_PROPERTY_FIELD(ConstructSurfaceModifier, smoothingLevel, "SmoothingLevel", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(ConstructSurfaceModifier, probeSphereRadius, "Radius", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_REFERENCE_FIELD(ConstructSurfaceModifier, surfaceMeshDisplay, "SurfaceMeshDisplay", SurfaceMeshDisplay, PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(ConstructSurfaceModifier, onlySelectedParticles, "OnlySelectedParticles");
SET_PROPERTY_FIELD_LABEL(ConstructSurfaceModifier, smoothingLevel, "Smoothing level");
SET_PROPERTY_FIELD_LABEL(ConstructSurfaceModifier, probeSphereRadius, "Probe sphere radius");
SET_PROPERTY_FIELD_LABEL(ConstructSurfaceModifier, surfaceMeshDisplay, "Surface mesh display");
SET_PROPERTY_FIELD_LABEL(ConstructSurfaceModifier, onlySelectedParticles, "Use only selected particles");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ConstructSurfaceModifier, probeSphereRadius, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ConstructSurfaceModifier, smoothingLevel, IntegerParameterUnit, 0);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
ConstructSurfaceModifier::ConstructSurfaceModifier(DataSet* dataset) : AsynchronousParticleModifier(dataset),
	_smoothingLevel(8), _probeSphereRadius(4), _onlySelectedParticles(false),
	_solidVolume(0), _totalVolume(0), _surfaceArea(0)
{
	INIT_PROPERTY_FIELD(smoothingLevel);
	INIT_PROPERTY_FIELD(probeSphereRadius);
	INIT_PROPERTY_FIELD(surfaceMeshDisplay);
	INIT_PROPERTY_FIELD(onlySelectedParticles);

	// Create the display object.
	_surfaceMeshDisplay = new SurfaceMeshDisplay(dataset);
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void ConstructSurfaceModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	AsynchronousParticleModifier::propertyChanged(field);

	// Recompute results when the parameters have changed.
	if(field == PROPERTY_FIELD(smoothingLevel)
			|| field == PROPERTY_FIELD(probeSphereRadius)
			|| field == PROPERTY_FIELD(onlySelectedParticles))
		invalidateCachedResults();
}

/******************************************************************************
* Handles reference events sent by reference targets of this object.
******************************************************************************/
bool ConstructSurfaceModifier::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	// Do not propagate messages from the attached display object.
	if(source == surfaceMeshDisplay())
		return false;

	return AsynchronousParticleModifier::referenceEvent(source, event);
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
std::shared_ptr<AsynchronousParticleModifier::ComputeEngine> ConstructSurfaceModifier::createEngine(TimePoint time, TimeInterval validityInterval)
{
	// Get modifier inputs.
	ParticlePropertyObject* posProperty = expectStandardProperty(ParticleProperty::PositionProperty);
	ParticlePropertyObject* selProperty = nullptr;
	if(onlySelectedParticles())
		selProperty = expectStandardProperty(ParticleProperty::SelectionProperty);
	SimulationCellObject* simCell = expectSimulationCell();

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<ConstructSurfaceEngine>(validityInterval, posProperty->storage(),
			selProperty ? selProperty->storage() : nullptr,
			simCell->data(), probeSphereRadius(), smoothingLevel());
}

/******************************************************************************
* Unpacks the results of the computation engine and stores them in the modifier.
******************************************************************************/
void ConstructSurfaceModifier::transferComputationResults(ComputeEngine* engine)
{
	ConstructSurfaceEngine* eng = static_cast<ConstructSurfaceEngine*>(engine);
	_surfaceMesh = eng->mesh();
	_isCompletelySolid = eng->isCompletelySolid();
	_solidVolume = eng->solidVolume();
	_totalVolume = eng->totalVolume();
	_surfaceArea = eng->surfaceArea();
}

/******************************************************************************
* Lets the modifier insert the cached computation results into the
* modification pipeline.
******************************************************************************/
PipelineStatus ConstructSurfaceModifier::applyComputationResults(TimePoint time, TimeInterval& validityInterval)
{
	if(!_surfaceMesh)
		throwException(tr("No computation results available."));

	// Create the output data object.
	OORef<SurfaceMesh> meshObj(new SurfaceMesh(dataset(), _surfaceMesh.data()));
	meshObj->setIsCompletelySolid(_isCompletelySolid);
	meshObj->addDisplayObject(_surfaceMeshDisplay);

	// Insert output object into the pipeline.
	output().addObject(meshObj);

	output().attributes().insert(QStringLiteral("ConstructSurfaceMesh.surface_area"), QVariant::fromValue(surfaceArea()));
	output().attributes().insert(QStringLiteral("ConstructSurfaceMesh.solid_volume"), QVariant::fromValue(solidVolume()));

	return PipelineStatus(PipelineStatus::Success, tr("Surface area: %1\nSolid volume: %2\nTotal cell volume: %3\nSolid volume fraction: %4\nSurface area per solid volume: %5\nSurface area per total volume: %6")
			.arg(surfaceArea()).arg(solidVolume()).arg(totalVolume())
			.arg(solidVolume() / totalVolume()).arg(surfaceArea() / solidVolume()).arg(surfaceArea() / totalVolume()));
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void ConstructSurfaceModifier::ConstructSurfaceEngine::perform()
{
	setProgressText(tr("Constructing surface mesh"));

	if(_radius <= 0)
		throw Exception(tr("Radius parameter must be positive."));

	double alpha = _radius * _radius;
	FloatType ghostLayerSize = _radius * FloatType(3);

	// Check if combination of radius parameter and simulation cell size is valid.
	for(size_t dim = 0; dim < 3; dim++) {
		if(_simCell.pbcFlags()[dim]) {
			int stencilCount = (int)ceil(ghostLayerSize / _simCell.matrix().column(dim).dot(_simCell.cellNormalVector(dim)));
			if(stencilCount > 1)
				throw Exception(tr("Cannot generate Delaunay tessellation. Simulation cell is too small, or radius parameter is too large."));
		}
	}

	_solidVolume = 0;
	_surfaceArea = 0;

	// If there are too few particles, don't build Delaunay tessellation.
	// It is going to be invalid anyway.
	size_t numInputParticles = positions()->size();
	if(selection())
		numInputParticles = positions()->size() - std::count(selection()->constDataInt(), selection()->constDataInt() + selection()->size(), 0);
	if(numInputParticles <= 3)
		return;

	// Algorithm is divided into several sub-steps.
	// Assign weights to sub-steps according to estimated runtime.
	beginProgressSubSteps({ 20, 1, 6, 1 });

	// Generate Delaunay tessellation.
	DelaunayTessellation tessellation;
	if(!tessellation.generateTessellation(_simCell, positions()->constDataPoint3(), positions()->size(), ghostLayerSize,
			selection() ? selection()->constDataInt() : nullptr, *this))
		return;

	nextProgressSubStep();

	// Determines the region a solid Delaunay cell belongs to.
	// We use this callback function to compute the total volume of the solid region.
	auto tetrahedronRegion = [this, &tessellation](DelaunayTessellation::CellHandle cell) {
		if(tessellation.isGhostCell(cell) == false) {
			Point3 p0 = tessellation.vertexPosition(tessellation.cellVertex(cell, 0));
			Vector3 ad = tessellation.vertexPosition(tessellation.cellVertex(cell, 1)) - p0;
			Vector3 bd = tessellation.vertexPosition(tessellation.cellVertex(cell, 2)) - p0;
			Vector3 cd = tessellation.vertexPosition(tessellation.cellVertex(cell, 3)) - p0;
			_solidVolume += std::abs(ad.dot(cd.cross(bd))) / FloatType(6);
		}
		return 1;
	};

	ManifoldConstructionHelper<HalfEdgeMesh<>, true> manifoldConstructor(tessellation, *mesh(), alpha, positions());
	if(!manifoldConstructor.construct(tetrahedronRegion, *this))
		return;
	_isCompletelySolid = (manifoldConstructor.spaceFillingRegion() == 1);

	nextProgressSubStep();

	// Make sure every mesh vertex is only part of one surface manifold.
	_mesh->duplicateSharedVertices();

	nextProgressSubStep();
	if(!SurfaceMesh::smoothMesh(*_mesh, _simCell, _smoothingLevel, *this))
		return;

	// Compute surface area.
	for(const HalfEdgeMesh<>::Face* facet : _mesh->faces()) {
		if(isCanceled()) return;
		Vector3 e1 = _simCell.wrapVector(facet->edges()->vertex1()->pos() - facet->edges()->vertex2()->pos());
		Vector3 e2 = _simCell.wrapVector(facet->edges()->prevFaceEdge()->vertex1()->pos() - facet->edges()->vertex2()->pos());
		_surfaceArea += e1.cross(e2).length();
	}
	_surfaceArea *= FloatType(0.5);

	endProgressSubSteps();
}

}	// End of namespace
}	// End of namespace
}	// End of namespace

