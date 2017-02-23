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
#include "DislocationTracer.h"
#include "InterfaceMesh.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/******************************************************************************
* Allocates a new BurgersCircuit instance.
******************************************************************************/
BurgersCircuit* DislocationTracer::allocateCircuit()
{
	if(_unusedCircuit == nullptr)
		return _circuitPool.construct();
	else {
		BurgersCircuit* circuit = _unusedCircuit;
		_unusedCircuit = nullptr;
		return circuit;
	}
}

/******************************************************************************
* Discards a previously allocated BurgersCircuit instance.
******************************************************************************/
void DislocationTracer::discardCircuit(BurgersCircuit* circuit)
{
	OVITO_ASSERT(_unusedCircuit == nullptr);
	_unusedCircuit = circuit;
}

/******************************************************************************
* Performs a dislocation search on the interface mesh by generating
* trial Burgers circuits. Identified dislocation segments are converted to
* a continuous line representation.
******************************************************************************/
bool DislocationTracer::traceDislocationSegments(PromiseBase& promise)
{
	if(_maxBurgersCircuitSize < 3 || _maxBurgersCircuitSize > _maxExtendedBurgersCircuitSize)
		throw Exception("Invalid maximum circuit size parameter(s).");

	// Set up progress indicator.
	// The estimated runtime of each sub-step scales quadratically with the Burgers circuit search depth.
	std::vector<int> subStepWeights(_maxExtendedBurgersCircuitSize - 2);
	for(int i = 0; i < subStepWeights.size(); i++)
		subStepWeights[i] = (i+3)*(i+3);
	promise.beginProgressSubSteps(subStepWeights);

	mesh().clearFaceFlag(0);

	// Incrementally extend search radius for new Burgers circuits and extend existing segments by enlarging
	// the maximum circuit size until segments meet at a junction.
	size_t numJunctions = 0;
	for(int circuitLength = 3; circuitLength <= _maxExtendedBurgersCircuitSize; circuitLength++) {

		// Extend existing segments with dangling ends.
		for(DislocationNode* node : danglingNodes()) {
			OVITO_ASSERT(node->circuit->isDangling);
			OVITO_ASSERT(node->circuit->countEdges() == node->circuit->edgeCount);

			// Trace segment a bit further.
			traceSegment(*node->segment, *node, circuitLength, circuitLength <= _maxBurgersCircuitSize);
		}

		// Find dislocation segments by generating trial Burgers circuits on the interface mesh
		// and then moving them in both directions along the dislocation segment.
		if(circuitLength <= _maxBurgersCircuitSize && (circuitLength % 2) != 0) {
			if(!findPrimarySegments(circuitLength, promise))
				return false;
		}

		// Join segments forming dislocation junctions.
		numJunctions += joinSegments(circuitLength);

		// Store circuits of dangling ends.
		if(circuitLength >= _maxBurgersCircuitSize) {
			for(DislocationNode* node : danglingNodes()) {
				OVITO_ASSERT(node->circuit->isDangling);
				OVITO_ASSERT(node->isDangling());
				if(node->circuit->segmentMeshCap.empty()) {
					node->circuit->storeCircuit();
					node->circuit->numPreliminaryPoints = 0;
				}
			}
		}

		if(circuitLength < _maxExtendedBurgersCircuitSize)
			promise.nextProgressSubStep();
	}

	//qDebug() << "Number of dislocation segments:" << network().segments().size();
	//qDebug() << "Number of dislocation junctions:" << numJunctions;

	promise.endProgressSubSteps();
	return !promise.isCanceled();
}

/******************************************************************************
* After dislocation segments have been extracted, this method trims
* dangling lines.
******************************************************************************/
void DislocationTracer::finishDislocationSegments(int crystalStructure)
{
	// Remove extra line points from segments that do not end in a junction.
	// Also assign consecutive IDs to final segments.
	for(int segmentIndex = 0; segmentIndex < network().segments().size(); segmentIndex++) {
		DislocationSegment* segment = network().segments()[segmentIndex];
		std::deque<Point3>& line = segment->line;
		std::deque<int>& coreSize = segment->coreSize;
		segment->id = segmentIndex;
		OVITO_ASSERT(coreSize.size() == line.size());
		OVITO_ASSERT(segment->backwardNode().circuit->numPreliminaryPoints + segment->forwardNode().circuit->numPreliminaryPoints <= line.size());
		line.erase(line.begin(), line.begin() + segment->backwardNode().circuit->numPreliminaryPoints);
		line.erase(line.end() - segment->forwardNode().circuit->numPreliminaryPoints, line.end());
		coreSize.erase(coreSize.begin(), coreSize.begin() + segment->backwardNode().circuit->numPreliminaryPoints);
		coreSize.erase(coreSize.end() - segment->forwardNode().circuit->numPreliminaryPoints, coreSize.end());
	}

	// Express Burgers vectors of dislocations in a proper lattice frame whenever possible.
	for(DislocationSegment* segment : network().segments()) {
		Cluster* originalCluster = segment->burgersVector.cluster();
		if(originalCluster->structure != crystalStructure) {
			for(ClusterTransition* t = originalCluster->transitions; t != nullptr && t->distance <= 1; t = t->next) {
				if(t->cluster2->structure == crystalStructure) {
					segment->burgersVector = ClusterVector(t->transform(segment->burgersVector.localVec()), t->cluster2);
					break;
				}
			}
		}
	}

	// Align dislocations.
	for(DislocationSegment* segment : network().segments()) {
		std::deque<Point3>& line = segment->line;
		OVITO_ASSERT(line.size() >= 2);

		Vector3 dir = line.back() - line.front();
		if(dir.isZero(CA_ATOM_VECTOR_EPSILON))
			continue;

		if(std::abs(dir.x()) > std::abs(dir.y())) {
			if(std::abs(dir.x()) > std::abs(dir.z())) {
				if(dir.x() >= 0.0) continue;
			}
			else {
				if(dir.z() >= 0.0) continue;
			}
		}
		else {
			if(std::abs(dir.y()) > std::abs(dir.z())) {
				if(dir.y() >= 0.0) continue;
			}
			else {
				if(dir.z() >= 0.0) continue;
			}
		}

		segment->flipOrientation();
	}
}

/**
 * This data structure is used for the recursive generation of
 * trial Burgers circuits on the interface mesh.
 */
struct BurgersCircuitSearchStruct
{
	/// The current mesh node.
	InterfaceMesh::Vertex* node;

	/// The coordinates of this node in the unstrained reference crystal it was mapped to.
	Point3 latticeCoord;

	/// The matrix that transforms local lattice vectors to the reference frame of the start node.
	Matrix3 tm;

	/// Number of steps between this node and the start node of the recursive walk.
	int recursiveDepth;

	/// The previous edge in the path to this node.
	InterfaceMesh::Edge* predecessorEdge;

	/// Linked list pointer.
	BurgersCircuitSearchStruct* nextToProcess;
};

