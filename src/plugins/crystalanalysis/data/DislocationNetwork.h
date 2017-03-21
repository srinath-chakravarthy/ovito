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
#include <plugins/particles/data/SimulationCell.h>
#include <plugins/crystalanalysis/modifier/dxa/InterfaceMesh.h>
#include <plugins/crystalanalysis/modifier/dxa/BurgersCircuit.h>
#include "ClusterVector.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/**
 * Every dislocation segment is delimited by two dislocation nodes.
 */
struct DislocationNode
{
	/// The dislocation segment delimited by this node.
	DislocationSegment* segment;

	/// The opposite node of the dislocation segment.
	DislocationNode* oppositeNode;

	/// Pointer to the next node in linked list of nodes that form a junction.
	/// If this node is not part of a junction, then this pointer points to the node itself.
	DislocationNode* junctionRing;

	/// The Burgers circuit associated with this node.
	/// This field is only used during dislocation line tracing.
	BurgersCircuit* circuit;

	/// Constructor.
	DislocationNode() : circuit(nullptr) {
		junctionRing = this;
	}

	/// Returns the (signed) Burgers vector of the node.
	/// This is the Burgers vector of the segment if this node is a forward node,
	/// or the negative Burgers vector if this node is a backward node.
	inline ClusterVector burgersVector() const;

	/// Returns the position of the node by looking up the coordinates of the
	/// start or end point of the dislocation segment to which the node belongs.
	inline const Point3& position() const;

	/// Returns true if this node is the forward node of its segment, that is,
	/// when it is at the end of the associated dislocation segment.
	inline bool isForwardNode() const;

	/// Returns true if this node is the backward node of its segment, that is,
	/// when it is at the beginning of the associated dislocation segment.
	inline bool isBackwardNode() const;

	/// Determines whether the given node forms a junction with the given node.
	bool formsJunctionWith(DislocationNode* other) const {
		DislocationNode* n = this->junctionRing;
		do {
			if(other == n) return true;
			n = n->junctionRing;
		}
		while(n != this->junctionRing);
		return false;
	}

	/// Makes two nodes part of a junction.
	/// If any of the two nodes were already part of a junction, then
	/// a single junction is created that encompasses all nodes.
	void connectNodes(DislocationNode* other) {
		OVITO_ASSERT(!other->formsJunctionWith(this));
		OVITO_ASSERT(!this->formsJunctionWith(other));

		DislocationNode* tempStorage = junctionRing;
		junctionRing = other->junctionRing;
		other->junctionRing = tempStorage;

		OVITO_ASSERT(other->formsJunctionWith(this));
		OVITO_ASSERT(this->formsJunctionWith(other));
	}

	/// If this node is part of a junction, dissolves the junction.
	/// The nodes of all junction arms will become dangling nodes.
	void dissolveJunction() {
		DislocationNode* n = this->junctionRing;
		while(n != this) {
			DislocationNode* next = n->junctionRing;
			n->junctionRing = n;
			n = next;
		}
		this->junctionRing = this;
	}

	/// Counts the number of arms belonging to the junction.
	int countJunctionArms() const {
		int armCount = 1;
		for(DislocationNode* armNode = this->junctionRing; armNode != this; armNode = armNode->junctionRing)
			armCount++;
		return armCount;
	}

	/// Return whether the end of a segment, represented by this node, does not merge into a junction.
	bool isDangling() const {
		return (junctionRing == this);
	}
};

/**
 * A dislocation segment.
 *
 * Each segment has a Burgers vector and consists of a piecewise-linear curve in space.
 *
 * Two dislocation nodes delimit the segment.
 */
struct DislocationSegment
{
	/// The unique identifier of the dislocation segment.
	int id;

	/// The piecewise linear curve in space.
	std::deque<Point3> line;

	/// Stores the circumference of the dislocation core at every sampling point along the line.
	/// This information is used to coarsen the sampling point array adaptively since a large
	/// core size leads to a high sampling rate.
	std::deque<int> coreSize;

	/// The Burgers vector of the dislocation segment. It is expressed in the coordinate system of
	/// the crystal cluster which the segment is embedded in.
	ClusterVector burgersVector;

	/// The two nodes that delimit the segment.
	DislocationNode* nodes[2];

	/// The segment that replaces this discarded segment if the two have been merged into one segment.
	DislocationSegment* replacedWith;

