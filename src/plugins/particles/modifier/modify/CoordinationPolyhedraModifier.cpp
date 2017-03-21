///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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

#include <plugins/particles/Particles.h>
#include <plugins/particles/objects/ParticleTypeProperty.h>
#include <plugins/particles/objects/SurfaceMesh.h>
#include <plugins/particles/objects/SurfaceMeshDisplay.h>
#include <plugins/particles/objects/SimulationCellObject.h>
#include <plugins/particles/objects/BondsObject.h>
#include "CoordinationPolyhedraModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(CoordinationPolyhedraModifier, AsynchronousParticleModifier);
DEFINE_FLAGS_REFERENCE_FIELD(CoordinationPolyhedraModifier, surfaceMeshDisplay, "SurfaceMeshDisplay", SurfaceMeshDisplay, PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);
SET_PROPERTY_FIELD_LABEL(CoordinationPolyhedraModifier, surfaceMeshDisplay, "Surface mesh display");

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
CoordinationPolyhedraModifier::CoordinationPolyhedraModifier(DataSet* dataset) : AsynchronousParticleModifier(dataset)
{
	INIT_PROPERTY_FIELD(surfaceMeshDisplay);

	// Create the display object for rendering the created polyhedra.
	_surfaceMeshDisplay = new SurfaceMeshDisplay(dataset);
	_surfaceMeshDisplay->setShowCap(false);
	_surfaceMeshDisplay->setSmoothShading(false);
	_surfaceMeshDisplay->setSurfaceTransparency(FloatType(0.25));
	_surfaceMeshDisplay->setObjectTitle(tr("Polyhedra"));
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void CoordinationPolyhedraModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	AsynchronousParticleModifier::propertyChanged(field);
}

/******************************************************************************
* Handles reference events sent by reference targets of this object.
******************************************************************************/
bool CoordinationPolyhedraModifier::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	// Do not propagate messages from the attached display object.
	if(source == surfaceMeshDisplay())
		return false;

	return AsynchronousParticleModifier::referenceEvent(source, event);
}