/******************************************************************************
* Generates all possible trial circuits on the interface mesh until it finds
* one with a non-zero Burgers vector.
* Then moves the Burgers circuit in both directions along the dislocation
* segment until the maximum circuit size has been reached.
******************************************************************************/
bool DislocationTracer::findPrimarySegments(int maxBurgersCircuitSize, PromiseBase& promise)
{
	int searchDepth =  (maxBurgersCircuitSize - 1) / 2;
	OVITO_ASSERT(searchDepth >= 1);

	MemoryPool<BurgersCircuitSearchStruct> structPool;

	promise.setProgressValue(0);
	promise.setProgressMaximum(mesh().vertexCount());
	int progressCounter = 0;

	// Find an appropriate start node for the recursive search.
	for(InterfaceMesh::Vertex* startNode : mesh().vertices()) {
		OVITO_ASSERT(startNode->edges() != nullptr);
		OVITO_ASSERT(startNode->burgersSearchStruct == nullptr);

		// Update progress indicator.
		if(!promise.setProgressValueIntermittent(progressCounter++))
			return false;

		// The first node is the seed of our recursive walk.
		// It is mapped to the origin of the reference lattice.
		BurgersCircuitSearchStruct* start = structPool.construct();
		start->latticeCoord = Point3::Origin();
		start->predecessorEdge = nullptr;
		start->recursiveDepth = 0;
		start->nextToProcess = nullptr;
		start->tm.setIdentity();
		start->node = startNode;
		startNode->burgersSearchStruct = start;

		// This is the cluster we work in.
		OVITO_ASSERT(startNode->edges()->clusterTransition);
		Cluster* cluster = startNode->edges()->clusterTransition->cluster1;
		OVITO_ASSERT(cluster && cluster->id != 0);

		bool foundBurgersCircuit = false;
		BurgersCircuitSearchStruct* end_of_queue = start;

		// Process nodes from the queue until it becomes empty or until a valid Burgers circuit has been found.
		for(BurgersCircuitSearchStruct* current = start; current != nullptr && foundBurgersCircuit == false; current = current->nextToProcess) {
			InterfaceMesh::Vertex* currentNode = current->node;
			for(InterfaceMesh::Edge* edge = currentNode->edges(); edge != nullptr; edge = edge->nextVertexEdge()) {

				OVITO_ASSERT((edge->circuit == nullptr) == (edge->nextCircuitEdge == nullptr));
				OVITO_ASSERT((edge->oppositeEdge()->circuit == nullptr) == (edge->oppositeEdge()->nextCircuitEdge == nullptr));
				OVITO_ASSERT(edge->face() != nullptr);

				// Skip edges which are, or have already been, part of a Burgers circuit.
				if(edge->nextCircuitEdge != nullptr || edge->oppositeEdge()->nextCircuitEdge != nullptr)
					continue;

				// Skip edges that border an existing Burgers circuit.
				if(edge->face()->circuit != nullptr)
					continue;

				// Get the neighbor node.
				InterfaceMesh::Vertex* neighbor = edge->vertex2();

				// Calculate reference lattice coordinates of the neighboring vertex.
				Point3 neighborCoord = current->latticeCoord;
				neighborCoord += current->tm * edge->clusterVector;

				// If this neighbor has been assigned reference lattice coordinates before,
				// then perform the Burgers circuit test now by comparing the previous to the new coordinates.
				BurgersCircuitSearchStruct* neighborStruct = neighbor->burgersSearchStruct;
				if(neighborStruct != nullptr) {

					// Compute Burgers vector of the current circuit.
					Vector3 burgersVector = neighborStruct->latticeCoord - neighborCoord;
					if(burgersVector.isZero(CA_LATTICE_VECTOR_EPSILON) == false) {
						// Found circuit with non-zero Burgers vector.
						// Check if circuit encloses disclination.
						Matrix3 frankRotation = current->tm * edge->clusterTransition->reverse->tm;
						if(frankRotation.equals(neighborStruct->tm, CA_TRANSITION_MATRIX_EPSILON)) {
							// Stop as soon as a valid Burgers circuit has been found.
							if(createBurgersCircuit(edge, maxBurgersCircuitSize)) {
								foundBurgersCircuit = true;
								break;
							}
						}
					}
				}
				else if(current->recursiveDepth < searchDepth) {
					// This neighbor has not been visited before. Put it at the end of the queue.
					neighborStruct = structPool.construct();
					neighborStruct->node = neighbor;
					neighborStruct->latticeCoord = neighborCoord;
					neighborStruct->predecessorEdge = edge;
					neighborStruct->recursiveDepth = current->recursiveDepth + 1;
					if(edge->clusterTransition->isSelfTransition())
						neighborStruct->tm = current->tm;
					else
						neighborStruct->tm = current->tm * edge->clusterTransition->reverse->tm;
					neighborStruct->nextToProcess = nullptr;
					neighbor->burgersSearchStruct = neighborStruct;
					OVITO_ASSERT(end_of_queue->nextToProcess == nullptr);
					end_of_queue->nextToProcess = neighborStruct;
					end_of_queue = neighborStruct;
				}
			}
		}

		// Clear the pointers of the nodes that have been visited during the last pass.
		for(BurgersCircuitSearchStruct* s = start; s != nullptr; s = s->nextToProcess) {
			s->node->burgersSearchStruct = nullptr;
			s->node->visited = false;
		}
		structPool.clear(true);
	}

	return !promise.isCanceled();
}

