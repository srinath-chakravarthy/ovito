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
#include "ClusterGraph.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/******************************************************************************
* Constructor. Creates an empty cluster graph.
******************************************************************************/
ClusterGraph::ClusterGraph() : _maximumClusterDistance(2)
{
	// Create the null cluster.
	createCluster(0, 0);
}

/******************************************************************************
* Copy constructor. Creates copy of an existing cluster graph.
******************************************************************************/
ClusterGraph::ClusterGraph(const ClusterGraph& other)
{
	OVITO_ASSERT(false);	// Not implemented yet.

	_maximumClusterDistance = other._maximumClusterDistance;
}

/******************************************************************************
* Inserts a new cluster into the graph.
******************************************************************************/
Cluster* ClusterGraph::createCluster(int structureType, int id)
{
	// Select a unique ID for the new cluster.
	if(id < 0) {
		id = clusters().size();
		OVITO_ASSERT(id > 0);
	}

	// Construct new Cluster instance and insert it into the list of clusters.
	Cluster* cluster = _clusterPool.construct(id, structureType);
	_clusters.push_back(cluster);

	// Register cluster in ID lookup map.
	bool isUniqueId = _clusterMap.insert(std::make_pair(id, cluster)).second;
	OVITO_ASSERT(isUniqueId);

	return cluster;
}

/******************************************************************************
* Looks up the cluster with the given ID.
* Returns NULL if the cluster with the given ID does not exist or is not
* known to this processor.
* ******************************************************************************/
Cluster* ClusterGraph::findCluster(int id) const
{
	OVITO_ASSERT(id >= 0);

	// Try fast lookup method (use ID as index).
	if(id < clusters().size() && clusters()[id]->id == id)
		return clusters()[id];

	// Use slower dictionary lookup.
	auto iter = _clusterMap.find(id);
	if(iter == _clusterMap.end())
		return nullptr;
	else {
		OVITO_ASSERT(iter->second->id == id);
		return iter->second;
	}
}

/******************************************************************************
* Registers a new cluster transition between two clusters A and B.
* This will create a new edge in the cluster graph unless an identical edge with the
* same transformation matrix already exists.
* The reverse transition B->A will also be created automatically.
******************************************************************************/
ClusterTransition* ClusterGraph::createClusterTransition(Cluster* clusterA, Cluster* clusterB, const Matrix3& tm, int distance)
{
	// Handle trivial case (the self-transition).
	if(clusterA == clusterB && tm.equals(Matrix3::Identity(), CA_TRANSITION_MATRIX_EPSILON)) {
		return createSelfTransition(clusterA);
	}
	OVITO_ASSERT(distance >= 1);

	// Look for existing transition connecting the same pair of clusters and having the same transition matrix.
	for(ClusterTransition* t = clusterA->transitions; t != nullptr; t = t->next) {
		if(t->cluster2 == clusterB && t->tm.equals(tm, CA_TRANSITION_MATRIX_EPSILON))
			return t;
	}

	// Create a new transition for the pair of clusters.
	ClusterTransition* tAB = _clusterTransitionPool.construct();
	ClusterTransition* tBA = _clusterTransitionPool.construct();
	tAB->cluster1 = clusterA;
	tAB->cluster2 = clusterB;
	tBA->cluster1 = clusterB;
	tBA->cluster2 = clusterA;
	tAB->tm = tm;
	tBA->tm = tm.inverse();
	tAB->reverse = tBA;
	tBA->reverse = tAB;
	tAB->distance = distance;
	tBA->distance = distance;
	tAB->area = 0;
	tBA->area = 0;

	// Insert the new transition and its reverse into the linked lists of the two clusters.
	clusterA->insertTransition(tAB);
	clusterB->insertTransition(tBA);

	// Register pair of new transitions in global list.
	// For this, we need to add only one of them.
	_clusterTransitions.push_back(tAB);

	// When inserting an edge that is not the concatenation of other edges, then
	// the topology of disconnected graph components may have changed.
	// This will invalidate our cache.
	if(distance == 1)
		_disconnectedClusters.clear();

	return tAB;
}

/******************************************************************************
* Creates the self-transition for a cluster (or returns the existing one).
* ******************************************************************************/
ClusterTransition* ClusterGraph::createSelfTransition(Cluster* cluster)
{
	OVITO_ASSERT(cluster != nullptr);
	OVITO_ASSERT(cluster->id != 0);

	// Check for existing self-transition.
	if(cluster->transitions != nullptr && cluster->transitions->isSelfTransition()) {
		return cluster->transitions;
	}
	else {
		// Create the self-transition.
		ClusterTransition* t = _clusterTransitionPool.construct();
		t->cluster1 = t->cluster2 = cluster;
		t->tm.setIdentity();
		t->reverse = t;
		t->distance = 0;
		t->next = cluster->transitions;
		t->area = 0;
		cluster->transitions = t;
		OVITO_ASSERT(t->isSelfTransition());
		OVITO_ASSERT(t->next == nullptr || t->next->distance >= 1);
		return t;
	}
}

