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
#include <plugins/crystalanalysis/objects/clusters/ClusterGraphObject.h>
#include <plugins/particles/objects/SimulationCellObject.h>
#include <core/rendering/SceneRenderer.h>
#include <core/utilities/mesh/TriMesh.h>
#include <core/animation/controller/Controller.h>
#include "PartitionMeshDisplay.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(PartitionMeshDisplay, AsynchronousDisplayObject);
DEFINE_FLAGS_PROPERTY_FIELD(PartitionMeshDisplay, surfaceColor, "SurfaceColor", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(PartitionMeshDisplay, showCap, "ShowCap", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(PartitionMeshDisplay, smoothShading, "SmoothShading");
DEFINE_PROPERTY_FIELD(PartitionMeshDisplay, flipOrientation, "FlipOrientation");
DEFINE_REFERENCE_FIELD(PartitionMeshDisplay, surfaceTransparencyController, "SurfaceTransparency", Controller);
DEFINE_REFERENCE_FIELD(PartitionMeshDisplay, capTransparencyController, "CapTransparency", Controller);
SET_PROPERTY_FIELD_LABEL(PartitionMeshDisplay, surfaceColor, "Free surface color");
SET_PROPERTY_FIELD_LABEL(PartitionMeshDisplay, showCap, "Show cap polygons");
SET_PROPERTY_FIELD_LABEL(PartitionMeshDisplay, smoothShading, "Smooth shading");
SET_PROPERTY_FIELD_LABEL(PartitionMeshDisplay, surfaceTransparencyController, "Surface transparency");
SET_PROPERTY_FIELD_LABEL(PartitionMeshDisplay, capTransparencyController, "Cap transparency");
SET_PROPERTY_FIELD_LABEL(PartitionMeshDisplay, flipOrientation, "Flip surface orientation");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(PartitionMeshDisplay, surfaceTransparencyController, PercentParameterUnit, 0, 1);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(PartitionMeshDisplay, capTransparencyController, PercentParameterUnit, 0, 1);

/******************************************************************************
* Constructor.
******************************************************************************/
PartitionMeshDisplay::PartitionMeshDisplay(DataSet* dataset) : AsynchronousDisplayObject(dataset),
	_surfaceColor(1, 1, 1), _showCap(true), _smoothShading(true), _flipOrientation(false), _trimeshUpdate(true)
{
	INIT_PROPERTY_FIELD(surfaceColor);
	INIT_PROPERTY_FIELD(showCap);
	INIT_PROPERTY_FIELD(smoothShading);
	INIT_PROPERTY_FIELD(surfaceTransparencyController);
	INIT_PROPERTY_FIELD(capTransparencyController);
	INIT_PROPERTY_FIELD(flipOrientation);

	setSurfaceTransparencyController(ControllerManager::createFloatController(dataset));
	setCapTransparencyController(ControllerManager::createFloatController(dataset));
}

/******************************************************************************
* Computes the bounding box of the displayed data.
******************************************************************************/
Box3 PartitionMeshDisplay::boundingBox(TimePoint time, DataObject* dataObject, ObjectNode* contextNode, const PipelineFlowState& flowState)
{
	// We'll use the entire simulation cell as bounding box for the mesh.
	if(SimulationCellObject* cellObject = flowState.findObject<SimulationCellObject>())
		return Box3(Point3(0,0,0), Point3(1,1,1)).transformed(cellObject->cellMatrix());
	else
		return Box3();
}

/******************************************************************************
* Creates a computation engine that will prepare the data to be displayed.
******************************************************************************/
std::shared_ptr<AsynchronousTask> PartitionMeshDisplay::createEngine(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState)
{
	// Get the simulation cell.
	SimulationCellObject* cellObject = flowState.findObject<SimulationCellObject>();

	// Get the partition mesh.
	PartitionMesh* partitionMeshObj = dynamic_object_cast<PartitionMesh>(dataObject);

	// Check if input is available.
	if(cellObject && partitionMeshObj) {
		// Check if the input has changed.
		if(_preparationCacheHelper.updateState(dataObject, cellObject->data(), flipOrientation())) {
			// Create compute engine.
			return std::make_shared<PrepareMeshEngine>(partitionMeshObj->storage(), cellObject->data(), partitionMeshObj->spaceFillingRegion(), partitionMeshObj->cuttingPlanes(), flipOrientation());
		}
	}
	else {
		_surfaceMesh.clear();
		_capPolygonsMesh.clear();
		_trimeshUpdate = true;
	}

	return std::shared_ptr<AsynchronousTask>();
}

/******************************************************************************
* Computes the results and stores them in this object for later retrieval.
******************************************************************************/
void PartitionMeshDisplay::PrepareMeshEngine::perform()
{
	setProgressText(tr("Preparing microstructure mesh for display"));

	if(!buildMesh(*_inputMesh, _simCell, _cuttingPlanes, _surfaceMesh, *this))
		throw Exception(tr("Failed to generate non-periodic version of microstructure mesh for display. Simulation cell might be too small."));

	if(_flipOrientation)
		_surfaceMesh.flipFaces();

	if(isCanceled())
		return;
}

/******************************************************************************
* Unpacks the results of the computation engine and stores them in the display object.
******************************************************************************/
void PartitionMeshDisplay::transferComputationResults(AsynchronousTask* engine)
{
	if(engine) {
		_surfaceMesh = static_cast<PrepareMeshEngine*>(engine)->surfaceMesh();
		_capPolygonsMesh = static_cast<PrepareMeshEngine*>(engine)->capPolygonsMesh();
		_trimeshUpdate = true;
	}
	else {
		// Reset cache when compute task has been canceled.
		_preparationCacheHelper.updateState(nullptr, SimulationCell(), false);
	}
}

/******************************************************************************
* Lets the display object render the data object.
******************************************************************************/
void PartitionMeshDisplay::render(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState, SceneRenderer* renderer, ObjectNode* contextNode)
{
	// Check if geometry preparation was successful.
	// If not, reset triangle mesh.
	if(status().type() == PipelineStatus::Error && _surfaceMesh.faceCount() != 0) {
		_surfaceMesh.clear();
		_capPolygonsMesh.clear();
		_trimeshUpdate = true;
	}

	// Get the cluster graph.
	ClusterGraphObject* clusterGraph = flowState.findObject<ClusterGraphObject>();

	// Get the rendering colors for the surface and cap meshes.
	FloatType transp_surface = 0;
	FloatType transp_cap = 0;
	TimeInterval iv;
	if(surfaceTransparencyController()) transp_surface = surfaceTransparencyController()->getFloatValue(time, iv);
	if(capTransparencyController()) transp_cap = capTransparencyController()->getFloatValue(time, iv);
	ColorA color_surface(surfaceColor(), FloatType(1) - transp_surface);

	// Do we have to re-create the render primitives from scratch?
	bool recreateSurfaceBuffer = !_surfaceBuffer || !_surfaceBuffer->isValid(renderer);
	bool recreateCapBuffer = showCap() && (!_capBuffer || !_capBuffer->isValid(renderer));

	// Do we have to update the render primitives?
	bool updateContents = _geometryCacheHelper.updateState(color_surface, smoothShading(), clusterGraph)
					|| recreateSurfaceBuffer || recreateCapBuffer || _trimeshUpdate;

	// Re-create the render primitives if necessary.
	if(recreateSurfaceBuffer)
		_surfaceBuffer = renderer->createMeshPrimitive();
	if(recreateCapBuffer && _showCap)
		_capBuffer = renderer->createMeshPrimitive();

	// Update render primitives.
	if(updateContents) {

		// Assign smoothing group to faces to interpolate normals.
		const quint32 smoothingGroup = smoothShading() ? 1 : 0;
		for(auto& face : _surfaceMesh.faces()) {
			face.setSmoothingGroups(smoothingGroup);
		}

		// Define surface colors for the regions by taking them from the cluster graph.
		int maxClusterId = 0;
		if(clusterGraph) {
			for(Cluster* cluster : clusterGraph->storage()->clusters())
				if(cluster->id > maxClusterId)
					maxClusterId = cluster->id;
		}
		std::vector<ColorA> materialColors(maxClusterId + 1, color_surface);
		materialColors[0] = color_surface;
		if(clusterGraph) {
			for(Cluster* cluster : clusterGraph->storage()->clusters()) {
				if(cluster->id != 0)
					materialColors[cluster->id] = ColorA(cluster->color, 1.0f - transp_surface);
			}
		}
		_surfaceBuffer->setMaterialColors(std::move(materialColors));

		_surfaceBuffer->setMesh(_surfaceMesh, color_surface);
		_surfaceBuffer->setCullFaces(true);

		if(showCap())
			_capBuffer->setMesh(_capPolygonsMesh, color_surface);

		// Reset update flag.
		_trimeshUpdate = false;
	}

	// Handle picking of triangles.
	renderer->beginPickObject(contextNode);
	_surfaceBuffer->render(renderer);
	if(showCap())
		_capBuffer->render(renderer);
	else
		_capBuffer.reset();
	renderer->endPickObject();
}

/******************************************************************************
* Generates the final triangle mesh, which will be rendered.
******************************************************************************/
bool PartitionMeshDisplay::buildMesh(const PartitionMeshData& input, const SimulationCell& cell, const QVector<Plane3>& cuttingPlanes, TriMesh& output, PromiseBase& promise)
{
	// Convert half-edge mesh to triangle mesh.
	input.convertToTriMesh(output);

	// Transfer region IDs to triangle faces.
	auto fout = output.faces().begin();
	for(PartitionMeshData::Face* face : input.faces()) {
		for(PartitionMeshData::Edge* edge = face->edges()->nextFaceEdge()->nextFaceEdge(); edge != face->edges(); edge = edge->nextFaceEdge()) {
			fout->setMaterialIndex(face->region);
			++fout;
		}
	}
	OVITO_ASSERT(fout == output.faces().end());

	if(promise.isCanceled())
		return false;

	// Convert vertex positions to reduced coordinates.
	for(Point3& p : output.vertices()) {
		p = cell.absoluteToReduced(p);
		OVITO_ASSERT(std::isfinite(p.x()) && std::isfinite(p.y()) && std::isfinite(p.z()));
	}

	// Wrap mesh at periodic boundaries.
	for(size_t dim = 0; dim < 3; dim++) {
		if(cell.pbcFlags()[dim] == false) continue;

		if(promise.isCanceled())
			return false;

		// Make sure all vertices are located inside the periodic box.
		for(Point3& p : output.vertices()) {
			OVITO_ASSERT(std::isfinite(p[dim]));
			p[dim] -= floor(p[dim]);
			OVITO_ASSERT(p[dim] >= FloatType(0) && p[dim] <= FloatType(1));
		}

		// Split triangle faces at periodic boundaries.
		int oldFaceCount = output.faceCount();
		int oldVertexCount = output.vertexCount();
		std::vector<Point3> newVertices;
		std::map<std::pair<int,int>,std::pair<int,int>> newVertexLookupMap;
		for(int findex = 0; findex < oldFaceCount; findex++) {
			if(!splitFace(output, findex, oldVertexCount, newVertices, newVertexLookupMap, cell, dim))
				return false;
		}

		// Insert newly created vertices into mesh.
		output.setVertexCount(oldVertexCount + (int)newVertices.size());
		std::copy(newVertices.cbegin(), newVertices.cend(), output.vertices().begin() + oldVertexCount);
	}

	// Check for early abortion.
	if(promise.isCanceled())
		return false;

	// Convert vertex positions back from reduced coordinates to absolute coordinates.
	AffineTransformation cellMatrix = cell.matrix();
	for(Point3& p : output.vertices())
		p = cellMatrix * p;

	// Clip mesh at cutting planes.
	for(const Plane3& plane : cuttingPlanes) {
		if(promise.isCanceled())
			return false;

		output.clipAtPlane(plane);
	}

	output.invalidateVertices();
	output.invalidateFaces();

	return !promise.isCanceled();
}

/******************************************************************************
* Splits a triangle face at a periodic boundary.
******************************************************************************/
bool PartitionMeshDisplay::splitFace(TriMesh& output, int faceIndex, int oldVertexCount, std::vector<Point3>& newVertices,
		std::map<std::pair<int,int>,std::pair<int,int>>& newVertexLookupMap, const SimulationCell& cell, size_t dim)
{
	TriMeshFace& face = output.face(faceIndex);
	OVITO_ASSERT(face.vertex(0) != face.vertex(1));
	OVITO_ASSERT(face.vertex(1) != face.vertex(2));
	OVITO_ASSERT(face.vertex(2) != face.vertex(0));

	FloatType z[3];
	for(int v = 0; v < 3; v++)
		z[v] = output.vertex(face.vertex(v))[dim];
	FloatType zd[3] = { z[1] - z[0], z[2] - z[1], z[0] - z[2] };

	OVITO_ASSERT(z[1] - z[0] == -(z[0] - z[1]));
	OVITO_ASSERT(z[2] - z[1] == -(z[1] - z[2]));
	OVITO_ASSERT(z[0] - z[2] == -(z[2] - z[0]));

	if(std::abs(zd[0]) < FloatType(0.5) && std::abs(zd[1]) < FloatType(0.5) && std::abs(zd[2]) < FloatType(0.5))
		return true;	// Face is not crossing the periodic boundary.

	// Create four new vertices (or use existing ones created during splitting of adjacent faces).
	int properEdge = -1;
	int newVertexIndices[3][2];
	for(int i = 0; i < 3; i++) {
		if(std::abs(zd[i]) < FloatType(0.5)) {
			if(properEdge != -1)
				return false;		// The simulation box may be too small or invalid.
			properEdge = i;
			continue;
		}
		int vi1 = face.vertex(i);
		int vi2 = face.vertex((i+1)%3);
		int oi1, oi2;
		if(zd[i] <= FloatType(-0.5)) {
			std::swap(vi1, vi2);
			oi1 = 1; oi2 = 0;
		}
		else {
			oi1 = 0; oi2 = 1;
		}
		auto entry = newVertexLookupMap.find(std::make_pair(vi1, vi2));
		if(entry != newVertexLookupMap.end()) {
			newVertexIndices[i][oi1] = entry->second.first;
			newVertexIndices[i][oi2] = entry->second.second;
		}
		else {
			Vector3 delta = output.vertex(vi2) - output.vertex(vi1);
			delta[dim] -= FloatType(1);
			for(size_t d = dim + 1; d < 3; d++) {
				if(cell.pbcFlags()[d])
					delta[d] -= floor(delta[d] + FloatType(0.5));
			}
			FloatType t;
			if(delta[dim] != 0)
				t = output.vertex(vi1)[dim] / (-delta[dim]);
			else
				t = FloatType(0.5);
			OVITO_ASSERT(std::isfinite(t));
			Point3 p = delta * t + output.vertex(vi1);
			newVertexIndices[i][oi1] = oldVertexCount + (int)newVertices.size();
			newVertexIndices[i][oi2] = oldVertexCount + (int)newVertices.size() + 1;
			newVertexLookupMap.insert(std::make_pair(std::pair<int,int>(vi1, vi2), std::pair<int,int>(newVertexIndices[i][oi1], newVertexIndices[i][oi2])));
			newVertices.push_back(p);
			p[dim] += FloatType(1);
			newVertices.push_back(p);
		}
	}
	OVITO_ASSERT(properEdge != -1);

	// Build output triangles.
	int originalVertices[3] = { face.vertex(0), face.vertex(1), face.vertex(2) };
	face.setVertices(originalVertices[properEdge], originalVertices[(properEdge+1)%3], newVertexIndices[(properEdge+2)%3][1]);

	int materialIndex = face.materialIndex();
	output.setFaceCount(output.faceCount() + 2);
	TriMeshFace& newFace1 = output.face(output.faceCount() - 2);
	TriMeshFace& newFace2 = output.face(output.faceCount() - 1);
	newFace1.setVertices(originalVertices[(properEdge+1)%3], newVertexIndices[(properEdge+1)%3][0], newVertexIndices[(properEdge+2)%3][1]);
	newFace2.setVertices(newVertexIndices[(properEdge+1)%3][1], originalVertices[(properEdge+2)%3], newVertexIndices[(properEdge+2)%3][0]);
	newFace1.setMaterialIndex(materialIndex);
	newFace2.setMaterialIndex(materialIndex);

	return true;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