/******************************************************************************
* Creates a dislocation segment and a pair of Burgers circuits.
******************************************************************************/
bool DislocationTracer::createBurgersCircuit(InterfaceMesh::Edge* edge, int maxBurgersCircuitSize)
{
	OVITO_ASSERT(edge->circuit == nullptr);

	InterfaceMesh::Vertex* currentNode = edge->vertex1();
	InterfaceMesh::Vertex* neighborNode = edge->vertex2();
	BurgersCircuitSearchStruct* currentStruct = currentNode->burgersSearchStruct;
	BurgersCircuitSearchStruct* neighborStruct = neighborNode->burgersSearchStruct;
	OVITO_ASSERT(currentStruct != neighborStruct);

	// Reconstruct the Burgers circuit from the path we took along the mesh edges.
	BurgersCircuit* forwardCircuit = allocateCircuit();
	forwardCircuit->edgeCount = 1;
	forwardCircuit->firstEdge = forwardCircuit->lastEdge = edge->oppositeEdge();
	OVITO_ASSERT(forwardCircuit->firstEdge->circuit == nullptr);
	forwardCircuit->firstEdge->circuit = forwardCircuit;

	// Clear flags of nodes on the second branch of the recursive walk.
	for(BurgersCircuitSearchStruct* a = neighborStruct; ; a = a->predecessorEdge->vertex1()->burgersSearchStruct) {
		a->node->visited = false;
		if(a->predecessorEdge == nullptr) break;
		OVITO_ASSERT(a->predecessorEdge->circuit == nullptr);
		OVITO_ASSERT(a->predecessorEdge->oppositeEdge()->circuit == nullptr);
	}

	// Mark all nodes on the first branch of the recursive walk.
	for(BurgersCircuitSearchStruct* a = currentStruct; ; a = a->predecessorEdge->vertex1()->burgersSearchStruct) {
		a->node->visited = true;
		if(a->predecessorEdge == nullptr) break;
		OVITO_ASSERT(a->predecessorEdge->circuit == nullptr);
		OVITO_ASSERT(a->predecessorEdge->oppositeEdge()->circuit == nullptr);
	}

	// Then walk on the second branch again until we hit the first branch.
	for(BurgersCircuitSearchStruct* a = neighborStruct; ; a = a->predecessorEdge->vertex1()->burgersSearchStruct) {
		if(a->node->visited) {
			a->node->visited = false;
			break;
		}
		OVITO_ASSERT(a->predecessorEdge != nullptr);
		OVITO_ASSERT(a->predecessorEdge->circuit == nullptr);
		OVITO_ASSERT(a->predecessorEdge->oppositeEdge()->circuit == nullptr);
		// Insert edge into the circuit.
		a->predecessorEdge->nextCircuitEdge = forwardCircuit->firstEdge;
		forwardCircuit->firstEdge = a->predecessorEdge;
		forwardCircuit->edgeCount++;
		forwardCircuit->firstEdge->circuit = forwardCircuit;
	}

	// Walk along the first branch again until the second branch is hit.
	for(BurgersCircuitSearchStruct* a = currentStruct; a->node->visited == true; a = a->predecessorEdge->vertex1()->burgersSearchStruct) {
		OVITO_ASSERT(a->predecessorEdge != nullptr);
		OVITO_ASSERT(a->predecessorEdge->circuit == nullptr);
		OVITO_ASSERT(a->predecessorEdge->oppositeEdge()->circuit == nullptr);
		// Insert edge into the circuit.
		forwardCircuit->lastEdge->nextCircuitEdge = a->predecessorEdge->oppositeEdge();
		forwardCircuit->lastEdge = forwardCircuit->lastEdge->nextCircuitEdge;
		forwardCircuit->edgeCount++;
		forwardCircuit->lastEdge->circuit = forwardCircuit;
		a->node->visited = false;
	}

	// Close circuit.
	forwardCircuit->lastEdge->nextCircuitEdge = forwardCircuit->firstEdge;
	OVITO_ASSERT(forwardCircuit->firstEdge != forwardCircuit->firstEdge->nextCircuitEdge);
	OVITO_ASSERT(forwardCircuit->countEdges() == forwardCircuit->edgeCount);
	OVITO_ASSERT(forwardCircuit->edgeCount >= 3);

	// Make sure the circuit is not infinite, spanning periodic boundaries.
	// This can be checked by summing up the atom-to-atom vectors of the circuit's edges.
	// The sum should be zero for valid closed circuits.
	InterfaceMesh::Edge* e = forwardCircuit->firstEdge;
	Vector3 edgeSum = Vector3::Zero();
	Matrix3 frankRotation = Matrix3::Identity();
	Vector3 b = Vector3::Zero();
	do {
		edgeSum += e->physicalVector;
		b += frankRotation * e->clusterVector;
		if(e->clusterTransition->isSelfTransition() == false)
			frankRotation = frankRotation * e->clusterTransition->reverse->tm;
		e = e->nextCircuitEdge;
	}
	while(e != forwardCircuit->firstEdge);
	OVITO_ASSERT(frankRotation.equals(Matrix3::Identity(), CA_TRANSITION_MATRIX_EPSILON));

	// Make sure new circuit does not intersect other circuits.
	bool intersects = intersectsOtherCircuits(forwardCircuit);
	if(b.isZero(CA_LATTICE_VECTOR_EPSILON) || edgeSum.isZero(CA_ATOM_VECTOR_EPSILON) == false || intersects) {
		// Reset edges.
		InterfaceMesh::Edge* e = forwardCircuit->firstEdge;
		do {
			InterfaceMesh::Edge* nextEdge = e->nextCircuitEdge;
			OVITO_ASSERT(e->circuit == forwardCircuit);
			e->nextCircuitEdge = nullptr;
			e->circuit = nullptr;
			e = nextEdge;
		}
		while(e != forwardCircuit->firstEdge);

#ifdef OVITO_DEBUG
		for(BurgersCircuitSearchStruct* a = neighborStruct; a->predecessorEdge != nullptr; a = a->predecessorEdge->vertex1()->burgersSearchStruct) {
			OVITO_ASSERT(a->predecessorEdge->circuit == nullptr);
			OVITO_ASSERT(a->predecessorEdge->oppositeEdge()->circuit == nullptr);
		}
		for(BurgersCircuitSearchStruct* a = currentStruct; a->predecessorEdge != nullptr; a = a->predecessorEdge->vertex1()->burgersSearchStruct) {
			OVITO_ASSERT(a->predecessorEdge->circuit == nullptr);
			OVITO_ASSERT(a->predecessorEdge->oppositeEdge()->circuit == nullptr);
		}
		OVITO_ASSERT(edge->circuit == nullptr);
		OVITO_ASSERT(edge->oppositeEdge()->circuit == nullptr);
#endif

		discardCircuit(forwardCircuit);
		return intersects;
	}

	OVITO_ASSERT(!forwardCircuit->calculateBurgersVector().localVec().isZero(CA_LATTICE_VECTOR_EPSILON));
	OVITO_ASSERT(!b.isZero(CA_LATTICE_VECTOR_EPSILON));
	createAndTraceSegment(ClusterVector(b, forwardCircuit->firstEdge->clusterTransition->cluster1), forwardCircuit, maxBurgersCircuitSize);

	return true;
}

/******************************************************************************
* Creates a reverse Burgers circuit, allocates a new DislocationSegment,
* and traces it in both directions.
******************************************************************************/
void DislocationTracer::createAndTraceSegment(const ClusterVector& burgersVector, BurgersCircuit* forwardCircuit, int maxCircuitLength)
{
	// Generate the reverse circuit.
	BurgersCircuit* backwardCircuit = buildReverseCircuit(forwardCircuit);

	// Create new dislocation segment.
	DislocationSegment* segment = network().createSegment(burgersVector);
	segment->forwardNode().circuit = forwardCircuit;
	segment->backwardNode().circuit = backwardCircuit;
	forwardCircuit->dislocationNode = &segment->forwardNode();
	backwardCircuit->dislocationNode = &segment->backwardNode();
	_danglingNodes.push_back(&segment->forwardNode());
	_danglingNodes.push_back(&segment->backwardNode());

	// Add the first point to the line.
	segment->line.push_back(backwardCircuit->calculateCenter());
	segment->coreSize.push_back(backwardCircuit->countEdges());
	// Add a second point to the line.
	appendLinePoint(segment->forwardNode());

	// Trace the segment in the forward direction.
	traceSegment(*segment, segment->forwardNode(), maxCircuitLength, true);

	// Trace the segment in the backward direction.
	traceSegment(*segment, segment->backwardNode(), maxCircuitLength, true);
}

/******************************************************************************
* Tests whether the given circuit intersection any other existing circuit.
******************************************************************************/
bool DislocationTracer::intersectsOtherCircuits(BurgersCircuit* circuit)
{
	InterfaceMesh::Edge* edge1 = circuit->firstEdge;
	do {
		InterfaceMesh::Edge* edge2 = edge1->nextCircuitEdge;
		if(edge1 != edge2->oppositeEdge()) {
			InterfaceMesh::Edge* currentEdge = edge1->oppositeEdge();
			do {
				InterfaceMesh::Edge* nextEdge = currentEdge->prevFaceEdge();
				OVITO_ASSERT(nextEdge != edge2);
				if(nextEdge != edge2 && nextEdge->circuit != nullptr) {
					OVITO_ASSERT(nextEdge->circuit == nextEdge->nextCircuitEdge->circuit);
					int goingOutside = 0, goingInside = 0;
					circuitCircuitIntersection(edge2->oppositeEdge(), edge1->oppositeEdge(), nextEdge, nextEdge->nextCircuitEdge, goingOutside, goingInside);
					OVITO_ASSERT(goingInside == 0);
					if(goingOutside)
						return true;
				}
				currentEdge = nextEdge->oppositeEdge();
			}
			while(currentEdge != edge2);
		}
		edge1 = edge2;
	}
	while(edge1 != circuit->firstEdge);
	return false;
}

