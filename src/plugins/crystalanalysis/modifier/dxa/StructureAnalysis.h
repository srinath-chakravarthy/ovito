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
#include <plugins/particles/modifier/analysis/cna/CommonNeighborAnalysisModifier.h>
#include <plugins/crystalanalysis/data/ClusterGraph.h>
#include <core/utilities/concurrent/Promise.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/*
 * Determines the local structure of each atom.
 */
class StructureAnalysis
{
public:

	/// The coordination structure types.
	enum CoordinationStructureType {
		COORD_OTHER = 0,		//< Unidentified structure
		COORD_FCC,				//< Face-centered cubic
		COORD_HCP,				//< Hexagonal close-packed
		COORD_BCC,				//< Body-centered cubic
		COORD_CUBIC_DIAMOND,	//< Diamond cubic
		COORD_HEX_DIAMOND,		//< Diamond hexagonal

		NUM_COORD_TYPES 		//< This just counts the number of defined coordination types.
	};

	/// The lattice structure types.
	enum LatticeStructureType {
		LATTICE_OTHER = 0,			//< Unidentified structure
		LATTICE_FCC,				//< Face-centered cubic
		LATTICE_HCP,				//< Hexagonal close-packed
		LATTICE_BCC,				//< Body-centered cubic
		LATTICE_CUBIC_DIAMOND,		//< Diamond cubic
		LATTICE_HEX_DIAMOND,		//< Diamond hexagonal

		NUM_LATTICE_TYPES 			//< This just counts the number of defined coordination types.
	};

	/// The maximum number of neighbor atoms taken into account for the common neighbor analysis.
	enum { MAX_NEIGHBORS = 16 };

	struct CoordinationStructure {
		int numNeighbors;
		std::vector<Vector3> latticeVectors;
		CommonNeighborAnalysisModifier::NeighborBondArray neighborArray;
		int cnaSignatures[MAX_NEIGHBORS];
		int commonNeighbors[MAX_NEIGHBORS][2];
	};

	struct SymmetryPermutation {
		Matrix3 transformation;
		std::array<int, MAX_NEIGHBORS> permutation;
		std::vector<int> product;
		std::vector<int> inverseProduct;
	};

	struct LatticeStructure {
		const CoordinationStructure* coordStructure;
		std::vector<Vector3> latticeVectors;
		Matrix3 primitiveCell;
		Matrix3 primitiveCellInverse;
		int maxNeighbors;

		/// List of symmetry permutations of the lattice structure.
		/// Each entry contains the rotation/reflection matrix and the corresponding permutation of the neighbor bonds.
		std::vector<SymmetryPermutation> permutations;
	};

public:

	/// Constructor.
	StructureAnalysis(
			ParticleProperty* positions,
			const SimulationCell& simCell,
			LatticeStructureType inputCrystalType,
			ParticleProperty* particleSelection,
			ParticleProperty* outputStructures,
			std::vector<Matrix3>&& preferredCrystalOrientations = std::vector<Matrix3>(),
			bool identifyPlanarDefects = true);

	/// Identifies the atomic structures.
	bool identifyStructures(PromiseBase& promise);

	/// Combines adjacent atoms to clusters.
	bool buildClusters(PromiseBase& promise);

	/// Determines the transition matrices between clusters.
	bool connectClusters(PromiseBase& promise);

	/// Combines clusters to super clusters.
	bool formSuperClusters(PromiseBase& promise);

	/// Returns the number of input atoms.
	int atomCount() const { return positions()->size(); }

	/// Returns the input particle positions.
	ParticleProperty* positions() const { return _positions.data(); }

	/// Returns the input simulation cell.
	const SimulationCell& cell() const { return _simCell; }

	/// Returns the array of atom structure types.
	ParticleProperty* structureTypes() const { return _structureTypes.data(); }

	/// Returns the array of atom cluster IDs.
	ParticleProperty* atomClusters() const { return _atomClusters.data(); }

	/// Returns the maximum distance of any neighbor from a crystalline atom.
	FloatType maximumNeighborDistance() const { return _maximumNeighborDistance; }

	/// Returns the cluster graph.
	const ClusterGraph& clusterGraph() const { return *_clusterGraph; }

	/// Returns the cluster graph.
	ClusterGraph& clusterGraph() { return *_clusterGraph; }

