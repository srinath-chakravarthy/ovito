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
#include "ElasticMapping.h"
#include "CrystalPathFinder.h"
#include "DislocationTracer.h"
#include "DislocationAnalysisEngine.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

// List of vertices that bound the six edges of a tetrahedron.
static const int edgeVertices[6][2] = {{0,1},{0,2},{0,3},{1,2},{1,3},{2,3}};

/******************************************************************************
* Builds the list of edges in the tetrahedral tessellation.
******************************************************************************/
bool ElasticMapping::generateTessellationEdges(PromiseBase& promise)
{
	promise.setProgressValue(0);
	promise.setProgressMaximum(tessellation().numberOfPrimaryTetrahedra());

	// Generate list of tessellation edges.
	for(DelaunayTessellation::CellIterator cell = tessellation().begin_cells(); cell != tessellation().end_cells(); ++cell) {

		// Skip invalid cells (those not connecting four physical atoms) and ghost cells.
		if(tessellation().isGhostCell(cell)) continue;

		// Update progress indicator.
		if(!promise.setProgressValueIntermittent(tessellation().getCellIndex(cell)))
			return false;

		// Create edge data structure for each of the six edges of the cell.
		for(int edgeIndex = 0; edgeIndex < 6; edgeIndex++) {
			int vertex1 = tessellation().vertexIndex(tessellation().cellVertex(cell, edgeVertices[edgeIndex][0]));
			int vertex2 = tessellation().vertexIndex(tessellation().cellVertex(cell, edgeVertices[edgeIndex][1]));
			if(vertex1 == vertex2)
				continue;
			Point3 p1 = tessellation().vertexPosition(tessellation().cellVertex(cell, edgeVertices[edgeIndex][0]));
			Point3 p2 = tessellation().vertexPosition(tessellation().cellVertex(cell, edgeVertices[edgeIndex][1]));
			if(structureAnalysis().cell().isWrappedVector(p1 - p2))
				continue;
			OVITO_ASSERT(vertex1 >= 0 && vertex2 >= 0);
			TessellationEdge* edge = findEdge(vertex1, vertex2);
			if(edge == nullptr) {
				// Create a new edge.
				TessellationEdge* edge12 = _edgePool.construct(vertex1, vertex2);
				edge12->nextLeavingEdge = _vertexEdges[vertex1].first;
				_vertexEdges[vertex1].first = edge12;
				edge12->nextArrivingEdge = _vertexEdges[vertex2].second;
				_vertexEdges[vertex2].second = edge12;
				_edgeCount++;
			}
		}
	}

	return !promise.isCanceled();
}

/******************************************************************************
* Assigns each tessellation vertex to a cluster.
******************************************************************************/
bool ElasticMapping::assignVerticesToClusters(PromiseBase& promise)
{
	// Unknown runtime length.
	promise.setProgressValue(0);
	promise.setProgressMaximum(0);

	// Assign a cluster to each vertex of the tessellation, which will be used to express
	// reference vectors assigned to the edges leaving the vertex.

	// If an atoms is part of an atomic cluster, then the cluster is also assigned to the corresponding tessellation vertex.
	for(size_t atomIndex = 0; atomIndex < _vertexClusters.size(); atomIndex++) {
		_vertexClusters[atomIndex] = structureAnalysis().atomCluster(atomIndex);
	}

	// Now try to assign a cluster to those vertices of the tessellation whose corresponding atom
	// is not part of a cluster. This is performed by repeatedly copying the cluster assignment
	// from an already assigned vertex to all its unassigned neighbors.
	bool notDone;
	do {
		if(promise.isCanceled())
			return false;

		notDone = false;
		for(int vertexIndex = 0; vertexIndex < _vertexClusters.size(); vertexIndex++) {
			if(clusterOfVertex(vertexIndex)->id != 0) continue;
			for(TessellationEdge* e = _vertexEdges[vertexIndex].first; e != nullptr; e = e->nextLeavingEdge) {
				OVITO_ASSERT(e->vertex1 == vertexIndex);
				if(clusterOfVertex(e->vertex2)->id != 0) {
					_vertexClusters[vertexIndex] = _vertexClusters[e->vertex2];
					notDone = true;
					break;
				}
			}
			if(clusterOfVertex(vertexIndex)->id != 0) continue;
			for(TessellationEdge* e = _vertexEdges[vertexIndex].second; e != nullptr; e = e->nextArrivingEdge) {
				OVITO_ASSERT(e->vertex2 == vertexIndex);
				if(clusterOfVertex(e->vertex1)->id != 0) {
					_vertexClusters[vertexIndex] = _vertexClusters[e->vertex1];
					notDone = true;
					break;
				}
			}
		}
	}
	while(notDone);

	return !promise.isCanceled();
}