/******************************************************************************
* Given some Burgers circuit, this function generates a reverse circuit.
******************************************************************************/
BurgersCircuit* DislocationTracer::buildReverseCircuit(BurgersCircuit* forwardCircuit)
{
	BurgersCircuit* backwardCircuit = allocateCircuit();

	// Build the backward circuit along inner outline.
	backwardCircuit->edgeCount = 0;
	backwardCircuit->firstEdge = nullptr;
	backwardCircuit->lastEdge = nullptr;
	InterfaceMesh::Edge* edge1 = forwardCircuit->firstEdge;
	do {
		InterfaceMesh::Edge* edge2 = edge1->nextCircuitEdge;
		InterfaceMesh::Edge* oppositeEdge1 = edge1->oppositeEdge();
		InterfaceMesh::Edge* oppositeEdge2 = edge2->oppositeEdge();
		InterfaceMesh::Face* facet1 = oppositeEdge1->face();
		InterfaceMesh::Face* facet2 = oppositeEdge2->face();
		OVITO_ASSERT(facet1 != nullptr && facet2 != nullptr);
		OVITO_ASSERT(facet1->circuit == nullptr || facet1->circuit == backwardCircuit);
		OVITO_ASSERT(facet2->circuit == nullptr || facet2->circuit == backwardCircuit);
		OVITO_ASSERT(edge1->vertex2() == edge2->vertex1());
		OVITO_ASSERT((edge1->clusterVector + oppositeEdge1->clusterTransition->tm * oppositeEdge1->clusterVector).isZero(CA_LATTICE_VECTOR_EPSILON));
		OVITO_ASSERT((edge2->clusterVector + oppositeEdge2->clusterTransition->tm * oppositeEdge2->clusterVector).isZero(CA_LATTICE_VECTOR_EPSILON));

		if(facet1 != facet2) {
			InterfaceMesh::Edge* outerEdge1 = oppositeEdge1->nextFaceEdge()->oppositeEdge();
			InterfaceMesh::Edge* innerEdge1 = oppositeEdge1->prevFaceEdge()->oppositeEdge();
			InterfaceMesh::Edge* outerEdge2 = oppositeEdge2->prevFaceEdge()->oppositeEdge();
			InterfaceMesh::Edge* innerEdge2 = oppositeEdge2->nextFaceEdge()->oppositeEdge();
			OVITO_ASSERT(innerEdge1 != nullptr && innerEdge2 != nullptr);
			OVITO_ASSERT(innerEdge1->vertex1() == edge1->vertex2());
			OVITO_ASSERT(innerEdge2->vertex2() == edge1->vertex2());
			OVITO_ASSERT(innerEdge1->vertex1() == innerEdge2->vertex2());
			OVITO_ASSERT(innerEdge1->circuit == nullptr || innerEdge1->circuit == backwardCircuit);
			OVITO_ASSERT(innerEdge2->circuit == nullptr || innerEdge2->circuit == backwardCircuit);
			facet1->setFlag(1);
			facet1->circuit = backwardCircuit;
			facet2->setFlag(1);
			facet2->circuit = backwardCircuit;
			innerEdge1->circuit = backwardCircuit;
			innerEdge2->circuit = backwardCircuit;
			innerEdge2->nextCircuitEdge = innerEdge1;
			if(backwardCircuit->lastEdge == nullptr) {
				OVITO_ASSERT(backwardCircuit->firstEdge == nullptr);
				OVITO_ASSERT(innerEdge1->nextCircuitEdge == nullptr);
				backwardCircuit->lastEdge = innerEdge1;
				backwardCircuit->firstEdge = innerEdge2;
				backwardCircuit->edgeCount += 2;
			}
			else if(backwardCircuit->lastEdge != innerEdge2) {
				if(innerEdge1 != backwardCircuit->firstEdge) {
					innerEdge1->nextCircuitEdge = backwardCircuit->firstEdge;
					backwardCircuit->edgeCount += 2;
				}
				else {
					backwardCircuit->edgeCount += 1;
				}
				backwardCircuit->firstEdge = innerEdge2;
			}
			else if(backwardCircuit->firstEdge != innerEdge1) {
				innerEdge1->nextCircuitEdge = backwardCircuit->firstEdge;
				backwardCircuit->firstEdge = innerEdge1;
				backwardCircuit->edgeCount += 1;
			}
			OVITO_ASSERT(innerEdge1->vertex1() != innerEdge1->vertex2());
			OVITO_ASSERT(innerEdge2->vertex1() != innerEdge2->vertex2());
		}

		edge1 = edge2;
	}
	while(edge1 != forwardCircuit->firstEdge);
	OVITO_ASSERT(backwardCircuit->lastEdge->vertex2() == backwardCircuit->firstEdge->vertex1());
	OVITO_ASSERT(backwardCircuit->lastEdge->nextCircuitEdge == nullptr || backwardCircuit->lastEdge->nextCircuitEdge == backwardCircuit->firstEdge);

	// Close circuit.
	backwardCircuit->lastEdge->nextCircuitEdge = backwardCircuit->firstEdge;

	OVITO_ASSERT(backwardCircuit->firstEdge != backwardCircuit->firstEdge->nextCircuitEdge);
	OVITO_ASSERT(backwardCircuit->countEdges() == backwardCircuit->edgeCount);
	OVITO_ASSERT(backwardCircuit->edgeCount >= 3);
	OVITO_ASSERT(!backwardCircuit->calculateBurgersVector().localVec().isZero(CA_LATTICE_VECTOR_EPSILON));

	return backwardCircuit;
}

