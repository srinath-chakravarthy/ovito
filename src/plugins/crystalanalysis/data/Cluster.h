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

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

struct Cluster;
struct ClusterTransition;

/// Two transitions matrices are considered equal if their elements don't differ by more than this value.
#define CA_TRANSITION_MATRIX_EPSILON				Ovito::FloatType(1e-4)


/**
 * A cluster transition T_12 is a transformation matrix that connects the
 * reference frames of two clusters 1 and 2.
 * A cluster transition also corresponds to a directed edge in the cluster graph.
 *
 * For every cluster transition T_12 there exists a reverse transition
 * T_21 = (T_12)^-1.
 *
 * If clusters 1 and 2 are adjacent in the input structure, then we can determine
 * the transition matrix T_12 from the neighboring atoms at the common border of the two
 * clusters.
 *
 * Given two cluster transitions T_12 and T_23, we can construct a third
 * cluster transition T_13 = T_23 * T_12, which connects clusters 1 and 3.
 *
 * Every cluster has a so-called self-transition (or identity transition),
 * which is the reverse of itself.
 */
struct ClusterTransition
{
	/// The first cluster.
	/// The transition matrix transforms vectors from this cluster to the coordinate system of cluster 2.
	Cluster* cluster1;

	/// The second cluster.
	/// The transition matrix transforms vectors from cluster 1 to the coordinate system of this cluster.
	Cluster* cluster2;

	/// The transformation matrix that transforms vectors from the reference frame of cluster 1 to the frame
	/// of cluster 2.
	Matrix3 tm;

	/// Pointer to the reverse transition from cluster 2 to cluster 1.
	/// The transformation matrix of the reverse transition is the inverse of this transition's matrix.
	ClusterTransition* reverse;

	/// The cluster transitions form the directed edges of the cluster cluster graph (with the clusters being the nodes).
	/// Each node's outgoing edges are stored in a linked list. This field points
	/// to the next element in the linked list of cluster 1.
	ClusterTransition* next;

	/// The distance of clusters 1 and 2 in the cluster graph.
	/// The cluster transition is of distance 1 if the two cluster are immediate
	/// neighbors (i.e. they have a common border).
	/// From two transitions A->B and B->C we can derive a new transition A->C, which
	/// is the concatenation of the first two. The distance associated with the transition
	/// A->C is the sum of distances of A->B and B->C.
	/// The distance of a self-transition A->A is defined to be zero.
	int distance;

	/// The number of bonds that are part of this cluster transition.
	int area;

	/// Returns true if this is the self-transition that connects a cluster with itself.
	/// The transformation matrix of an identity transition is always the identity matrix.
	bool isSelfTransition() const {
		OVITO_ASSERT((reverse != this) || (cluster1 == cluster2));
		OVITO_ASSERT((reverse != this) || tm.equals(Matrix3::Identity(), CA_TRANSITION_MATRIX_EPSILON));
		OVITO_ASSERT((reverse != this) || (distance == 0));
		return reverse == this;
	}

	/// Transforms a vector from the coordinate space of cluster 1 to the coordinate space of cluster 2.
	Vector3 transform(const Vector3& v) const {
		if(isSelfTransition()) return v;
		else return (tm * v);
	}

	/// Back-transforms a vector from the coordinate space of cluster 2 to the coordinate space of cluster 1.
	Vector3 reverseTransform(const Vector3& v) const {
		if(isSelfTransition()) return v;
		else return (reverse->tm * v);
	}
};

/**
 * A cluster is a connected group of atoms in the input structure that all match
 * one pattern, i.e. they form a contiguous arrangement with long-range order.
 *
 * A cluster constitutes a node in the the so-called cluster graph, which is generated
 * during the pattern matching procedure.
 *
 * Every cluster is associated with an internal frame of reference (which is implicitly defined by the
 * template structure used to create the atomic pattern). When a cluster is created for a group
 * of atoms, an average orientation matrix is calculated that transforms vectors from the cluster's reference frame to the global
 * simulation frame (in a least-square sense).
 *
 * Two clusters that are adjacent in the input structure can have a specific
 * crystallographic orientation relationship, which can be determined from the atoms
 * at their common border. Vectors given in the local coordinate frame of one of the clusters
 * can be transformed to the other cluster's coordinate space. The corresponding transformation
 * matrix is referred to as a 'cluster transition', which constitutes a directed edge in the so-called cluster graph.
 */