	/// Returns the cluster an atom belongs to.
	Cluster* atomCluster(int atomIndex) const {
		return clusterGraph().findCluster(_atomClusters->getInt(atomIndex));
	}

	/// Returns the number of neighbors of the given atom.
	int numberOfNeighbors(int atomIndex) const {
		OVITO_ASSERT(_neighborLists);
		const int* neighborList = _neighborLists->constDataInt() + (size_t)atomIndex * _neighborLists->componentCount();
		size_t count = 0;
		while(count < _neighborLists->componentCount() && neighborList[count] != -1)
			count++;
		return count;
	}

	/// Returns an atom from an atom's neighbor list.
	int getNeighbor(int centralAtomIndex, int neighborListIndex) const {
		OVITO_ASSERT(_neighborLists);
		return _neighborLists->getIntComponent(centralAtomIndex, neighborListIndex);
	}

	/// Sets an entry in an atom's neighbor list.
	void setNeighbor(int centralAtomIndex, int neighborListIndex, int neighborAtomIndex) const {
		_neighborLists->setIntComponent(centralAtomIndex, neighborListIndex, neighborAtomIndex);
	}

	/// Returns the neighbor list index of the given atom.
	int findNeighbor(int centralAtomIndex, int neighborAtomIndex) const {
		OVITO_ASSERT(_neighborLists);
		const int* neighborList = _neighborLists->constDataInt() + (size_t)centralAtomIndex * _neighborLists->componentCount();
		for(size_t index = 0; index < _neighborLists->componentCount() && neighborList[index] != -1; index++) {
			if(neighborList[index] == neighborAtomIndex)
				return index;
		}
		return -1;
	}

	/// Releases the memory allocated for neighbor lists.
	void freeNeighborLists() {
		_neighborLists.reset();
		_atomSymmetryPermutations.reset();
	}

	/// Returns the ideal lattice vector associated with a neighbor bond.
	const Vector3& neighborLatticeVector(int centralAtomIndex, int neighborIndex) const {
		OVITO_ASSERT(_atomSymmetryPermutations);
		int structureType = _structureTypes->getInt(centralAtomIndex);
		const LatticeStructure& latticeStructure = _latticeStructures[structureType];
		OVITO_ASSERT(neighborIndex >= 0 && neighborIndex < _coordinationStructures[structureType].numNeighbors);
		int symmetryPermutationIndex = _atomSymmetryPermutations->getInt(centralAtomIndex);
		OVITO_ASSERT(symmetryPermutationIndex >= 0 && symmetryPermutationIndex < latticeStructure.permutations.size());
		const auto& permutation = latticeStructure.permutations[symmetryPermutationIndex].permutation;
		return latticeStructure.latticeVectors[permutation[neighborIndex]];
	}

	/// Returns the given lattice structure.
	static const LatticeStructure& latticeStructure(int structureIndex) {
		return _latticeStructures[structureIndex];
	}

	/// Throws an exception which tells the user that the periodic simulation cell is too small.
	static void generateCellTooSmallError(int dimension);

private:

	/// Determines the coordination structure of a particle.
	void determineLocalStructure(NearestNeighborFinder& neighList, size_t particleIndex);

	/// Prepares the list of coordination and lattice structures.
	static void initializeListOfStructures();

private:

	LatticeStructureType _inputCrystalType;
	bool _identifyPlanarDefects;
	QExplicitlySharedDataPointer<ParticleProperty> _positions;
	QExplicitlySharedDataPointer<ParticleProperty> _structureTypes;
	QExplicitlySharedDataPointer<ParticleProperty> _neighborLists;
	QExplicitlySharedDataPointer<ParticleProperty> _atomClusters;
	QExplicitlySharedDataPointer<ParticleProperty> _atomSymmetryPermutations;
	QExplicitlySharedDataPointer<ParticleProperty> _particleSelection;
	QExplicitlySharedDataPointer<ClusterGraph> _clusterGraph;
	std::atomic<FloatType> _maximumNeighborDistance;
	SimulationCell _simCell;
	std::vector<Matrix3> _preferredCrystalOrientations;

	static CoordinationStructure _coordinationStructures[NUM_COORD_TYPES];
	static LatticeStructure _latticeStructures[NUM_LATTICE_TYPES];
};

}	// End of namespace
}	// End of namespace
}	// End of namespace