/******************************************************************************
* Traces a dislocation segment in one direction by advancing the Burgers circuit.
******************************************************************************/
void DislocationTracer::traceSegment(DislocationSegment& segment, DislocationNode& node, int maxCircuitLength, bool isPrimarySegment)
{
	BurgersCircuit& circuit = *node.circuit;
	OVITO_ASSERT(circuit.countEdges() == circuit.edgeCount);
	OVITO_ASSERT(circuit.isDangling);

	// Advance circuit as far as possible.
	for(;;) {

		// During each iteration, first shorten circuit as much as possible.
		// Pick a random start edge to distribute the removal of edges
		// over the whole circuit.
		int edgeIndex = boost::random::uniform_int_distribution<>(0, circuit.edgeCount - 1)(_rng);
		InterfaceMesh::Edge* firstEdge = circuit.getEdge(edgeIndex);

		InterfaceMesh::Edge* edge0 = firstEdge;
		InterfaceMesh::Edge* edge1 = edge0->nextCircuitEdge;
		InterfaceMesh::Edge* edge2 = edge1->nextCircuitEdge;
		OVITO_ASSERT(edge1->circuit == &circuit);
		int counter = 0;
		do {
			// Check Burgers circuit.
			OVITO_ASSERT(circuit.edgeCount >= 3);
			OVITO_ASSERT(!circuit.calculateBurgersVector().localVec().isZero(CA_LATTICE_VECTOR_EPSILON));
			OVITO_ASSERT(circuit.countEdges() == circuit.edgeCount);
			OVITO_ASSERT(edge0->circuit == &circuit && edge1->circuit == &circuit && edge2->circuit == &circuit);

			bool wasShortened = false;
			if(tryRemoveTwoCircuitEdges(edge0, edge1, edge2))
				wasShortened = true;
			else if(tryRemoveThreeCircuitEdges(edge0, edge1, edge2, isPrimarySegment))
				wasShortened = true;
			else if(tryRemoveOneCircuitEdge(edge0, edge1, edge2, isPrimarySegment))
				wasShortened = true;
			else if(trySweepTwoFacets(edge0, edge1, edge2, isPrimarySegment))
				wasShortened = true;

			if(wasShortened) {
				appendLinePoint(node);
				counter = -1;
			}

			edge0 = edge1;
			edge1 = edge2;
			edge2 = edge2->nextCircuitEdge;
			counter++;
		}
		while(counter <= circuit.edgeCount);
		OVITO_ASSERT(circuit.edgeCount >= 3);
		OVITO_ASSERT(circuit.countEdges() == circuit.edgeCount);

		if(circuit.edgeCount >= maxCircuitLength)
			break;

		// In the second step, extend circuit by inserting an edge if possible.
		bool wasExtended = false;

		// Pick a random start edge to distribute the insertion of new edges
		// over the whole circuit.
		edgeIndex = boost::random::uniform_int_distribution<>(0, circuit.edgeCount - 1)(_rng);
		firstEdge = circuit.getEdge(edgeIndex);

		edge0 = firstEdge;
		edge1 = firstEdge->nextCircuitEdge;
		do {
			if(tryInsertOneCircuitEdge(edge0, edge1, isPrimarySegment)) {
				wasExtended = true;
				appendLinePoint(node);
				break;
			}

			edge0 = edge1;
			edge1 = edge1->nextCircuitEdge;
		}
		while(edge0 != firstEdge);
		if(!wasExtended) break;
	}
}

/******************************************************************************
* Eliminates two edges from a Burgers circuit if they are opposite halfedges.
******************************************************************************/
bool DislocationTracer::tryRemoveTwoCircuitEdges(InterfaceMesh::Edge*& edge0, InterfaceMesh::Edge*& edge1, InterfaceMesh::Edge*& edge2)
{
	if(edge1 != edge2->oppositeEdge())
		return false;

	BurgersCircuit* circuit = edge0->circuit;
	OVITO_ASSERT(circuit->edgeCount >= 4);
	edge0->nextCircuitEdge = edge2->nextCircuitEdge;
	if(edge0 == circuit->lastEdge) {
		circuit->firstEdge = circuit->lastEdge->nextCircuitEdge;
	}
	else if(edge1 == circuit->lastEdge) {
		circuit->lastEdge = edge0;
		circuit->firstEdge = edge0->nextCircuitEdge;
	}
	else if(edge2 == circuit->lastEdge) {
		circuit->lastEdge = edge0;
	}
	circuit->edgeCount -= 2;
	OVITO_ASSERT(circuit->edgeCount >= 0);

	edge1 = edge0->nextCircuitEdge;
	edge2 = edge1->nextCircuitEdge;
	return true;
}

/******************************************************************************
* Eliminates three edges from a Burgers circuit if they border a triangle.
******************************************************************************/
bool DislocationTracer::tryRemoveThreeCircuitEdges(InterfaceMesh::Edge*& edge0, InterfaceMesh::Edge*& edge1, InterfaceMesh::Edge*& edge2, bool isPrimarySegment)
{
	InterfaceMesh::Face* facet1 = edge1->face();
	InterfaceMesh::Face* facet2 = edge2->face();

	if(facet2 != facet1 || facet1->circuit != nullptr)
		return false;

	BurgersCircuit* circuit = edge0->circuit;
	OVITO_ASSERT(circuit->edgeCount > 2);
	InterfaceMesh::Edge* edge3 = edge2->nextCircuitEdge;

	if(edge3->face() != facet1) return false;
	OVITO_ASSERT(circuit->edgeCount > 4);

	edge0->nextCircuitEdge = edge3->nextCircuitEdge;

	if(edge2 == circuit->firstEdge || edge3 == circuit->firstEdge) {
		circuit->firstEdge = edge3->nextCircuitEdge;
		circuit->lastEdge = edge0;
	}
	else if(edge1 == circuit->firstEdge) {
		circuit->firstEdge = edge3->nextCircuitEdge;
		OVITO_ASSERT(circuit->lastEdge == edge0);
	}
	else if(edge3 == circuit->lastEdge) {
		circuit->lastEdge = edge0;
	}
	circuit->edgeCount -= 3;
	edge1 = edge3->nextCircuitEdge;
	edge2 = edge1->nextCircuitEdge;

	facet1->circuit = circuit;
	if(isPrimarySegment)
		facet1->setFlag(1);

	return true;
}

/******************************************************************************
* Eliminates one edge from a Burgers circuit by replacing two edges with one.
******************************************************************************/
bool DislocationTracer::tryRemoveOneCircuitEdge(InterfaceMesh::Edge*& edge0, InterfaceMesh::Edge*& edge1, InterfaceMesh::Edge*& edge2, bool isPrimarySegment)
{
	InterfaceMesh::Face* facet1 = edge1->face();
	InterfaceMesh::Face* facet2 = edge2->face();
	if(facet2 != facet1 || facet1->circuit != nullptr) return false;

	BurgersCircuit* circuit = edge0->circuit;
	OVITO_ASSERT(circuit->edgeCount > 2);

	if(edge0->face() == facet1) return false;

	InterfaceMesh::Edge* shortEdge = edge1->prevFaceEdge()->oppositeEdge();
	OVITO_ASSERT(shortEdge->vertex1() == edge1->vertex1());
	OVITO_ASSERT(shortEdge->vertex2() == edge2->vertex2());

	if(shortEdge->circuit != nullptr)
		return false;

	OVITO_ASSERT(shortEdge->nextCircuitEdge == nullptr);
	shortEdge->nextCircuitEdge = edge2->nextCircuitEdge;
	OVITO_ASSERT(shortEdge != edge2->nextCircuitEdge->oppositeEdge());
	OVITO_ASSERT(shortEdge != edge0->oppositeEdge());
	edge0->nextCircuitEdge = shortEdge;
	if(edge0 == circuit->lastEdge) {
		OVITO_ASSERT(circuit->lastEdge != edge2);
		OVITO_ASSERT(circuit->firstEdge == edge1);
		OVITO_ASSERT(shortEdge != circuit->lastEdge->oppositeEdge());
		circuit->firstEdge = shortEdge;
	}

	if(edge2 == circuit->lastEdge) {
		circuit->lastEdge = shortEdge;
	}
	else if(edge2 == circuit->firstEdge) {
		circuit->firstEdge = shortEdge->nextCircuitEdge;
		circuit->lastEdge = shortEdge;
	}
	circuit->edgeCount -= 1;
	edge1 = shortEdge;
	edge2 = shortEdge->nextCircuitEdge;
	shortEdge->circuit = circuit;

	facet1->circuit = circuit;
	if(isPrimarySegment)
		facet1->setFlag(1);

	return true;
}