struct Cluster
{
	/// The identifier of the cluster.
	int id;

	/// The structural pattern formed by atoms of the cluster.
	int structure;

	/// The number of atoms that belong to the cluster.
	int atomCount = 0;

	/// Linked list of transitions from this cluster to other clusters. They form the directed edges
	/// of the cluster graph.
	///
	/// The elements in the linked list are always ordered in ascending distance order.
	/// Thus, the self-transition (having distance 0) will always be a the head of the linked list (if it has already been created).
	ClusterTransition* transitions = nullptr;

	/// This is a work variable used only during a recursive path search in the
	/// cluster graph. It points to the preceding node in the path.
	ClusterTransition* predecessor;

	union {

		/// This is a work variable used only during a recursive shortest path search in the
		/// cluster graph. It keeps track of the distance of this cluster from the
		/// start node of the path search.
		int distanceFromStart;

		/// Used by the disjoint-set forest algorithm using union-by-rank and path compression.
		int rank;
	};

	/// Transformation matrix that transforms vectors from the cluster's internal coordinate space
	/// to the global simulation frame. Note that this describes the (average) orientation of the
	/// atom group in the simulation coordinate system.
	Matrix3 orientation = Matrix3::Identity();

	/// An additional symmetry transformation applied to the orientation of this cluster to align it
	/// with one of the preferred crystal orientation as much as possible.
	int symmetryTransformation = 0;

	/// The center of mass of the cluster. This is computed from the atoms
	/// that are part of the cluster.
	Point3 centerOfMass = Point3::Origin();

	/// The visualization color of the atom cluster.
	Color color;

	/// The transition from this cluster to its parent if the cluster
	/// is a child cluster.
	ClusterTransition* parentTransition = nullptr;

	/// Constructor.
	Cluster(int _id, int _structure) : id(_id), structure(_structure), color(1,1,1) {}

	/// Inserts a transition into this cluster's list of transitions.
	void insertTransition(ClusterTransition* newTransition) {
		OVITO_ASSERT(newTransition->cluster1 == this);
		// Determine the point of insertion to keep the linked list of transitions sorted.
		ClusterTransition* appendAfter = nullptr;
		for(ClusterTransition* t = this->transitions; t != nullptr && t->distance < newTransition->distance; t = t->next)
			appendAfter = t;
		if(appendAfter) {
			newTransition->next = appendAfter->next;
			appendAfter->next = newTransition;
			OVITO_ASSERT(appendAfter->distance < newTransition->distance);
		}
		else {
			newTransition->next = this->transitions;
			this->transitions = newTransition;
		}
	}

	/// Removes a transition from the cluster's list of transitions.
	void removeTransition(ClusterTransition* t) {
		if(this->transitions == t) {
			this->transitions = t->next;
			t->next = nullptr;
			return;
		}
		for(ClusterTransition* iter = this->transitions; iter != nullptr; iter = iter->next) {
			if(iter->next == t) {
				iter->next = t->next;
				t->next = nullptr;
				return;
			}
		}
		OVITO_ASSERT(false); // Transition was not in the list. This should not happen.
	}

	/// Returns the transition to the given cluster or NULL if there is no direct transition.
	ClusterTransition* findTransition(Cluster* clusterB) const {
		for(ClusterTransition* t = transitions; t != nullptr; t = t->next) {
			if(t->cluster2 == clusterB)
				return t;
		}
		return nullptr;
	}

	/// Returns true if the given transition is in this cluster's list of transitions.
	bool hasTransition(ClusterTransition* transition) const {
		for(ClusterTransition* t = transitions; t != nullptr; t = t->next)
			if(t == transition) return true;
		return false;
	}
};

}	// End of namespace
}	// End of namespace
}	// End of namespace


