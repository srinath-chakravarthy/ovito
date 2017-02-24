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

#pragma once


#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/crystalanalysis/util/DelaunayTessellation.h>
#include <core/utilities/MemoryPool.h>
#include <core/utilities/concurrent/Promise.h>
#include <plugins/crystalanalysis/data/Cluster.h>
#include <plugins/crystalanalysis/data/ClusterGraph.h>
#include <plugins/particles/data/BondsStorage.h>
#include "StructureAnalysis.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/**
 * Computes the elastic mapping from the physical configuration to a stress-free reference state.
 */
class ElasticMapping
{
private:

	/// Data structure associated with each edge of the tessellation.
	struct TessellationEdge {

		/// Constructor.
		TessellationEdge(int v1, int v2) : vertex1(v1), vertex2(v2), clusterTransition(nullptr) {}

		/// The vertex this edge is originating from.
		int vertex1;

		/// The vertex this edge is pointing to.
		int vertex2;

		/// The vector corresponding to this edge in the stress-free reference configuration.
		Vector3 clusterVector;

		/// The transition matrix when going from the cluster of vertex 1 to the cluster of vertex 2.
		ClusterTransition* clusterTransition;

		/// The next edge in the linked-list of edges leaving vertex 1.
		TessellationEdge* nextLeavingEdge;

		/// The next edge in the linked-list of edges arriving at vertex 1.
		TessellationEdge* nextArrivingEdge;

		/// Returns true if this edge has been assigned an ideal vector in the coordinate system of the local cluster.
		bool hasClusterVector() const { return clusterTransition != nullptr; }

		/// Assigns a vector to this edge.
		/// Also stores the cluster transition that connects the two clusters of the two vertices.
		void assignClusterVector(const Vector3& v, ClusterTransition* transition) {
			clusterVector = v;
			clusterTransition = transition;
		}

		/// Removes the assigned cluster vector.
		void clearClusterVector() {
			clusterTransition = nullptr;
		}
	};

public:

	/// Constructor.
	ElasticMapping(StructureAnalysis& structureAnalysis, DelaunayTessellation& tessellation) :
		_structureAnalysis(structureAnalysis),
		_tessellation(tessellation), _clusterGraph(structureAnalysis.clusterGraph()), _edgeCount(0),
		_edgePool(16384),
		_vertexEdges(structureAnalysis.atomCount(), std::pair<TessellationEdge*,TessellationEdge*>(nullptr,nullptr)),
		_vertexClusters(structureAnalysis.atomCount(), nullptr)
	{}

	/// Returns the structure analysis object.
	const StructureAnalysis& structureAnalysis() const { return _structureAnalysis; }

	/// Returns the underlying tessellation.
	DelaunayTessellation& tessellation() { return _tessellation; }

	/// Returns the underlying tessellation.
	const DelaunayTessellation& tessellation() const { return _tessellation; }

	/// Returns the cluster graph.
	const ClusterGraph& clusterGraph() const { return _clusterGraph; }

	/// Returns the cluster graph.
	ClusterGraph& clusterGraph() { return _clusterGraph; }

	/// Builds the list of edges in the tetrahedral tessellation.
	bool generateTessellationEdges(PromiseBase& promise);

	/// Assigns each tessellation vertex to a cluster.
	bool assignVerticesToClusters(PromiseBase& promise);

	/// Determines the ideal vector corresponding to each edge of the tessellation.
	bool assignIdealVectorsToEdges(bool reconstructEdgeVectors, int crystalPathSteps, PromiseBase& promise);

	/// Tries to determine the ideal vectors of tessellation edges, which haven't
	/// been assigned one during the first phase.
	bool reconstructIdealEdgeVectors(PromiseBase& promise);

	/// Determines whether the elastic mapping from the physical configuration
	/// of the crystal to the imaginary, stress-free configuration is compatible
	/// within the given tessellation cell. Returns false if the mapping is incompatible
	/// or cannot be determined.
	bool isElasticMappingCompatible(DelaunayTessellation::CellHandle cell) const;

	/// Returns the list of edges, which don't have a lattice vector.
	BondsStorage* unassignedEdges() const { return _unassignedEdges.data(); }

	/// Returns the cluster to which a vertex of the tessellation has been assigned (may be NULL).
	Cluster* clusterOfVertex(int vertexIndex) const {
		OVITO_ASSERT(vertexIndex < (int)_vertexClusters.size());
		return _vertexClusters[vertexIndex];
	}

	/// Returns the lattice vector assigned to a tessellation edge.
	std::pair<Vector3, ClusterTransition*> getEdgeClusterVector(int vertexIndex1, int vertexIndex2) const {
		TessellationEdge* tessEdge = findEdge(vertexIndex1, vertexIndex2);
		OVITO_ASSERT(tessEdge != nullptr);
		OVITO_ASSERT(tessEdge->hasClusterVector());
		if(tessEdge->vertex1 == vertexIndex1)
			return std::make_pair(tessEdge->clusterVector, tessEdge->clusterTransition);
		else
			return std::make_pair(tessEdge->clusterTransition->transform(-tessEdge->clusterVector), tessEdge->clusterTransition->reverse);
	}

private:

	/// Returns the number of tessellation edges.
	int edgeCount() const { return _edgeCount; }

	/// Looks up the tessellation edge connecting two tessellation vertices.
	/// Returns NULL if the vertices are not connected by an edge.
	TessellationEdge* findEdge(int vertexIndex1, int vertexIndex2) const {
		OVITO_ASSERT(vertexIndex1 >= 0 && vertexIndex1 < (int)_vertexEdges.size());
		OVITO_ASSERT(vertexIndex2 >= 0 && vertexIndex2 < (int)_vertexEdges.size());
		for(TessellationEdge* e = _vertexEdges[vertexIndex1].first; e != nullptr; e = e->nextLeavingEdge)
			if(e->vertex2 == vertexIndex2) return e;
		for(TessellationEdge* e = _vertexEdges[vertexIndex1].second; e != nullptr; e = e->nextArrivingEdge)
			if(e->vertex1 == vertexIndex2) return e;
		return nullptr;
	}

private:

	/// The structure analysis object.
	StructureAnalysis& _structureAnalysis;

	/// The underlying tessellation of the atomistic system.
	DelaunayTessellation& _tessellation;

	/// The cluster graph.
	ClusterGraph& _clusterGraph;

	/// Stores the heads of the linked lists of leaving/arriving edges of each vertex.
	std::vector<std::pair<TessellationEdge*,TessellationEdge*>> _vertexEdges;

	/// Memory pool for the creation of TessellationEdge structure instances.
	MemoryPool<TessellationEdge> _edgePool;

	/// Number of tessellation edges on the local processor.
	int _edgeCount;

	/// Stores the cluster assigned to each vertex atom of the tessellation.
	std::vector<Cluster*> _vertexClusters;

	/// List of edges, which don't have a lattice vector.
	QExplicitlySharedDataPointer<BondsStorage> _unassignedEdges;
};

}	// End of namespace
}	// End of namespace
}	// End of namespace