/******************************************************************************
* Advances a Burgers circuit by skipping two facets.
******************************************************************************/
bool DislocationTracer::trySweepTwoFacets(InterfaceMesh::Edge*& edge0, InterfaceMesh::Edge*& edge1, InterfaceMesh::Edge*& edge2, bool isPrimarySegment)
{
	InterfaceMesh::Face* facet1 = edge1->face();
	InterfaceMesh::Face* facet2 = edge2->face();

	if(facet1->circuit != nullptr || facet2->circuit != nullptr) return false;

	BurgersCircuit* circuit = edge0->circuit;
	if(facet1 == facet2 || circuit->edgeCount <= 2) return false;

	InterfaceMesh::Edge* outerEdge1 = edge1->prevFaceEdge()->oppositeEdge();
	InterfaceMesh::Edge* innerEdge1 = edge1->nextFaceEdge();
	InterfaceMesh::Edge* outerEdge2 = edge2->nextFaceEdge()->oppositeEdge();
	InterfaceMesh::Edge* innerEdge2 = edge2->prevFaceEdge();

	if(innerEdge1 != innerEdge2->oppositeEdge() || outerEdge1->circuit != nullptr || outerEdge2->circuit != nullptr)
		return false;

	OVITO_ASSERT(outerEdge1->nextCircuitEdge == nullptr);
	OVITO_ASSERT(outerEdge2->nextCircuitEdge == nullptr);
	outerEdge1->nextCircuitEdge = outerEdge2;
	outerEdge2->nextCircuitEdge = edge2->nextCircuitEdge;
	edge0->nextCircuitEdge = outerEdge1;
	if(edge0 == circuit->lastEdge) {
		circuit->firstEdge = outerEdge1;
	}
	else if(edge1 == circuit->lastEdge) {
		circuit->lastEdge = outerEdge1;
		circuit->firstEdge = outerEdge2;
	}
	else if(edge2 == circuit->lastEdge) {
		circuit->lastEdge = outerEdge2;
	}
	outerEdge1->circuit = circuit;
	outerEdge2->circuit = circuit;

	facet1->circuit = circuit;
	facet2->circuit = circuit;
	if(isPrimarySegment) {
		facet1->setFlag(1);
		facet2->setFlag(1);
	}

	edge0 = outerEdge1;
	edge1 = outerEdge2;
	edge2 = edge1->nextCircuitEdge;

	return true;
}

/******************************************************************************
* Advances a Burgers circuit by skipping one facet and inserting an
* additional edge.
******************************************************************************/
bool DislocationTracer::tryInsertOneCircuitEdge(InterfaceMesh::Edge*& edge0, InterfaceMesh::Edge*& edge1, bool isPrimarySegment)
{
	OVITO_ASSERT(edge0 != edge1->oppositeEdge());

	InterfaceMesh::Face* facet = edge1->face();
	if(facet->circuit != nullptr)
		return false;

	InterfaceMesh::Edge* insertEdge1 = edge1->prevFaceEdge()->oppositeEdge();
	if(insertEdge1->circuit != nullptr)
		return false;

	InterfaceMesh::Edge* insertEdge2 = edge1->nextFaceEdge()->oppositeEdge();
	if(insertEdge2->circuit != nullptr)
		return false;

	OVITO_ASSERT(insertEdge1->nextCircuitEdge == nullptr);
	OVITO_ASSERT(insertEdge2->nextCircuitEdge == nullptr);
	BurgersCircuit* circuit = edge0->circuit;
	insertEdge1->nextCircuitEdge = insertEdge2;
	insertEdge2->nextCircuitEdge = edge1->nextCircuitEdge;
	edge0->nextCircuitEdge = insertEdge1;
	if(edge0 == circuit->lastEdge) {
		circuit->firstEdge = insertEdge1;
	}
	else if(edge1 == circuit->lastEdge) {
		circuit->lastEdge = insertEdge2;
	}
	insertEdge1->circuit = circuit;
	insertEdge2->circuit = circuit;
	circuit->edgeCount++;

	// Check Burgers circuit.
	OVITO_ASSERT(circuit->countEdges() == circuit->edgeCount);

	facet->circuit = circuit;
	if(isPrimarySegment)
		facet->setFlag(1);

	return true;
}

/******************************************************************************
* Appends another point to the curve at one end of a dislocation segment.
******************************************************************************/
void DislocationTracer::appendLinePoint(DislocationNode& node)
{
	DislocationSegment& segment = *node.segment;
	OVITO_ASSERT(!segment.line.empty());

	// Get size of dislocation core.
	int coreSize = node.circuit->edgeCount;

	// Make sure the line is not wrapped at periodic boundaries.
	const Point3& lastPoint = node.isForwardNode() ? segment.line.back() : segment.line.front();
	Point3 newPoint = lastPoint + cell().wrapVector(node.circuit->calculateCenter() - lastPoint);

	if(node.isForwardNode()) {
		// Add a new point to end the line.
		segment.line.push_back(newPoint);
		segment.coreSize.push_back(coreSize);
	}
	else {
		// Add a new point to start the line.
		segment.line.push_front(newPoint);
		segment.coreSize.push_front(coreSize);
	}
	node.circuit->numPreliminaryPoints++;
}

/******************************************************************************
* Determines whether two Burgers circuits intersect.
******************************************************************************/
void DislocationTracer::circuitCircuitIntersection(InterfaceMesh::Edge* circuitAEdge1, InterfaceMesh::Edge* circuitAEdge2, InterfaceMesh::Edge* circuitBEdge1, InterfaceMesh::Edge* circuitBEdge2, int& goingOutside, int& goingInside)
{
	OVITO_ASSERT(circuitAEdge2->vertex1() == circuitBEdge2->vertex1());
	OVITO_ASSERT(circuitAEdge1->vertex2() == circuitBEdge2->vertex1());
	OVITO_ASSERT(circuitBEdge1->vertex2() == circuitBEdge2->vertex1());

	// Iterate over interior facet edges.
	InterfaceMesh::Edge* edge = circuitBEdge2;
	bool contour1inside = false;
	bool contour2inside = false;
	for(;;) {
		InterfaceMesh::Edge* oppositeEdge = edge->oppositeEdge();
		if(oppositeEdge == circuitBEdge1) break;
		if(edge != circuitBEdge2) {
			if(oppositeEdge == circuitAEdge1) contour1inside = true;
			if(edge == circuitAEdge2) contour2inside = true;
		}
		edge = oppositeEdge->nextFaceEdge();
		OVITO_ASSERT(edge->vertex1() == circuitBEdge2->vertex1());
		OVITO_ASSERT(edge != circuitBEdge2);
	}
	OVITO_ASSERT(circuitAEdge2 != circuitBEdge2 || contour2inside == false);

	// Iterate over exterior facet edges.
	bool contour1outside = false;
	bool contour2outside = false;
	edge = circuitBEdge1;
	for(;;) {
		InterfaceMesh::Edge* nextEdge = edge->nextFaceEdge();
		if(nextEdge == circuitBEdge2) break;
		InterfaceMesh::Edge* oppositeEdge = nextEdge->oppositeEdge();
		OVITO_ASSERT(oppositeEdge->vertex2() == circuitBEdge2->vertex1());
		edge = oppositeEdge;
		if(edge == circuitAEdge1) contour1outside = true;
		if(nextEdge == circuitAEdge2) contour2outside = true;
	}

	OVITO_ASSERT(contour1outside == false || contour1inside == false);
	OVITO_ASSERT(contour2outside == false || contour2inside == false);

	if(contour2outside == true && contour1outside == false) {
		goingOutside += 1;
	}
	else if(contour2inside == true && contour1inside == false) {
		goingInside += 1;
	}
}