/******************************************************************************
* Determines the transformation matrix that transforms vectors from cluster A to cluster B.
* For this, the cluster graph is searched for the shortest path connecting the two cluster nodes.
* If the two clusters are part of different disconnected components of the graph, then NULL is returned.
* Once a new transition between A and B has been found, it is cached by creating a new edge in the graph between
* the clusters A and B. Future queries for the same pair can then be answered efficiently.
******************************************************************************/
ClusterTransition* ClusterGraph::determineClusterTransition(Cluster* clusterA, Cluster* clusterB)
{
	OVITO_ASSERT(clusterA != nullptr && clusterB != nullptr);

	// Handle trivial case (self-transition).
	if(clusterA == clusterB)
		return createSelfTransition(clusterA);

	// Check if there is a direct transition to the target cluster.
	for(ClusterTransition* t = clusterA->transitions; t != nullptr; t = t->next) {
		// Verify that the linked list is ordered.
		OVITO_ASSERT(t->next == nullptr || t->next->distance >= t->distance);
		if(t->cluster2 == clusterB)
			return t;
	}

	// Check if either the start or the destination cluster has no transitions to other clusters.
	// Then there cannot be a path connecting them.
	if(clusterA->transitions == nullptr || (clusterA->transitions->isSelfTransition() && clusterA->transitions->next == nullptr)) {
		return nullptr;
	}
	if(clusterB->transitions == nullptr || (clusterB->transitions->isSelfTransition() && clusterB->transitions->next == nullptr)) {
		return nullptr;
	}

	// Make sure the algorithm always finds the same path, independent of whether we are searching for
	// the connection A->B or B->A.
	bool reversedSearch = false;
	if(clusterA->id > clusterB->id) {
		reversedSearch = true;
		std::swap(clusterA, clusterB);
	}

	// Check if the transition between the same pair of clusters has been requested
	// in the past and we already found that they are part of disconnected components of the graph.
	if(_disconnectedClusters.find(std::make_pair(clusterA, clusterB)) != _disconnectedClusters.end()) {
		return nullptr;
	}

	OVITO_ASSERT(_maximumClusterDistance == 2);

	// A hardcoded shortest path search for maximum depth 2:
	int shortestDistance = _maximumClusterDistance + 1;
	ClusterTransition* shortestPath1 = nullptr;
	ClusterTransition* shortestPath2;
	for(ClusterTransition* t1 = clusterA->transitions; t1 != nullptr; t1 = t1->next) {
		OVITO_ASSERT(t1->cluster2 != clusterB);
		if(t1->cluster2 == clusterA) continue;
		OVITO_ASSERT(t1->distance >= 1);
		for(ClusterTransition* t2 = t1->cluster2->transitions; t2 != nullptr; t2 = t2->next) {
			if(t2->cluster2 == clusterB) {
				OVITO_ASSERT(t2->distance >= 1);
				int distance = t1->distance + t2->distance;
				if(distance < shortestDistance) {
					shortestDistance = distance;
					shortestPath1 = t1;
					shortestPath2 = t2;
				}
				break;
			}
		}
	}
	if(shortestPath1) {
		// Create a direct transition (edge) between the two nodes in the cluster graph to speed up subsequent path queries.
		OVITO_ASSERT(shortestDistance >= 1);
		ClusterTransition* newTransition = createClusterTransition(clusterA, clusterB, shortestPath2->tm * shortestPath1->tm, shortestDistance);
		if(reversedSearch == false)
			return newTransition;
		else
			return newTransition->reverse;
	}

	// Flag this pair as disconnected to speed up subsequent queries for the same pair.
	_disconnectedClusters.insert(std::make_pair(clusterA, clusterB));

	return nullptr;
}

/******************************************************************************
* Returns the concatenation of two cluster transitions (A->B->C ->  A->C)
******************************************************************************/
ClusterTransition* ClusterGraph::concatenateClusterTransitions(ClusterTransition* tAB, ClusterTransition* tBC)
{
	OVITO_ASSERT(tAB != nullptr && tBC != nullptr);
	OVITO_ASSERT(tAB->cluster2 == tBC->cluster1);

	// Just return A->B if B->C is a self-transition (B==C).
	if(tBC->isSelfTransition())
		return tAB;

	// Just return B->C if A->B is a self-transition (A==B).
	if(tAB->isSelfTransition())
		return tBC;

	// Return A->A self-transition in case A->B->A.
	if(tAB->reverse == tBC)
		return createSelfTransition(tAB->cluster1);

	OVITO_ASSERT(tAB->distance >= 1);
	OVITO_ASSERT(tBC->distance >= 1);

	// Actually concatenate transition matrices by multiplying the transformation matrices.
	return createClusterTransition(tAB->cluster1, tBC->cluster2, tBC->tm * tAB->tm, tAB->distance + tBC->distance);
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