/******************************************************************************
* Determines the ideal vector corresponding to each edge of the tessellation.
******************************************************************************/
bool ElasticMapping::assignIdealVectorsToEdges(bool reconstructEdgeVectors, int crystalPathSteps, PromiseBase& promise)
{
	CrystalPathFinder pathFinder(_structureAnalysis, crystalPathSteps);

	// Try to assign a reference vector to the tessellation edges.
	promise.setProgressValue(0);
	promise.setProgressMaximum(_vertexEdges.size());
	int progressCounter = 0;
	for(const auto& firstEdge : _vertexEdges) {

		if(!promise.setProgressValueIntermittent(progressCounter++))
			return false;

		for(TessellationEdge* edge = firstEdge.first; edge != nullptr; edge = edge->nextLeavingEdge) {
			// Check if the reference vector of this edge has already been determined.
			if(edge->hasClusterVector()) continue;

			Cluster* cluster1 = clusterOfVertex(edge->vertex1);
			Cluster* cluster2 = clusterOfVertex(edge->vertex2);
			OVITO_ASSERT(cluster1 && cluster2);
			if(cluster1->id == 0 || cluster2->id == 0) continue;

			// Determine the ideal vector connecting the two atoms.
			boost::optional<ClusterVector> idealVector = pathFinder.findPath(edge->vertex1, edge->vertex2);
			if(!idealVector)
				continue;

			// Translate vector to the frame of the vertex cluster.
			Vector3 localVec;
			if(idealVector->cluster() == cluster1)
				localVec = idealVector->localVec();
			else {
				ClusterTransition* transition = clusterGraph().determineClusterTransition(idealVector->cluster(), cluster1);
				if(!transition)
					continue;
				localVec = transition->transform(idealVector->localVec());
			}

			// Assign the cluster transition to the edge.
			ClusterTransition* transition = clusterGraph().determineClusterTransition(cluster1, cluster2);
			// The two clusters may be part of two disconnected components of the cluster graph.
			if(!transition)
				continue;

			// Assign cluster vector to the edge.
			edge->assignClusterVector(localVec, transition);
		}
	}

#if 0
	if(reconstructEdgeVectors) {
		if(!reconstructIdealEdgeVectors(progress))
			return false;
	}
#endif

#if 0
	_unassignedEdges = new BondsStorage();
	for(const auto& firstEdge : _vertexEdges) {
		for(TessellationEdge* edge = firstEdge.first; edge != nullptr; edge = edge->nextLeavingEdge) {
			if(edge->hasClusterVector()) continue;
			_unassignedEdges->push_back({ Vector_3<int8_t>::Zero(), (unsigned int)edge->vertex1, (unsigned int)edge->vertex2 });
			_unassignedEdges->push_back({ Vector_3<int8_t>::Zero(), (unsigned int)edge->vertex2, (unsigned int)edge->vertex1 });
		}
	}
#endif

	return !promise.isCanceled();
}