/******************************************************************************
* Look for dislocation segments whose circuits touch each other.
******************************************************************************/
size_t DislocationTracer::joinSegments(int maxCircuitLength)
{
	// First iteration over all dangling circuits.
	// Try to create secondary dislocation segments in the adjacent regions of the
	// interface mesh.
	for(size_t nodeIndex = 0; nodeIndex < danglingNodes().size(); nodeIndex++) {
		DislocationNode* node = danglingNodes()[nodeIndex];
		BurgersCircuit* circuit = node->circuit;
		OVITO_ASSERT(circuit->isDangling);

		// Go around the circuit to find an unvisited region on the interface mesh.
		InterfaceMesh::Edge* edge = circuit->firstEdge;
		do {
			OVITO_ASSERT(edge->circuit == circuit);
			BurgersCircuit* oppositeCircuit = edge->oppositeEdge()->circuit;
			if(oppositeCircuit == nullptr) {
				OVITO_ASSERT(edge->oppositeEdge()->nextCircuitEdge == nullptr);

				// Try to create a new circuit inside the unvisited region.
				createSecondarySegment(edge, circuit, maxCircuitLength);

				// Skip edges to the end of the unvisited interval.
				while(edge->oppositeEdge()->circuit == nullptr && edge != circuit->firstEdge)
					edge = edge->nextCircuitEdge;
			}
			else edge = edge->nextCircuitEdge;
		}
		while(edge != circuit->firstEdge);
	}

	// Second pass over all dangling nodes.
	// Mark circuits that are completely blocked by other circuits.
	// They are candidates for the formation of junctions.
	for(DislocationNode* node : danglingNodes()) {
		BurgersCircuit* circuit = node->circuit;
		OVITO_ASSERT(circuit->isDangling);

		// Go around the circuit to see whether it is completely surrounded by other circuits.
		// Put it into one ring with the adjacent circuits.
		circuit->isCompletelyBlocked = true;
		InterfaceMesh::Edge* edge = circuit->firstEdge;
		do {
			OVITO_ASSERT(edge->circuit == circuit);
			BurgersCircuit* adjacentCircuit = edge->oppositeEdge()->circuit;
			if(adjacentCircuit == nullptr) {
				// Found a section of the circuit, which is not blocked
				// by some other circuit.
				circuit->isCompletelyBlocked = false;
				break;
			}
			else if(adjacentCircuit != circuit) {
				OVITO_ASSERT(adjacentCircuit->isDangling);
				DislocationNode* adjacentNode = adjacentCircuit->dislocationNode;
				if(node->formsJunctionWith(adjacentNode) == false) {
					node->connectNodes(adjacentNode);
				}
			}
			edge = edge->nextCircuitEdge;
		}
		while(edge != circuit->firstEdge);
	}

	// Count number of created dislocation junctions.
	size_t numJunctions = 0;

	// Actually create junctions for completely blocked circuits.
	for(DislocationNode* node : danglingNodes()) {
		BurgersCircuit* circuit = node->circuit;

		// Skip circuits which have already become part of a junction.
		if(circuit->isDangling == false) continue;
		// Skip dangling circuits, which are not completely blocked by other circuits;
		if(!circuit->isCompletelyBlocked) {
			node->dissolveJunction();
			continue;
		}
		// Junctions must consist of at least two dislocation segments.
		if(node->junctionRing == node) continue;

		OVITO_ASSERT(node->segment->replacedWith == nullptr);

		// Compute center of mass of junction node.
		Vector3 centerOfMassVector = Vector3::Zero();
		Point3 basePoint = node->position();
		int armCount = 1;
		bool allCircuitsCompletelyBlocked = true;
		DislocationNode* armNode = node->junctionRing;
		while(armNode != node) {
			OVITO_ASSERT(armNode->segment->replacedWith == nullptr);
			OVITO_ASSERT(armNode->circuit->isDangling);
			if(armNode->circuit->isCompletelyBlocked == false) {
				allCircuitsCompletelyBlocked = false;
				break;
			}
			armCount++;
			centerOfMassVector += cell().wrapVector(armNode->position() - basePoint);
			armNode = armNode->junctionRing;
		}

		// All circuits of the junction must be fully blocked by other circuits.
		if(!allCircuitsCompletelyBlocked) {
			node->dissolveJunction();
			continue;
		}

		// Junctions must consist of at least two dislocation segments.
		OVITO_ASSERT(armCount >= 2);

		// Only create a real junction for three or more segments.
		if(armCount >= 3) {
			Point3 centerOfMass = basePoint + centerOfMassVector / armCount;

			// Iterate over all arms of the new junction.
			armNode = node;
			do {
				// Mark this node as no longer dangling.
				armNode->circuit->isDangling = false;
				OVITO_ASSERT(armNode != armNode->junctionRing);

				// Extend arm to junction's exact center point.
				std::deque<Point3>& line = armNode->segment->line;
				if(armNode->isForwardNode()) {
					line.push_back(line.back() + cell().wrapVector(centerOfMass - line.back()));
					armNode->segment->coreSize.push_back(armNode->segment->coreSize.back());
				}
				else {
					line.push_front(line.front() + cell().wrapVector(centerOfMass - line.front()));
					armNode->segment->coreSize.push_front(armNode->segment->coreSize.front());
				}
				armNode->circuit->numPreliminaryPoints = 0;
				armNode = armNode->junctionRing;
			}
			while(armNode != node);
			numJunctions++;
		}
		else {
			// For a two-armed junction, just merge the two segments into one.
			DislocationNode* node1 = node;
			DislocationNode* node2 = node->junctionRing;
			OVITO_ASSERT(node1 != node2);
			OVITO_ASSERT(node2->junctionRing == node1);
			OVITO_ASSERT(node1->junctionRing == node2);

			BurgersCircuit* circuit1 = node1->circuit;
			BurgersCircuit* circuit2 = node2->circuit;
			circuit1->isDangling = false;
			circuit2->isDangling = false;
			circuit1->numPreliminaryPoints = 0;
			circuit2->numPreliminaryPoints = 0;

			// Check if this is a closed dislocation loop.
			if(node1->oppositeNode == node2) {
				OVITO_ASSERT(node1->segment == node2->segment);
				DislocationSegment* loop = node1->segment;
				OVITO_ASSERT(loop->isClosedLoop());

				// Make both ends of the segment coincide by adding an extra point if necessary.
				if(!cell().wrapVector(node1->position() - node2->position()).isZero(CA_ATOM_VECTOR_EPSILON)) {
					loop->line.push_back(loop->line.back() + cell().wrapVector(loop->line.front() - loop->line.back()));
					OVITO_ASSERT(cell().wrapVector(node1->position() - node2->position()).isZero(CA_ATOM_VECTOR_EPSILON));
					loop->coreSize.push_back(loop->coreSize.back());
				}

				// Loop segment should not be degenerate.
				OVITO_ASSERT(loop->line.size() >= 3);
			}
			else {
				// If not a closed loop, merge the two segments into a single line.

				OVITO_ASSERT(node1->segment != node2->segment);

				DislocationNode* farEnd1 = node1->oppositeNode;
				DislocationNode* farEnd2 = node2->oppositeNode;
				DislocationSegment* segment1 = node1->segment;
				DislocationSegment* segment2 = node2->segment;
				if(node1->isBackwardNode()) {
					segment1->nodes[1] = farEnd2;
					Vector3 shiftVector;
					if(node2->isBackwardNode()) {
						shiftVector = calculateShiftVector(segment1->line.front(), segment2->line.front());
						segment1->line.insert(segment1->line.begin(), segment2->line.rbegin(), segment2->line.rend() - 1);
						segment1->coreSize.insert(segment1->coreSize.begin(), segment2->coreSize.rbegin(), segment2->coreSize.rend() - 1);
					}
					else {
						shiftVector = calculateShiftVector(segment1->line.front(), segment2->line.back());
						segment1->line.insert(segment1->line.begin(), segment2->line.begin(), segment2->line.end() - 1);
						segment1->coreSize.insert(segment1->coreSize.begin(), segment2->coreSize.begin(), segment2->coreSize.end() - 1);
					}
					if(shiftVector != Vector3::Zero()) {
						for(auto p = segment1->line.begin(); p != segment1->line.begin() + segment2->line.size() - 1; ++p)
							*p -= shiftVector;
					}
				}
				else {
					segment1->nodes[0] = farEnd2;
					Vector3 shiftVector;
					if(node2->isBackwardNode()) {
						shiftVector = calculateShiftVector(segment1->line.back(), segment2->line.front());
						segment1->line.insert(segment1->line.end(), segment2->line.begin() + 1, segment2->line.end());
						segment1->coreSize.insert(segment1->coreSize.end(), segment2->coreSize.begin() + 1, segment2->coreSize.end());
					}
					else {
						shiftVector = calculateShiftVector(segment1->line.back(), segment2->line.back());
						segment1->line.insert(segment1->line.end(), segment2->line.rbegin() + 1, segment2->line.rend());
						segment1->coreSize.insert(segment1->coreSize.end(), segment2->coreSize.rbegin() + 1, segment2->coreSize.rend());
					}
					if(shiftVector != Vector3::Zero()) {
						for(auto p = segment1->line.end() - segment2->line.size() + 1; p != segment1->line.end(); ++p)
							*p -= shiftVector;
					}
				}
				farEnd2->segment = segment1;
				farEnd2->oppositeNode = farEnd1;
				farEnd1->oppositeNode = farEnd2;
				node1->oppositeNode = node2;
				node2->oppositeNode = node1;
				segment2->replacedWith = segment1;
				network().discardSegment(segment2);
			}
		}
	}

	// Clean up list of dangling nodes. Remove joined nodes.
	_danglingNodes.erase(std::remove_if(_danglingNodes.begin(), _danglingNodes.end(),
			[](DislocationNode* node) { return !node->isDangling(); }), _danglingNodes.end());

	return numJunctions;
}

