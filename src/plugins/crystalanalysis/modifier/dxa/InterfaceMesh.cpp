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
#include <plugins/crystalanalysis/util/ManifoldConstructionHelper.h>
#include "InterfaceMesh.h"
#include "DislocationTracer.h"
#include "DislocationAnalysisModifier.h"
#include "DislocationAnalysisEngine.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/** Find the most common element in the [first, last) range.

    O(n) in time; O(1) in space.

    [first, last) must be valid sorted range.
    Elements must be equality comparable.
*/
template <class ForwardIterator>
ForwardIterator most_common(ForwardIterator first, ForwardIterator last)
{
	ForwardIterator it(first), max_it(first);
	size_t count = 0, max_count = 0;
	for( ; first != last; ++first) {
		if(*it == *first)
			count++;
		else {
			it = first;
			count = 1;
		}
		if(count > max_count) {
			max_count = count;
			max_it = it;
		}
	}
	return max_it;
}

/******************************************************************************
* Creates the mesh facets separating good and bad tetrahedra.
******************************************************************************/
bool InterfaceMesh::createMesh(FloatType maximumNeighborDistance, ParticleProperty* crystalClusters, PromiseBase& promise)
{
	promise.beginProgressSubSteps(2);

	_isCompletelyGood = true;
	_isCompletelyBad = true;

	// Determines if a tetrahedron belongs to the good or bad crystal region.
	auto tetrahedronRegion = [this,crystalClusters](DelaunayTessellation::CellHandle cell) {
		if(elasticMapping().isElasticMappingCompatible(cell)) {
			_isCompletelyBad = false;
			if(crystalClusters) {
				std::array<int,4> clusters;
				for(int v = 0; v < 4; v++)
					clusters[v] = crystalClusters->getInt(tessellation().vertexIndex(tessellation().cellVertex(cell, v)));
				std::sort(std::begin(clusters), std::end(clusters));
				return (*most_common(std::begin(clusters), std::end(clusters)) + 1);
			}
			else return 1;
		}
		else {
			_isCompletelyGood = false;
			return 0;
		}
	};

	// Transfer cluster vectors from tessellation edges to mesh edges.
	auto prepareMeshFace = [this](Face* face, const std::array<int,3>& vertexIndices, const std::array<DelaunayTessellation::VertexHandle,3>& vertexHandles, DelaunayTessellation::CellHandle cell) {
		// Obtain unwrapped vertex positions.
		Point3 vertexPositions[3] = { tessellation().vertexPosition(vertexHandles[0]), tessellation().vertexPosition(vertexHandles[1]), tessellation().vertexPosition(vertexHandles[2]) };

		Edge* edge = face->edges();
		for(int i = 0; i < 3; i++, edge = edge->nextFaceEdge()) {
			edge->physicalVector = vertexPositions[(i+1)%3] - vertexPositions[i];

			// Check if edge is spanning more than half of a periodic simulation cell.
			for(size_t dim = 0; dim < 3; dim++) {
				if(structureAnalysis().cell().pbcFlags()[dim]) {
					if(std::abs(structureAnalysis().cell().inverseMatrix().prodrow(edge->physicalVector, dim)) >= FloatType(0.5)+FLOATTYPE_EPSILON)
						StructureAnalysis::generateCellTooSmallError(dim);
				}
			}

			// Transfer cluster vector from Delaunay edge to interface mesh edge.
			std::tie(edge->clusterVector, edge->clusterTransition) = elasticMapping().getEdgeClusterVector(vertexIndices[i], vertexIndices[(i+1)%3]);
		}
	};

	// Threshold for filtering out elements at the surface.
	double alpha = 5.0 * maximumNeighborDistance;

	ManifoldConstructionHelper<InterfaceMesh> manifoldConstructor(tessellation(), *this, alpha, structureAnalysis().positions());
	if(!manifoldConstructor.construct(tetrahedronRegion, promise, prepareMeshFace))
		return false;

	promise.nextProgressSubStep();

	// Make sure each vertex is only part of a single manifold.
	duplicateSharedVertices();

	// Validate constructed mesh.
#ifdef OVITO_DEBUG
	for(Vertex* vertex : vertices()) {
		int edgeCount = 0;
		for(Edge* edge = vertex->edges(); edge != nullptr; edge = edge->nextVertexEdge()) {
			OVITO_ASSERT(edge->oppositeEdge()->oppositeEdge() == edge);
			OVITO_ASSERT(edge->physicalVector.equals(-edge->oppositeEdge()->physicalVector, CA_ATOM_VECTOR_EPSILON));
			OVITO_ASSERT(edge->clusterTransition == edge->oppositeEdge()->clusterTransition->reverse);
			OVITO_ASSERT(edge->clusterTransition->reverse == edge->oppositeEdge()->clusterTransition);
			OVITO_ASSERT(edge->clusterVector.equals(-edge->oppositeEdge()->clusterTransition->transform(edge->oppositeEdge()->clusterVector), CA_LATTICE_VECTOR_EPSILON));
			OVITO_ASSERT(edge->nextFaceEdge()->prevFaceEdge() == edge);
			OVITO_ASSERT(edge->prevFaceEdge()->nextFaceEdge() == edge);
			OVITO_ASSERT(edge->nextFaceEdge()->nextFaceEdge() == edge->prevFaceEdge());
			OVITO_ASSERT(edge->prevFaceEdge()->prevFaceEdge() == edge->nextFaceEdge());
			edgeCount++;
		}
		OVITO_ASSERT(edgeCount == vertex->numEdges());
		OVITO_ASSERT(edgeCount >= 3);

		Edge* edge = vertex->edges();
		do {
			OVITO_ASSERT(edgeCount > 0);
			Edge* nextEdge = edge->oppositeEdge()->nextFaceEdge();
			OVITO_ASSERT(nextEdge->prevFaceEdge()->oppositeEdge() == edge);
			edge = nextEdge;
			edgeCount--;
		}
		while(edge != vertex->edges());
		OVITO_ASSERT(edgeCount == 0);
	}
#endif

	promise.endProgressSubSteps();
	return !promise.isCanceled();
}

