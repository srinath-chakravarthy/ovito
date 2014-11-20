///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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

/**
 * \file
 * \brief Contains the definition of the Particles::TreeNeighborListBuilder class.
 */

#ifndef __OVITO_TREE_NEIGHBOR_LIST_BUILDER_H
#define __OVITO_TREE_NEIGHBOR_LIST_BUILDER_H

#include <core/Core.h>
#include <core/utilities/BoundedPriorityQueue.h>
#include <core/utilities/MemoryPool.h>
#include <plugins/particles/data/ParticleProperty.h>
#include <plugins/particles/data/SimulationCellData.h>

namespace Particles {

using namespace Ovito;

/**
 * \brief Finds the N nearest neighbors of particles.
 */
class OVITO_PARTICLES_EXPORT TreeNeighborListBuilder
{
private:

	// An internal atom structure.
	struct NeighborListAtom {
		/// The next atom in the linked list used for binning.
		NeighborListAtom* nextInBin;
		/// The wrapped position of the atom.
		Point3 pos;
	};

	struct TreeNode {
		/// Constructor for a leaf node.
		TreeNode() : splitDim(-1), atoms(nullptr), numAtoms(0) {}

		/// Returns true this is a leaf node.
		bool isLeaf() const { return splitDim == -1; }

		/// Converts the bounds of this node and all children to absolute coordinates.
		void convertToAbsoluteCoordinates(const SimulationCellData& cell) {
			bounds.minc = cell.reducedToAbsolute(bounds.minc);
			bounds.maxc = cell.reducedToAbsolute(bounds.maxc);
			if(!isLeaf()) {
				children[0]->convertToAbsoluteCoordinates(cell);
				children[1]->convertToAbsoluteCoordinates(cell);
			}
		}

		/// The splitting direction (or -1 if this is a leaf node).
		int splitDim;
		union {
			struct {
				/// The two child nodes (if this is not a leaf node).
				TreeNode* children[2];
				/// The position of the split plane.
				FloatType splitPos;
			};
			struct {
				/// The linked list of atoms (if this is a leaf node).
				NeighborListAtom* atoms;
				/// Number of atoms in this leaf node.
				int numAtoms;
			};
		};
		/// The bounding box of the node.
		Box3 bounds;
	};

public:

	//// Constructor that builds the binary search tree.
	TreeNeighborListBuilder(int _numNeighbors = 16) : numNeighbors(_numNeighbors), numLeafNodes(0), maxTreeDepth(1) {
		bucketSize = std::max(_numNeighbors / 2, 8);
	}

	/// \brief Prepares the tree data structure.
	/// \param posProperty The positions of the particles.
	/// \param cellData The simulation cell data.
	/// \return \c false when the operation has been canceled by the user;
	///         \c true on success.
	/// \throw Exception on error.
	bool prepare(ParticleProperty* posProperty, const SimulationCellData& cellData);

	/// Returns the position of the i-th particle.
	const Point3& particlePos(size_t index) const {
		OVITO_ASSERT(index >= 0 && index < atoms.size());
		return atoms[index].pos;
	}

	/// Returns the index of the particle closest to the given point.
	int findClosestParticle(const Point3& query_point, FloatType& closestDistanceSq, bool includeSelf = true) const {
		int closestIndex = -1;
		closestDistanceSq = FLOATTYPE_MAX;
		auto visitor = [&closestIndex, &closestDistanceSq](const Neighbor& n, FloatType& mrs) {
			if(n.distanceSq < closestDistanceSq) {
				mrs = closestDistanceSq = n.distanceSq;
				closestIndex = n.index;
			}
		};
		visitNeighbors(query_point, visitor, includeSelf);
		return closestIndex;
	}

	struct Neighbor
	{
		Vector3 delta;
		FloatType distanceSq;
		NeighborListAtom* atom;
		size_t index;

		/// Used for ordering.
		bool operator<(const Neighbor& other) const { return distanceSq < other.distanceSq; }
	};

	template<int MAX_NEIGHBORS_LIMIT>
	class Locator
	{
	public:

		/// Constructor.
		Locator(const TreeNeighborListBuilder& tree) : t(tree), queue(tree.numNeighbors) {}

		/// Builds the sorted list of neighbors around the given point.
		void findNeighbors(const Point3& query_point) {
			queue.clear();
			for(const Vector3& pbcShift : t.pbcImages) {
				q = query_point - pbcShift;
				if(!queue.full() || queue.top().distanceSq > t.minimumDistance(t.root, q)) {
					qr = t.simCell.absoluteToReduced(q);
					visitNode(t.root);
				}
			}
			queue.sort();
		}

		/// Returns the neighbor list.
		const BoundedPriorityQueue<Neighbor, std::less<Neighbor>, MAX_NEIGHBORS_LIMIT>& results() const { return queue; }