/******************************************************************************
* Tries to determine the ideal vectors of tessellation edges, which haven't
* been assigned one during the first phase.
******************************************************************************/
bool ElasticMapping::reconstructIdealEdgeVectors(PromiseBase& promise)
{
#if 0
	auto reconstructRecursive = [this](std::deque<TessellationEdge*>& toBeVisited) {
		while(!toBeVisited.empty()) {
			TessellationEdge* currentEdge = toBeVisited.front();
			toBeVisited.pop_front();
			for(TessellationEdge* e1 = _vertexEdges[currentEdge->vertex1].first; e1 != nullptr; e1 = e1->nextLeavingEdge) {
				if(e1->hasClusterVector()) continue;
				if(TessellationEdge* e2 = findEdge(currentEdge->vertex2, e1->vertex2)) {
					if(e2->hasClusterVector()) {
						if(e2->vertex2 == e1->vertex2) {
							ClusterTransition* transition = clusterGraph().concatenateClusterTransitions(currentEdge->clusterTransition, e2->clusterTransition);
							OVITO_ASSERT(transition != nullptr);
							Vector3 clusterVector = currentEdge->clusterVector + currentEdge->clusterTransition->reverseTransform(e2->clusterVector);
							e1->assignClusterVector(clusterVector, transition);
							OVITO_ASSERT(e1->clusterTransition->cluster1 == clusterOfVertex(e1->vertex1));
							OVITO_ASSERT(e1->clusterTransition->cluster2 == clusterOfVertex(e2->vertex2));
							toBeVisited.push_back(e1);
						}
						else {
							ClusterTransition* transition = clusterGraph().concatenateClusterTransitions(currentEdge->clusterTransition, e2->clusterTransition->reverse);
							OVITO_ASSERT(transition != nullptr);
							Vector3 clusterVector = currentEdge->clusterVector - transition->reverseTransform(e2->clusterVector);
							e1->assignClusterVector(clusterVector, transition);
							OVITO_ASSERT(e1->clusterTransition->cluster1 == clusterOfVertex(e1->vertex1));
							OVITO_ASSERT(e1->clusterTransition->cluster2 == clusterOfVertex(e1->vertex2));
							toBeVisited.push_back(e1);
						}
					}
				}
			}
			for(TessellationEdge* e1 = _vertexEdges[currentEdge->vertex1].second; e1 != nullptr; e1 = e1->nextArrivingEdge) {
				if(e1->hasClusterVector()) continue;
				if(TessellationEdge* e2 = findEdge(currentEdge->vertex2, e1->vertex1)) {
					if(e2->hasClusterVector()) {
						if(e2->vertex1 == e1->vertex1) {
							ClusterTransition* transition = clusterGraph().concatenateClusterTransitions(e2->clusterTransition, currentEdge->clusterTransition->reverse);
							OVITO_ASSERT(transition != nullptr);
							Vector3 clusterVector = e2->clusterVector - transition->reverseTransform(currentEdge->clusterVector);
							e1->assignClusterVector(clusterVector, transition);
							OVITO_ASSERT(e1->clusterTransition->cluster1 == clusterOfVertex(e1->vertex1));
							OVITO_ASSERT(e1->clusterTransition->cluster2 == clusterOfVertex(e1->vertex2));
							toBeVisited.push_back(e1);
						}
						else {
							ClusterTransition* transition = clusterGraph().concatenateClusterTransitions(e2->clusterTransition->reverse, currentEdge->clusterTransition->reverse);
							OVITO_ASSERT(transition != nullptr);
							Vector3 clusterVector = e2->clusterTransition->reverseTransform(-e2->clusterVector) - transition->reverseTransform(currentEdge->clusterVector);
							e1->assignClusterVector(clusterVector, transition);
							OVITO_ASSERT(e1->clusterTransition->cluster1 == clusterOfVertex(e1->vertex1));
							OVITO_ASSERT(e1->clusterTransition->cluster2 == clusterOfVertex(e1->vertex2));
							toBeVisited.push_back(e1);
						}
					}
				}
			}
		}
	};

	std::deque<TessellationEdge*> toBeVisited;
	for(size_t vertexIndex = 0; vertexIndex < _vertexEdges.size(); vertexIndex++) {
		if(progress.isCanceled()) return false;
		if(clusterOfVertex(vertexIndex)->id == 0) continue;
		for(TessellationEdge* edge = _vertexEdges[vertexIndex]; edge != nullptr; edge = edge->next) {
			if(edge->hasClusterVector()) continue;
			if(clusterOfVertex(edge->vertex2)->id == 0) continue;
			for(TessellationEdge* e1 = _vertexEdges[vertexIndex]; e1 != nullptr && !edge->hasClusterVector(); e1 = e1->next) {
				if(e1->hasClusterVector() == false) continue;
				OVITO_ASSERT(e1 != edge);
				for(TessellationEdge* e2 = _vertexEdges[e1->vertex2]; e2 != nullptr; e2 = e2->next) {
					if(e2->hasClusterVector() == false) continue;
					if(e2->vertex2 == edge->vertex2) {
						OVITO_ASSERT(e1->clusterTransition->cluster2 == e2->clusterTransition->cluster1);
						ClusterTransition* transition = clusterGraph().concatenateClusterTransitions(e1->clusterTransition, e2->clusterTransition);
						OVITO_ASSERT(transition != nullptr);
						Vector3 clusterVector = e1->clusterVector + e1->clusterTransition->reverseTransform(e2->clusterVector);
						edge->assignClusterVector(clusterVector, transition);
						OVITO_ASSERT(edge->clusterTransition->cluster1 == clusterOfVertex(e1->vertex1));
						OVITO_ASSERT(edge->clusterTransition->cluster2 == clusterOfVertex(e2->vertex2));
						toBeVisited.push_back(edge);
						toBeVisited.push_back(edge->reverse);
						break;
					}
				}
			}
		}
	}
	reconstructRecursive(toBeVisited);

	for(DelaunayTessellation::CellIterator cell = tessellation().begin_cells(); cell != tessellation().end_cells(); ++cell) {
		// Skip invalid cells (those not connecting four physical atoms) and ghost cells.
		if(cell->info().isGhost) continue;
		if(progress.isCanceled()) return false;

		int nWithVector = 0;
		int nWithoutVector = 0;
		TessellationEdge* unassignedEdge = nullptr;
		for(int edgeIndex = 0; edgeIndex < 6; edgeIndex++) {
			int vertex1 = cell->vertex(edgeVertices[edgeIndex][0])->point().index();
			int vertex2 = cell->vertex(edgeVertices[edgeIndex][1])->point().index();
			TessellationEdge* edge = findEdge(vertex1, vertex2);
			if(!edge) break;
			if(edge->hasClusterVector()) nWithVector++;
			else {
				nWithoutVector++;
				unassignedEdge = edge;
			}
		}

		if(nWithVector == 3 && nWithoutVector == 3) {
			Cluster* cluster1 = clusterOfVertex(unassignedEdge->vertex1);
			Cluster* cluster2 = clusterOfVertex(unassignedEdge->vertex2);
			if(cluster1->id == 0 || cluster2->id == 0) continue;
			ClusterTransition* transition = clusterGraph().determineClusterTransition(cluster1, cluster2);
			if(!transition) continue;
			unassignedEdge->assignClusterVector(Vector3::Zero(), transition);
			toBeVisited.push_back(unassignedEdge);
			toBeVisited.push_back(unassignedEdge->reverse);
			reconstructRecursive(toBeVisited);
		}
	}
#endif
	return true;
}