/******************************************************************************
* Generates the nodes and facets of the defect mesh based on the interface mesh.
******************************************************************************/
bool InterfaceMesh::generateDefectMesh(const DislocationTracer& tracer, HalfEdgeMesh<>& defectMesh, PromiseBase& progress)
{
	// Copy vertices.
	defectMesh.reserveVertices(vertexCount());
	for(Vertex* v : vertices()) {
		int index = defectMesh.createVertex(v->pos())->index();
		OVITO_ASSERT(index == v->index());
	}

	// Copy faces and half-edges.
	std::vector<HalfEdgeMesh<>::Face*> faceMap(faces().size());
	auto faceMapIter = faceMap.begin();
	for(Face* face_o : faces()) {

		// Skip parts of the interface mesh that have been swept by a Burgers circuit and are
		// now part of a dislocation line.
		if(face_o->circuit != nullptr) {
			if(face_o->testFlag(1) || face_o->circuit->isDangling == false) {
				OVITO_ASSERT(*faceMapIter == nullptr);
				++faceMapIter;
				continue;
			}
		}

		HalfEdgeMesh<>::Face* face_c = defectMesh.createFace();
		*faceMapIter++ = face_c;

		if(!face_o->edges()) continue;
		Edge* edge_o = face_o->edges();
		do {
			HalfEdgeMesh<>::Vertex* v1 = defectMesh.vertex(edge_o->vertex1()->index());
			HalfEdgeMesh<>::Vertex* v2 = defectMesh.vertex(edge_o->vertex2()->index());
			defectMesh.createEdge(v1, v2, face_c);
			edge_o = edge_o->nextFaceEdge();
		}
		while(edge_o != face_o->edges());
	}

	// Link opposite half-edges.
	auto face_c = faceMap.cbegin();
	for(auto face_o = faces().cbegin(); face_o != faces().cend(); ++face_o, ++face_c) {
		if(!*face_c) continue;
		Edge* edge_o = (*face_o)->edges();
		HalfEdgeMesh<>::Edge* edge_c = (*face_c)->edges();
		if(!edge_o) continue;
		do {
			if(edge_o->oppositeEdge() != nullptr && edge_c->oppositeEdge() == nullptr) {
				HalfEdgeMesh<>::Face* oppositeFace = faceMap[edge_o->oppositeEdge()->face()->index()];
				if(oppositeFace != nullptr) {
					HalfEdgeMesh<>::Edge* oppositeEdge = oppositeFace->edges();
					do {
						OVITO_CHECK_POINTER(oppositeEdge);
						if(oppositeEdge->vertex1() == edge_c->vertex2() && oppositeEdge->vertex2() == edge_c->vertex1()) {
							edge_c->linkToOppositeEdge(oppositeEdge);
							break;
						}
						oppositeEdge = oppositeEdge->nextFaceEdge();
					}
					while(oppositeEdge != oppositeFace->edges());
					OVITO_ASSERT(edge_c->oppositeEdge());
				}
			}
			edge_o = edge_o->nextFaceEdge();
			edge_c = edge_c->nextFaceEdge();
		}
		while(edge_o != (*face_o)->edges());
	}

	// Generate cap vertices and facets to close holes left by dangling Burgers circuits.
	for(DislocationNode* dislocationNode : tracer.danglingNodes()) {
		BurgersCircuit* circuit = dislocationNode->circuit;
		OVITO_ASSERT(dislocationNode->isDangling());
		OVITO_ASSERT(circuit != nullptr);
		OVITO_ASSERT(circuit->segmentMeshCap.size() >= 2);
		OVITO_ASSERT(circuit->segmentMeshCap[0]->vertex2() == circuit->segmentMeshCap[1]->vertex1());
		OVITO_ASSERT(circuit->segmentMeshCap.back()->vertex2() == circuit->segmentMeshCap.front()->vertex1());

		HalfEdgeMesh<>::Vertex* capVertex = defectMesh.createVertex(dislocationNode->position());

		for(Edge* meshEdge : circuit->segmentMeshCap) {
			OVITO_ASSERT(faceMap[meshEdge->oppositeEdge()->face()->index()] == nullptr);
			HalfEdgeMesh<>::Vertex* v1 = defectMesh.vertices()[meshEdge->vertex2()->index()];
			HalfEdgeMesh<>::Vertex* v2 = defectMesh.vertices()[meshEdge->vertex1()->index()];
			HalfEdgeMesh<>::Face* face = defectMesh.createFace();
			defectMesh.createEdge(v1, v2, face);
			defectMesh.createEdge(v2, capVertex, face);
			defectMesh.createEdge(capVertex, v1, face);
		}
	}

	// Link dangling half edges to their opposite edges.
	if(!defectMesh.connectOppositeHalfedges()) {
		OVITO_ASSERT(false);	// Mesh is not closed.
	}

	return true;
}


}	// End of namespace
}	// End of namespace
}	// End of namespace