	private:

		/// Inserts all atoms of the given leaf node into the priority queue.
		void visitNode(TreeNode* node) {
			if(node->isLeaf()) {
				for(NeighborListAtom* atom = node->atoms; atom != nullptr; atom = atom->nextInBin) {
					Neighbor n;
					n.delta = atom->pos - q;
					n.distanceSq = n.delta.squaredLength();
					if(n.distanceSq != 0) {
						n.atom = atom;
						n.index = atom - &t.atoms.front();
						queue.insert(n);
					}
				}
			}
			else {
				TreeNode* cnear;
				TreeNode* cfar;
				if(qr[node->splitDim] < node->splitPos) {
					cnear = node->children[0];
					cfar  = node->children[1];
				}
				else {
					cnear = node->children[1];
					cfar  = node->children[0];
				}
				visitNode(cnear);
				if(!queue.full() || queue.top().distanceSq > t.minimumDistance(cfar, q))
					visitNode(cfar);
			}
		}

	private:
		const TreeNeighborListBuilder& t;
		Point3 q, qr;
		BoundedPriorityQueue<Neighbor, std::less<Neighbor>, MAX_NEIGHBORS_LIMIT> queue;
	};

	template<class Visitor>
	void visitNeighbors(const Point3& query_point, Visitor& v, bool includeSelf = false) const {
		FloatType mrs = FLOATTYPE_MAX;
		for(const Vector3& pbcShift : pbcImages) {
			Point3 q = query_point - pbcShift;
			if(mrs > minimumDistance(root, q)) {
				visitNode(root, q, simCell.absoluteToReduced(q), v, mrs, includeSelf);
			}
		}
	}

private:

	/// Inserts a particle into the binary tree.
	void insertParticle(NeighborListAtom* atom, const Point3& p, TreeNode* node, int depth);

	/// Splits a leaf node into two new leaf nodes and redistributes the atoms to the child nodes.
	void splitLeafNode(TreeNode* node, int splitDim);

	/// Determines in which direction to split the given leaf node.
	int determineSplitDirection(TreeNode* node);

	/// Computes the minimum distance from the query point to the given bounding box.
	FloatType minimumDistance(TreeNode* node, const Point3& query_point) const {
		Vector3 p1 = node->bounds.minc - query_point;
		Vector3 p2 = query_point - node->bounds.maxc;
		FloatType minDistance = 0;
		for(size_t dim = 0; dim < 3; dim++) {
			FloatType t_min = planeNormals[dim].dot(p1);
			if(t_min > minDistance) minDistance = t_min;
			FloatType t_max = planeNormals[dim].dot(p2);
			if(t_max > minDistance) minDistance = t_max;
		}
		return minDistance * minDistance;
	}

	template<class Visitor>
	void visitNode(TreeNode* node, const Point3& q, const Point3& qr, Visitor& v, FloatType& mrs, bool includeSelf) const {
		if(node->isLeaf()) {
			for(NeighborListAtom* atom = node->atoms; atom != nullptr; atom = atom->nextInBin) {
				Neighbor n;
				n.delta = atom->pos - q;
				n.distanceSq = n.delta.squaredLength();
				if(includeSelf || n.distanceSq != 0) {
					n.atom = atom;
					n.index = atom - &atoms.front();
					v(n, mrs);
				}
			}
		}
		else {
			TreeNode* cnear;
			TreeNode* cfar;
			if(qr[node->splitDim] < node->splitPos) {
				cnear = node->children[0];
				cfar  = node->children[1];
			}
			else {
				cnear = node->children[1];
				cfar  = node->children[0];
			}
			visitNode(cnear, q, qr, v, mrs, includeSelf);
			if(mrs > minimumDistance(cfar, q))
				visitNode(cfar, q, qr, v, mrs, includeSelf);
		}
	}

private:

	/// The internal list of atoms.
	std::vector<NeighborListAtom> atoms;

	// Simulation cell.
	SimulationCellData simCell;

	/// The normal vectors of the three cell planes.
	Vector3 planeNormals[3];

	/// Used to allocate instances of TreeNode.
	MemoryPool<TreeNode> nodePool;

	/// The root node of the binary tree.
	TreeNode* root;

	/// The number of neighbors to finds for each atom.
	int numNeighbors;

	/// The maximum number of particles per leaf node.
	int bucketSize;

	/// List of pbc image shift vectors.
	std::vector<Vector3> pbcImages;

public:

	/// The number of leaf nodes in the tree.
	int numLeafNodes;

	/// The maximum depth of this binary tree.
	int maxTreeDepth;
};

}; // End of namespace

#endif // __OVITO_TREE_NEIGHBOR_LIST_BUILDER_H
