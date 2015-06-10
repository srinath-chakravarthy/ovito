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

#ifndef _OVITO_CA_CRYSTAL_PATH_FINDER_H
#define _OVITO_CA_CRYSTAL_PATH_FINDER_H

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <core/utilities/MemoryPool.h>
#include <plugins/crystalanalysis/data/ClusterVector.h>
#include "StructureAnalysis.h"

#include <boost/optional.hpp>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/**
 * Utility class that can find the shortest connecting path between two atoms
 * (which may not be nearest neighbors) in the good crystal region.
 *
 * If a path can be found, the routine returns the ClusterVector connecting the two atoms.
 */
class CrystalPathFinder
{
public:

	/// Constructor.
	CrystalPathFinder(StructureAnalysis& structureAnalysis, int maxPathLength) :
		_structureAnalysis(structureAnalysis), _nodePool(1024),
		_visitedAtoms(structureAnalysis.atomCount()),
		_maxPathLength(maxPathLength) {
		OVITO_ASSERT(maxPathLength >= 1);
	}

	/// Returns a reference to the results of the structure analysis.
	const StructureAnalysis& structureAnalysis() const { return _structureAnalysis; }

	/// Returns a reference to the cluster graph.
	ClusterGraph& clusterGraph() { return _structureAnalysis.clusterGraph(); }

	/// Returns a const-reference to the cluster graph.
	const ClusterGraph& clusterGraph() const { return structureAnalysis().clusterGraph(); }

	/// Finds an atom-to-atom path from atom 1 to atom 2 that lies entirely in the good crystal region.
	/// If a path could be found, returns the corresponding ideal vector connecting the two
	/// atoms in the ideal stress-free reference configuration.
	boost::optional<ClusterVector> findPath(int atomIndex1, int atomIndex2);

private:

	/**
	 * This data structure is used for the shortest path search.
	 */
	struct PathNode
	{
		/// Constructor
		PathNode(int _atomIndex, const ClusterVector& _idealVector) :
			atomIndex(_atomIndex), idealVector(_idealVector), nextToProcess(NULL) {}

		/// The atom index.
		int atomIndex;

		/// The vector from the start atom of the path to this atom.
		ClusterVector idealVector;

		/// Number of steps between this atom and the start atom of the recursive walk.
		int distance;

		/// Linked list.
		PathNode* nextToProcess;
	};

	/// The results of the pattern analysis.
	StructureAnalysis& _structureAnalysis;

	/// A memory pool to create PathNode instances.
	MemoryPool<PathNode> _nodePool;

	/// Work array to keep track of atoms which have been visited already.
	boost::dynamic_bitset<> _visitedAtoms;

	/// The maximum length of an atom-to-atom path.
	/// A length of 1 would only return paths between direct neighbor atoms.
	int _maxPathLength;
};

}	// End of namespace
}	// End of namespace
}	// End of namespace

#endif // _OVITO_CA_CRYSTAL_PATH_FINDER_H
