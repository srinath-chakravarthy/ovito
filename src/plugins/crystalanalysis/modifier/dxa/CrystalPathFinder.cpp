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
#include "CrystalPathFinder.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/******************************************************************************
* Finds an atom-to-atom path from atom 1 to atom 2 that lies entirely in the
* good crystal region.
******************************************************************************/
boost::optional<ClusterVector> CrystalPathFinder::findPath(int atomIndex1, int atomIndex2)
{
	OVITO_ASSERT(atomIndex1 != atomIndex2);

	Cluster* cluster1 = structureAnalysis().atomCluster(atomIndex1);
	Cluster* cluster2 = structureAnalysis().atomCluster(atomIndex2);

	// Test if atom 2 is a direct neighbor of atom 1.
	if(cluster1->id != 0) {
		int neighborIndex = structureAnalysis().findNeighbor(atomIndex1, atomIndex2);
		if(neighborIndex != -1) {
			const Vector3& refv = structureAnalysis().neighborLatticeVector(atomIndex1, neighborIndex);
			return boost::optional<ClusterVector>(ClusterVector(refv, cluster1));
		}
	}

	// Test if atom 1 is a direct neighbor of atom 2.
	else if(cluster2->id != 0) {
		int neighborIndex = structureAnalysis().findNeighbor(atomIndex2, atomIndex1);
		if(neighborIndex != -1) {
			const Vector3& refv = structureAnalysis().neighborLatticeVector(atomIndex2, neighborIndex);
			return boost::optional<ClusterVector>(ClusterVector(-refv, cluster2));
		}
	}

	if(_maxPathLength == 1)
		return boost::optional<ClusterVector>();

	_nodePool.clear(true);

	PathNode start(atomIndex1, Vector3::Zero());
	start.distance = 0;

	// Mark the head atom as visited.
	_visitedAtoms.set(atomIndex1);

	// Process items from queue until it becomes empty or the destination atom has been reached.
	PathNode* end_of_queue = &start;
	boost::optional<ClusterVector> result;
	for(PathNode* current = &start; current != nullptr && !result; current = current->nextToProcess) {
		int currentAtom = current->atomIndex;
		OVITO_ASSERT(currentAtom != atomIndex2);
		OVITO_ASSERT(_visitedAtoms.test(currentAtom) == true);

		Cluster* currentCluster = structureAnalysis().atomCluster(currentAtom);
		int numNeighbors = structureAnalysis().numberOfNeighbors(currentAtom);

		for(int neighborIndex = 0; neighborIndex < numNeighbors; neighborIndex++) {

			// Resolve pattern node neighbor to actual neighbor atom.
			int neighbor = structureAnalysis().getNeighbor(currentAtom, neighborIndex);

			// Skip neighbor atom if it has been visited before.
			if(_visitedAtoms.test(neighbor)) continue;

			// Check if maximum path length has been reached.
			if(current->distance >= _maxPathLength - 1 && neighbor != atomIndex2)
				continue;

			ClusterVector step{Vector3::Zero()};
			if(currentCluster->id != 0) {
				step = ClusterVector(structureAnalysis().neighborLatticeVector(currentAtom, neighborIndex), currentCluster);
			}
			else {
				// Do a reverse neighbor search.
				Cluster* neighborCluster = structureAnalysis().atomCluster(neighbor);
				if(neighborCluster->id == 0) continue;

				int numNeighbors2 = structureAnalysis().numberOfNeighbors(neighbor);
				for(int neighborIndex2 = 0; neighborIndex2 < numNeighbors2; neighborIndex2++) {
					if(structureAnalysis().getNeighbor(neighbor, neighborIndex2) == currentAtom) {
						step = ClusterVector(-structureAnalysis().neighborLatticeVector(neighbor, neighborIndex2), neighborCluster);
						break;
					}
				}
				if(step.cluster() == nullptr) continue;
			}

			// Compute the new ideal vector from the start atom to the current neighbor atom.
			ClusterVector pathVector(current->idealVector);

			// Concatenate the two cluster vectors.
			if(pathVector.cluster() == step.cluster())
				pathVector.localVec() += step.localVec();
			else if(pathVector.cluster() != nullptr) {
				OVITO_ASSERT(step.cluster() != nullptr);
				ClusterTransition* transition = clusterGraph().determineClusterTransition(step.cluster(), pathVector.cluster());
				if(!transition)
					continue;	// Failed to concatenate cluster vectors.
				pathVector.localVec() += transition->transform(step.localVec());
			}
			else pathVector = step;

			// Did we reach the destination atom already?
			if(neighbor == atomIndex2) {
				result = pathVector;
				break;
			}

			// Put current neighbor atom on recursive stack.
			if(current->distance < _maxPathLength - 1) {
				// Put neighbor at end of the queue.
				PathNode* neighborNode = _nodePool.construct(neighbor, pathVector);
				neighborNode->distance = current->distance + 1;
				OVITO_ASSERT(end_of_queue->nextToProcess == nullptr);
				end_of_queue->nextToProcess = neighborNode;
				end_of_queue = neighborNode;

				// Mark as visited.
				_visitedAtoms.set(neighbor);
			}
		}
	}

	// Cleanup visit flags.
	for(PathNode* current = &start; current != nullptr; current = current->nextToProcess)
		_visitedAtoms.reset(current->atomIndex);

	return result;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