/******************************************************************************
* Creates a new dislocation segment at an incomplete junction.
******************************************************************************/
void DislocationTracer::createSecondarySegment(InterfaceMesh::Edge* firstEdge, BurgersCircuit* outerCircuit, int maxCircuitLength)
{
	OVITO_ASSERT(firstEdge->circuit == outerCircuit);

	// Create circuit along the border of the hole.
	int edgeCount = 1;
	Vector3 burgersVector = Vector3::Zero();
	Vector3 edgeSum = Vector3::Zero();
	Cluster* baseCluster = nullptr;
	Matrix3 frankRotation = Matrix3::Identity();
	int numCircuits = 1;
	InterfaceMesh::Edge* circuitStart = firstEdge->oppositeEdge();
	InterfaceMesh::Edge* circuitEnd = circuitStart;
	InterfaceMesh::Edge* edge = circuitStart;
	for(;;) {
		for(;;) {
			OVITO_ASSERT(edge->circuit == nullptr);
			InterfaceMesh::Edge* oppositeEdge = edge->oppositeEdge();
			InterfaceMesh::Face* oppositeFacet = oppositeEdge->face();
			InterfaceMesh::Edge* nextEdge = oppositeEdge->prevFaceEdge();
			OVITO_ASSERT(nextEdge->vertex2() == oppositeEdge->vertex1());
			OVITO_ASSERT(nextEdge->vertex2() == edge->vertex2());
			if(nextEdge->circuit != nullptr) {
				if(nextEdge->circuit != outerCircuit) {
					outerCircuit = nextEdge->circuit;
					numCircuits++;
				}
				edge = nextEdge->oppositeEdge();
				break;
			}
			edge = nextEdge;
		}

		circuitEnd->nextCircuitEdge = edge;
		edgeSum += edge->physicalVector;
		burgersVector += frankRotation * edge->clusterVector;
		if(!baseCluster) baseCluster = edge->clusterTransition->cluster1;
		if(!edge->clusterTransition->isSelfTransition())
			frankRotation = frankRotation * edge->clusterTransition->reverse->tm;
		if(edge == circuitStart)
			break;
		circuitEnd = edge;
		edgeCount++;

		if(edgeCount > maxCircuitLength)
			break;
	}

	// Create secondary segment only for dislocations (b != 0) and small enough dislocation cores.
	if(numCircuits == 1
			|| edgeCount > maxCircuitLength
			|| burgersVector.isZero(CA_LATTICE_VECTOR_EPSILON)
			|| edgeSum.isZero(CA_ATOM_VECTOR_EPSILON) == false
			|| !frankRotation.equals(Matrix3::Identity(), CA_TRANSITION_MATRIX_EPSILON)) {

		// Discard unused circuit.
		edge = circuitStart;
		for(;;) {
			OVITO_ASSERT(edge->circuit == nullptr);
			InterfaceMesh::Edge* nextEdge = edge->nextCircuitEdge;
			edge->nextCircuitEdge = nullptr;
			if(edge == circuitEnd) break;
			edge = nextEdge;
		}
		return;
	}
	OVITO_ASSERT(circuitStart != circuitEnd);

	// Create forward circuit.
	BurgersCircuit* forwardCircuit = allocateCircuit();
	forwardCircuit->firstEdge = circuitStart;
	forwardCircuit->lastEdge = circuitEnd;
	forwardCircuit->edgeCount = edgeCount;
	edge = circuitStart;
	do {
		OVITO_ASSERT(edge->circuit == nullptr);
		edge->circuit = forwardCircuit;
		edge = edge->nextCircuitEdge;
	}
	while(edge != circuitStart);
	OVITO_ASSERT(forwardCircuit->countEdges() == forwardCircuit->edgeCount);

	// Do all the rest.
	createAndTraceSegment(ClusterVector(burgersVector, baseCluster), forwardCircuit, maxCircuitLength);
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
