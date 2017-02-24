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
#include <core/utilities/MemoryPool.h>
#include <plugins/crystalanalysis/data/DislocationNetwork.h>
#include "InterfaceMesh.h"

#include <boost/random/mersenne_twister.hpp>
#if BOOST_VERSION > 146000
#include <boost/random/uniform_int_distribution.hpp>
#else
#include <boost/random/uniform_int.hpp>
#endif

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/**
 * This is the central class for dislocation line tracing.
 */
class DislocationTracer
{
public:

	/// Constructor.
	DislocationTracer(InterfaceMesh& mesh, ClusterGraph* clusterGraph, int maxTrialCircuitSize, int maxCircuitElongation) :
		_mesh(mesh),
		_clusterGraph(clusterGraph),
		_network(new DislocationNetwork(clusterGraph)),
		_unusedCircuit(nullptr),
		_rng(1),
		_maxBurgersCircuitSize(maxTrialCircuitSize),
		_maxExtendedBurgersCircuitSize(maxTrialCircuitSize + maxCircuitElongation)
	{}

	/// Returns the interface mesh that separates the crystal defects from the perfect regions.
	const InterfaceMesh& mesh() const { return _mesh; }

	/// Returns a reference to the cluster graph.
	ClusterGraph& clusterGraph() { return *_clusterGraph; }

	/// Returns the extracted network of dislocation segments.
	DislocationNetwork& network() { return *_network; }

	/// Returns a const-reference to the extracted network of dislocation segments.
	const DislocationNetwork& network() const { return *_network; }

	/// Returns the simulation cell.
	const SimulationCell& cell() const { return mesh().structureAnalysis().cell(); }

	/// Performs a dislocation search on the interface mesh by generating
	/// trial Burgers circuits. Identified dislocation segments are converted to
	/// a continuous line representation
	bool traceDislocationSegments(PromiseBase& promise);

	/// After dislocation segments have been extracted, this method trims
	/// dangling lines and finds the optimal cluster to express each segment's
	/// Burgers vector.
	void finishDislocationSegments(int crystalStructure);

	/// Returns the list of nodes that are not part of a junction.
	const std::vector<DislocationNode*>& danglingNodes() const { return _danglingNodes; }

private:

	BurgersCircuit* allocateCircuit();
	void discardCircuit(BurgersCircuit* circuit);
	bool findPrimarySegments(int maxBurgersCircuitSize, PromiseBase& promise);
	bool createBurgersCircuit(InterfaceMesh::Edge* edge, int maxBurgersCircuitSize);
	void createAndTraceSegment(const ClusterVector& burgersVector, BurgersCircuit* forwardCircuit, int maxCircuitLength);
	bool intersectsOtherCircuits(BurgersCircuit* circuit);
	BurgersCircuit* buildReverseCircuit(BurgersCircuit* forwardCircuit);
	void traceSegment(DislocationSegment& segment, DislocationNode& node, int maxCircuitLength, bool isPrimarySegment);
	bool tryRemoveTwoCircuitEdges(InterfaceMesh::Edge*& edge0, InterfaceMesh::Edge*& edge1, InterfaceMesh::Edge*& edge2);
	bool tryRemoveThreeCircuitEdges(InterfaceMesh::Edge*& edge0, InterfaceMesh::Edge*& edge1, InterfaceMesh::Edge*& edge2, bool isPrimarySegment);
	bool tryRemoveOneCircuitEdge(InterfaceMesh::Edge*& edge0, InterfaceMesh::Edge*& edge1, InterfaceMesh::Edge*& edge2, bool isPrimarySegment);
	bool trySweepTwoFacets(InterfaceMesh::Edge*& edge0, InterfaceMesh::Edge*& edge1, InterfaceMesh::Edge*& edge2, bool isPrimarySegment);
	bool tryInsertOneCircuitEdge(InterfaceMesh::Edge*& edge0, InterfaceMesh::Edge*& edge1, bool isPrimarySegment);
	void appendLinePoint(DislocationNode& node);
	void circuitCircuitIntersection(InterfaceMesh::Edge* circuitAEdge1, InterfaceMesh::Edge* circuitAEdge2, InterfaceMesh::Edge* circuitBEdge1, InterfaceMesh::Edge* circuitBEdge2, int& goingOutside, int& goingInside);
	size_t joinSegments(int maxCircuitLength);
	void createSecondarySegment(InterfaceMesh::Edge* firstEdge, BurgersCircuit* outerCircuit, int maxCircuitLength);

	/// Calculates the shift vector that must be subtracted from point B to bring it close to point A such that
	/// the vector (B-A) is not a wrapped vector.
	Vector3 calculateShiftVector(const Point3& a, const Point3& b) const {
		Vector3 d = cell().absoluteToReduced(b - a);
		d.x() = cell().pbcFlags()[0] ? floor(d.x() + FloatType(0.5)) : FloatType(0);
		d.y() = cell().pbcFlags()[1] ? floor(d.y() + FloatType(0.5)) : FloatType(0);
		d.z() = cell().pbcFlags()[2] ? floor(d.z() + FloatType(0.5)) : FloatType(0);
		return cell().reducedToAbsolute(d);
	}

private:

	/// The interface mesh that separates the crystal defects from the perfect regions.
	InterfaceMesh& _mesh;

	/// The extracted network of dislocation segments.
	QExplicitlySharedDataPointer<DislocationNetwork> _network;

	/// The cluster graph.
	QExplicitlySharedDataPointer<ClusterGraph> _clusterGraph;

	/// The maximum length (number of edges) for Burgers circuits during the first tracing phase.
	int _maxBurgersCircuitSize;

	/// The maximum length (number of edges) for Burgers circuits during the second tracing phase.
	int _maxExtendedBurgersCircuitSize;

	// Used to allocate memory for BurgersCircuit instances.
	MemoryPool<BurgersCircuit> _circuitPool;

	/// List of nodes that do not form a junction.
	std::vector<DislocationNode*> _danglingNodes;

	/// Stores a pointer to the last allocated circuit which has been discarded.
	/// It can be re-used on the next allocation request.
	BurgersCircuit* _unusedCircuit;

#if BOOST_VERSION > 146000
	/// Used to generate random numbers;
	boost::random::mt19937 _rng;
#else
	/// Used to generate random numbers;
	boost::mt19937 _rng;
#endif
};

}	// End of namespace
}	// End of namespace
}	// End of namespace


