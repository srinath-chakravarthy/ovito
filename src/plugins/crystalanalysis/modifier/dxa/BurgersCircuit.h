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
#include <plugins/crystalanalysis/data/ClusterVector.h>
#include "InterfaceMesh.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

struct DislocationNode;		// Defined in DislocationNetwork.h

/**
 * A closed circuit on the interface mesh that consists of a sequence of mesh edges.
 *
 * During line tracing, every DislocationNode is associated with a circuit that
 * marks the beginning/end of the dislocation segment on the interface mesh.
 */
struct BurgersCircuit
{
	/// The first edge in the sequence of mesh edges.
	InterfaceMesh::Edge* firstEdge;

	/// The last edge in the sequence of mesh edges.
	InterfaceMesh::Edge* lastEdge;

	/// Saves the state of the Burgers circuit right after the primary part of dislocation segment has been traced.
	/// If the segment does not merge into a junction, then this tells us where it merges into the
	/// non-dislocation part of the interface mesh.
	std::vector<InterfaceMesh::Edge*> segmentMeshCap;

	/// Number of points in the segment's line array that are considered preliminary.
	size_t numPreliminaryPoints;

	/// The dislocation node this circuit belongs to.
	DislocationNode* dislocationNode;

	/// The number of mesh edges the circuit consists of.
	int edgeCount;

	/// Flag that indicates that all mesh edges of this Burgers circuit are blocked by other circuits.
	bool isCompletelyBlocked;

	/// Flag that indicates that this end of a segment does not merge into a junction.
	bool isDangling;

	/// Constructor.
	BurgersCircuit() : numPreliminaryPoints(0), isDangling(true) {}

	/// Calculates the Burgers vector of the dislocation enclosed by the circuit by summing up the
	/// ideal vectors of the interface mesh edges that make up the circuit.
	///
	/// Note that this method is for debugging purposes only since the Burgers vector is
	/// already known and stored in the DislocationSegment this circuit belongs to.
	ClusterVector calculateBurgersVector() const {
		Vector3 b = Vector3::Zero();
		Matrix3 tm = Matrix3::Identity();
		InterfaceMesh::Edge* edge = firstEdge;
		do {
			OVITO_ASSERT(edge != nullptr);
			b += tm * edge->clusterVector;
			if(!edge->clusterTransition->isSelfTransition()) {
				tm = tm * edge->clusterTransition->reverse->tm;
			}
			edge = edge->nextCircuitEdge;
		}
		while(edge != firstEdge);
		return ClusterVector(b, firstEdge->clusterTransition->cluster1);
	}

	/// Calculates the center of mass of the circuit.
	Point3 calculateCenter() const {
		Vector3 currentPoint = Vector3::Zero();
		Vector3 center = Vector3::Zero();
		InterfaceMesh::Edge* edge = firstEdge;
		do {
			OVITO_ASSERT(edge != nullptr);
			center += currentPoint;
			currentPoint += edge->physicalVector;
			edge = edge->nextCircuitEdge;
		}
		while(edge != firstEdge);
		return firstEdge->vertex1()->pos() + (center / edgeCount);
	}

	/// Counts the edges that form the circuit.
	/// Note that this function is for debugging purposes only since we already keep track
	/// of the number of edges with the BurgersCircuit::edgeCount variable.
	int countEdges() const {
		int count = 0;
		InterfaceMesh::Edge* edge = firstEdge;
		do {
			OVITO_ASSERT(edge != NULL);
			count++;
			edge = edge->nextCircuitEdge;
		}
		while(edge != firstEdge);
		return count;
	}

	/// Returns the i-th edge of the circuit.
	InterfaceMesh::Edge* getEdge(int index) const {
		OVITO_ASSERT(index >= 0);
		InterfaceMesh::Edge* edge = firstEdge;
		for(; index != 0; index--) {
			edge = edge->nextCircuitEdge;
		}
		return edge;
	}

	/// Saves the current state of the circuit.
	void storeCircuit() {
		OVITO_ASSERT(segmentMeshCap.empty());
		segmentMeshCap.reserve(edgeCount);
		InterfaceMesh::Edge* edge = firstEdge;
		do {
			segmentMeshCap.push_back(edge);
			edge = edge->nextCircuitEdge;
		}
		while(edge != firstEdge);
		OVITO_ASSERT(segmentMeshCap.size() >= 2);
	}
};

}	// End of namespace
}	// End of namespace
}	// End of namespace