	/// Constructs a new dislocation segment with the given Burgers vector
	/// and connecting the two dislocation nodes.
	DislocationSegment(const ClusterVector& b, DislocationNode* forwardNode, DislocationNode* backwardNode) :
		burgersVector(b), replacedWith(nullptr) {
		OVITO_ASSERT(b.localVec() != Vector3::Zero());
		nodes[0] = forwardNode;
		nodes[1] = backwardNode;
		forwardNode->segment = this;
		backwardNode->segment = this;
		forwardNode->oppositeNode = backwardNode;
		backwardNode->oppositeNode = forwardNode;
	}

	/// Returns the forward-pointing node at the end of the dislocation segment.
	DislocationNode& forwardNode() const { return *nodes[0]; }

	/// Returns the backward-pointing node at the start of the dislocation segment.
	DislocationNode& backwardNode() const { return *nodes[1]; }

	/// Returns true if this segment forms a closed loop, that is, when its two nodes form a single 2-junction.
	/// Note that an infinite dislocation line, passing through a periodic boundary, is also considered a loop.
	bool isClosedLoop() const {
		OVITO_ASSERT(nodes[0] && nodes[1]);
		return (nodes[0]->junctionRing == nodes[1]) && (nodes[1]->junctionRing == nodes[0]);
	}

	/// Returns true if this segment is an infinite dislocation line passing through a periodic boundary.
	/// A segment is considered infinite if it is a closed loop but its start and end points do not coincide.
	bool isInfiniteLine() const {
		return isClosedLoop() && line.back().equals(line.front(), CA_ATOM_VECTOR_EPSILON) == false;
	}

	/// Calculates the line length of the segment.
	FloatType calculateLength() const {
		OVITO_ASSERT(!isDegenerate());

		FloatType length = 0;
		auto i1 = line.begin();
		for(;;) {
			auto i2 = i1 + 1;
			if(i2 == line.end()) break;
			length += (*i1 - *i2).length();
			i1 = i2;
		}
		return length;
	}

	/// Returns true if this segment's curve consists of less than two points.
	bool isDegenerate() const { return line.size() <= 1; }

	/// Reverses the direction of the segment.
	/// This flips both the line sense and the segment's Burgers vector.
	void flipOrientation() {
		burgersVector = -burgersVector;
		std::swap(nodes[0], nodes[1]);
		std::reverse(line.begin(), line.end());
		std::reverse(coreSize.begin(), coreSize.end());
	}

	/// Computes the location of a point along the segment line.
	Point3 getPointOnLine(FloatType t) const;
};

/// Returns true if this node is the forward node of its segment, that is,
/// when it is at the end of the associated dislocation segment.
inline bool DislocationNode::isForwardNode() const
{
	return &segment->forwardNode() == this;
}

/// Returns true if this node is the backward node of its segment, that is,
/// when it is at the beginning of the associated dislocation segment.
inline bool DislocationNode::isBackwardNode() const
{
	return &segment->backwardNode() == this;
}

/// Returns the (signed) Burgers vector of the node.
/// This is the Burgers vector of the segment if this node is a forward node,
/// or the negative Burgers vector if this node is a backward node.
inline ClusterVector DislocationNode::burgersVector() const
{
	if(isForwardNode())
		return segment->burgersVector;
	else
		return -segment->burgersVector;
}

/// Returns the position of the node by looking up the coordinates of the
/// start or end point of the dislocation segment to which the node belongs.
inline const Point3& DislocationNode::position() const
{
	if(isForwardNode())
		return segment->line.back();
	else
		return segment->line.front();
}

/**
 * This class holds the entire network of dislocation segments.
 */
class DislocationNetwork : public QSharedData
{
public:

	/// Constructor that creates an empty dislocation network.
	DislocationNetwork(ClusterGraph* clusterGraph) : _clusterGraph(clusterGraph) {}

	/// Copy constructor.
	DislocationNetwork(const DislocationNetwork& other);

	/// Returns a const-reference to the cluster graph.
	const ClusterGraph& clusterGraph() const { return *_clusterGraph; }

	/// Returns the list of dislocation segments.
	const std::vector<DislocationSegment*>& segments() const { return _segments; }

	/// Allocates a new dislocation segment terminated by two nodes.
	DislocationSegment* createSegment(const ClusterVector& burgersVector);

	/// Removes a segment from the global list of segments.
	void discardSegment(DislocationSegment* segment);

private:

	/// The associated cluster graph.
	QExplicitlySharedDataPointer<ClusterGraph> _clusterGraph;

	// Used to allocate memory for DislocationNode instances.
	MemoryPool<DislocationNode> _nodePool;

	/// The list of dislocation segments.
	std::vector<DislocationSegment*> _segments;

	/// Used to allocate memory for DislocationSegment objects.
	MemoryPool<DislocationSegment> _segmentPool;
};

}	// End of namespace
}	// End of namespace
}	// End of namespace