/******************************************************************************
* Resets the modifier's result cache.
******************************************************************************/
void CoordinationPolyhedraModifier::invalidateCachedResults()
{
	AsynchronousParticleModifier::invalidateCachedResults();

	// Reset computed mesh when the input has changed.
	_polyhedraMesh.reset();
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a pipeline.
******************************************************************************/
void CoordinationPolyhedraModifier::initializeModifier(PipelineObject* pipeline, ModifierApplication* modApp)
{
	AsynchronousParticleModifier::initializeModifier(pipeline, modApp);
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
std::shared_ptr<AsynchronousParticleModifier::ComputeEngine> CoordinationPolyhedraModifier::createEngine(TimePoint time, TimeInterval validityInterval)
{
	// Get modifier inputs.
	ParticlePropertyObject* posProperty = expectStandardProperty(ParticleProperty::PositionProperty);
	ParticlePropertyObject* typeProperty = inputStandardProperty(ParticleProperty::ParticleTypeProperty);
	ParticlePropertyObject* selectionProperty = inputStandardProperty(ParticleProperty::SelectionProperty);
	BondsObject* bondsObj = input().findObject<BondsObject>();
	SimulationCellObject* simCell = expectSimulationCell();

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<ComputePolyhedraEngine>(validityInterval, posProperty->storage(),
			selectionProperty ? selectionProperty->storage() : nullptr,
			typeProperty ? typeProperty->storage() : nullptr, 
			bondsObj ? bondsObj->storage() : nullptr, simCell->data());
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void CoordinationPolyhedraModifier::ComputePolyhedraEngine::perform()
{
	if(!_selection)
		throw Exception(tr("Please select particles first for which coordination polyhedra should be generated."));
	if(!_bonds)
		throw Exception(tr("Please create bonds between particles first. They are needed for coordination polyhedra."));

	setProgressText(tr("Generating coordination polyhedra"));

	// Determine number of selected particles. 
	size_t npoly = std::count_if(_selection->constDataInt(), _selection->constDataInt() + _selection->size(), [](int s) { return s != 0; });
	setProgressMaximum(npoly);

	std::vector<Point3> bondVectors;
	ParticleBondMap bondMap(*_bonds);

	for(size_t i = 0; i < _positions->size(); i++) {
		if(_selection->getInt(i) == 0) continue;

		// Collect the bonds that are part of the coordination polyhedron.
		const Point3& p1 = _positions->getPoint3(i);
		for(auto bondIndex : bondMap.bondsOfParticle(i)) {
			const Bond& bond = (*_bonds)[bondIndex];
			if(_positions->size() > bond.index2) {
				Vector3 delta = _positions->getPoint3(bond.index2) - p1;
				if(bond.pbcShift.x()) delta += _simCell.matrix().column(0) * (FloatType)bond.pbcShift.x();
				if(bond.pbcShift.y()) delta += _simCell.matrix().column(1) * (FloatType)bond.pbcShift.y();
				if(bond.pbcShift.z()) delta += _simCell.matrix().column(2) * (FloatType)bond.pbcShift.z();
				bondVectors.push_back(p1 + delta);
			}
		}

		// Construct the polyhedron (i.e. convex hull) from the bond vectors.
		constructConvexHull(bondVectors);
		bondVectors.clear();

		if(!incrementProgressValue())
			return;
	}
	mesh()->reindexVerticesAndFaces();
}

/******************************************************************************
* Constructs the convex hull from a set of points and adds the resulting 
* polyhedron to the mesh.
******************************************************************************/
void CoordinationPolyhedraModifier::ComputePolyhedraEngine::constructConvexHull(std::vector<Point3>& vecs)
{
	using Vertex = HalfEdgeMesh<>::Vertex;
	using Edge = HalfEdgeMesh<>::Edge;
	using Face = HalfEdgeMesh<>::Face;

	if(vecs.size() < 4) return;	// Convex hull requires at least 4 input points.

	// Keep track of how many faces and vertices we started with. 
	// We won't touch the existing mesh faces and vertices.
	int originalFaceCount = mesh()->faceCount();
	int originalVertexCount = mesh()->vertexCount();

	// Determine which points should form the initial tetrahedron.
	// Make sure they are not co-planar.
	size_t tetrahedraCorners[4];
	tetrahedraCorners[0] = 0;
	size_t n = 1;
	Matrix3 m;
	for(size_t i = 1; i < vecs.size(); i++) {
		if(n == 1) {
			m.column(0) = vecs[i] - vecs[0];
			tetrahedraCorners[1] = i;
			if(!m.column(0).isZero()) n = 2;
		}
		else if(n == 2) {
			m.column(1) = vecs[i] - vecs[0];
			tetrahedraCorners[2] = i;
			if(!m.column(0).cross(m.column(1)).isZero()) n = 3;
		}
		else if(n == 3) {
			m.column(2) = vecs[i] - vecs[0];
			FloatType det = m.determinant();
			if(std::abs(det) > FLOATTYPE_EPSILON) {
				tetrahedraCorners[3] = i;
				if(det < 0) std::swap(tetrahedraCorners[0], tetrahedraCorners[1]);
				n = 4;
				break;
			}
		}
	}
	if(n != 4) return;
	
	// Create the initial tetrahedron.
	Vertex* tetverts[4];
	for(size_t i = 0; i < 4; i++) {
		tetverts[i] = mesh()->createVertex(vecs[tetrahedraCorners[i]]);
	}
	mesh()->createFace({tetverts[0], tetverts[1], tetverts[3]});
	mesh()->createFace({tetverts[2], tetverts[0], tetverts[3]});
	mesh()->createFace({tetverts[0], tetverts[2], tetverts[1]});
	mesh()->createFace({tetverts[1], tetverts[2], tetverts[3]});
	// Connect opposite half edges.
	for(size_t i = 0; i < 4; i++) {
		for(Edge* edge = tetverts[i]->edges(); edge != nullptr; edge = edge->nextVertexEdge()) {
			if(edge->oppositeEdge()) continue;
			for(Edge* oppositeEdge = edge->vertex2()->edges(); oppositeEdge != nullptr; oppositeEdge = oppositeEdge->nextVertexEdge()) {
				if(oppositeEdge->oppositeEdge()) continue;
				if(oppositeEdge->vertex2() == tetverts[i]) {
					edge->linkToOppositeEdge(oppositeEdge);
					break;
				}
			}
			OVITO_ASSERT(edge->oppositeEdge());
		}
	}

	// Remove 4 points of initial tetrahedron from input list.
	for(size_t i = 1; i <= 4; i++)
		vecs[tetrahedraCorners[4-i]] = vecs[vecs.size()-i];
	vecs.erase(vecs.end() - 4, vecs.end());

	// Simplified Quick-hull algorithm.
	for(;;) {
		// Find the point on the positive side of a face and furthest away from it.
		// Also remove points from list which are on the negative side of all faces.
		auto furthestPoint = vecs.rend();
		FloatType furthestPointDistance = 0;
		size_t remainingPointCount = vecs.size();
		for(auto p = vecs.rbegin(); p != vecs.rend(); ++p) {
			bool insideHull = true;
			for(int faceIndex = originalFaceCount; faceIndex < mesh()->faceCount(); faceIndex++) {
				Face* face = mesh()->face(faceIndex);
				Plane3 plane(face->edges()->vertex1()->pos(),
							face->edges()->vertex2()->pos(),
							face->edges()->nextFaceEdge()->vertex2()->pos(), true);
				FloatType signedDistance = plane.pointDistance(*p);
				if(signedDistance > FLOATTYPE_EPSILON) {
					insideHull = false;
					if(signedDistance > furthestPointDistance) {
						furthestPointDistance = signedDistance;
						furthestPoint = p;
					}
				}
			}
			// When point is inside the hull, remove it from the input list.
			if(insideHull) {
				remainingPointCount--;
				*p = vecs[remainingPointCount];
			}
		}
		if(!remainingPointCount) break;
		OVITO_ASSERT(furthestPointDistance > 0 && furthestPoint != vecs.rend());

		// Kill all faces of the polyhedron that can be seen from the selected point.
		for(int faceIndex = originalFaceCount; faceIndex < mesh()->faceCount(); faceIndex++) {
			Face* face = mesh()->face(faceIndex);
			Plane3 plane(face->edges()->vertex1()->pos(),
						face->edges()->vertex2()->pos(),
						face->edges()->nextFaceEdge()->vertex2()->pos(), true);
			if(plane.pointDistance(*furthestPoint) > FLOATTYPE_EPSILON) {
				mesh()->removeFace(faceIndex);
				faceIndex--;
			}
		}

		// Find an edge that borders the newly created hole in the mesh.
		Edge* firstBorderEdge = nullptr;
		for(int faceIndex = originalFaceCount; faceIndex < mesh()->faceCount() && !firstBorderEdge; faceIndex++) {
			Face* face = mesh()->face(faceIndex);
			Edge* e = face->edges();
			OVITO_ASSERT(e != nullptr);
			do {
				if(e->oppositeEdge() == nullptr) {
					firstBorderEdge = e;
					break;
				}
				e = e->nextFaceEdge();
			}
			while(e != face->edges());
		}
		OVITO_ASSERT(firstBorderEdge != nullptr); // If this assert fails, then there was no hole in the mesh.

		// Create new faces that connects the edges at the horizon (i.e. the border of the hole) with
		// the selected vertex.
		Vertex* vertex = mesh()->createVertex(*furthestPoint);
		Edge* borderEdge = firstBorderEdge;
		Face* previousFace;
		Face* firstFace;
		Face* newFace;
		do {
			newFace = mesh()->createFace({ borderEdge->vertex2(), borderEdge->vertex1(), vertex });
			newFace->edges()->linkToOppositeEdge(borderEdge);
			if(borderEdge == firstBorderEdge)
				firstFace = newFace;
			else
				newFace->edges()->nextFaceEdge()->linkToOppositeEdge(previousFace->edges()->prevFaceEdge());
			previousFace = newFace;
			// Proceed to next edge along the hole's border.
			for(;;) {
				borderEdge = borderEdge->nextFaceEdge();
				if(borderEdge->oppositeEdge() == nullptr || borderEdge == firstBorderEdge) break;
				borderEdge = borderEdge->oppositeEdge();
			}
		}
		while(borderEdge != firstBorderEdge);
		OVITO_ASSERT(firstFace != newFace);
		firstFace->edges()->nextFaceEdge()->linkToOppositeEdge(newFace->edges()->prevFaceEdge());

		// Remove selected point from the input list as well.
		remainingPointCount--;
		*furthestPoint = vecs[remainingPointCount];
		vecs.resize(remainingPointCount);
	}

	// Delete interior vertices from the mesh that are no longer attached to any of the faces.
	for(int vertexIndex = originalVertexCount; vertexIndex < mesh()->vertexCount(); vertexIndex++) {
		if(mesh()->vertex(vertexIndex)->numEdges() == 0) {
			mesh()->removeVertex(vertexIndex);
			vertexIndex--;
		}
	}
}

/******************************************************************************
* Unpacks the results of the computation engine and stores them in the modifier.
******************************************************************************/
void CoordinationPolyhedraModifier::transferComputationResults(ComputeEngine* engine)
{
	_polyhedraMesh = static_cast<ComputePolyhedraEngine*>(engine)->mesh();
}

/******************************************************************************
* Lets the modifier insert the cached computation results into the
* modification pipeline.
******************************************************************************/
PipelineStatus CoordinationPolyhedraModifier::applyComputationResults(TimePoint time, TimeInterval& validityInterval)
{
	if(!_polyhedraMesh)
		throwException(tr("No computation results available."));

	// Create the output data object.
	OORef<SurfaceMesh> meshObj(new SurfaceMesh(dataset(), _polyhedraMesh.data()));
	meshObj->addDisplayObject(_surfaceMeshDisplay);

	// Insert output object into the pipeline.
	output().addObject(meshObj);

	return PipelineStatus(PipelineStatus::Success);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