/******************************************************************************
* Determines whether the elastic mapping from the physical configuration
* of the crystal to the imaginary, stress-free configuration is compatible
* within the given tessellation cell. Returns false if the mapping is incompatible
* or cannot be determined at all.
******************************************************************************/
bool ElasticMapping::isElasticMappingCompatible(DelaunayTessellation::CellHandle cell) const
{
	// Must be a valid tessellation cell to determine the mapping.
	if(!tessellation().isValidCell(cell))
		return false;

	// Retrieve the cluster vectors assigned to the six edges of the tetrahedron.
	std::pair<Vector3, ClusterTransition*> edgeVectors[6];
	for(int edgeIndex = 0; edgeIndex < 6; edgeIndex++) {
		int vertex1 = tessellation().vertexIndex(tessellation().cellVertex(cell, edgeVertices[edgeIndex][0]));
		int vertex2 = tessellation().vertexIndex(tessellation().cellVertex(cell, edgeVertices[edgeIndex][1]));
		TessellationEdge* tessEdge = findEdge(vertex1, vertex2);
		if(!tessEdge || !tessEdge->hasClusterVector())
			return false;
		if(tessEdge->vertex1 == vertex1) {
			edgeVectors[edgeIndex].first = tessEdge->clusterVector;
			edgeVectors[edgeIndex].second = tessEdge->clusterTransition;
		}
		else {
			edgeVectors[edgeIndex].first = tessEdge->clusterTransition->transform(-tessEdge->clusterVector);
			edgeVectors[edgeIndex].second = tessEdge->clusterTransition->reverse;
		}
	}

	static const int circuits[4][3] = { {0,4,2}, {1,5,2}, {0,3,1}, {3,5,4} };

	// Perform the Burgers circuit test on each of the four faces of the tetrahedron.
	for(int face = 0; face < 4; face++) {
		Vector3 burgersVector = edgeVectors[circuits[face][0]].first;
		burgersVector += edgeVectors[circuits[face][0]].second->reverseTransform(edgeVectors[circuits[face][1]].first);
		burgersVector -= edgeVectors[circuits[face][2]].first;
		if(!burgersVector.isZero(CA_LATTICE_VECTOR_EPSILON)) {
			return false;
		}
	}

	// Perform disclination test on each of the four faces.
	for(int face = 0; face < 4; face++) {
		ClusterTransition* t1 = edgeVectors[circuits[face][0]].second;
		ClusterTransition* t2 = edgeVectors[circuits[face][1]].second;
		ClusterTransition* t3 = edgeVectors[circuits[face][2]].second;
		if(!t1->isSelfTransition() || !t2->isSelfTransition() || !t3->isSelfTransition()) {
			Matrix3 frankRotation = t3->reverse->tm * t2->tm * t1->tm;
			if(!frankRotation.equals(Matrix3::Identity(), CA_TRANSITION_MATRIX_EPSILON))
				return false;
		}
	}

	return true;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
