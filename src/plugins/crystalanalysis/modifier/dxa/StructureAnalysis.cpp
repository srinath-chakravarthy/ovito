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
#include <plugins/particles/util/NearestNeighborFinder.h>
#include <plugins/particles/modifier/analysis/cna/CommonNeighborAnalysisModifier.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include "StructureAnalysis.h"
#include "DislocationAnalysisModifier.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/// Contains the known coordination structures.
StructureAnalysis::CoordinationStructure StructureAnalysis::_coordinationStructures[NUM_COORD_TYPES];

/// Contains the known lattice types.
StructureAnalysis::LatticeStructure StructureAnalysis::_latticeStructures[NUM_LATTICE_TYPES];

/// Fast sorting function for an array of (bounded) integers.
/// Sorts values in descending order.
template<typename iterator>
void bitmapSort(iterator begin, iterator end, int max)
{
	OVITO_ASSERT(max <= 32);
	OVITO_ASSERT(end >= begin);
	int bitarray = 0;
	for(iterator pin = begin; pin != end; ++pin) {
		OVITO_ASSERT(*pin >= 0 && *pin < max);
		bitarray |= 1 << (*pin);
	}
	iterator pout = begin;
	for(int i = max - 1; i >= 0; i--)
		if(bitarray & (1 << i))
			*pout++ = i;
	OVITO_ASSERT(pout == end);
}

/******************************************************************************
* Constructor.
******************************************************************************/
StructureAnalysis::StructureAnalysis(ParticleProperty* positions, const SimulationCell& simCell,
		LatticeStructureType inputCrystalType, ParticleProperty* particleSelection,
		ParticleProperty* outputStructures, std::vector<Matrix3>&& preferredCrystalOrientations,
		bool identifyPlanarDefects) :
	_positions(positions), _simCell(simCell),
	_inputCrystalType(inputCrystalType),
	_structureTypes(outputStructures),
	_particleSelection(particleSelection),
	_atomClusters(new ParticleProperty(positions->size(), ParticleProperty::ClusterProperty, 0, true)),
	_atomSymmetryPermutations(new ParticleProperty(positions->size(), qMetaTypeId<int>(), 1, 0, QStringLiteral("SymmetryPermutations"), false)),
	_clusterGraph(new ClusterGraph()),
	_preferredCrystalOrientations(std::move(preferredCrystalOrientations)),
	_identifyPlanarDefects(identifyPlanarDefects)
{
	static bool initialized = false;
	if(!initialized) {
		initializeListOfStructures();
		initialized = true;
	}

	// Allocate memory for neighbor lists.
	_neighborLists = new ParticleProperty(positions->size(), qMetaTypeId<int>(),
			latticeStructure(inputCrystalType).maxNeighbors, 0, QStringLiteral("Neighbors"), false);
	std::fill(_neighborLists->dataInt(), _neighborLists->dataInt() + _neighborLists->size() * _neighborLists->componentCount(), -1);

	// Reset atomic structure types.
	std::fill(_structureTypes->dataInt(), _structureTypes->dataInt() + _structureTypes->size(), LATTICE_OTHER);
}

/******************************************************************************
* Throws an exception which tells the user that the periodic simulation cell is too small.
******************************************************************************/
void StructureAnalysis::generateCellTooSmallError(int dimension)
{
	static const QString axes[3] = { QStringLiteral("X"), QStringLiteral("Y"), QStringLiteral("Z") };
	throw Exception(DislocationAnalysisModifier::tr("Simulation box is too short along cell vector %1 (%2) to perform analysis. "
			"Please extend it first using the 'Show periodic images' modifier.").arg(dimension+1).arg(axes[dimension]));
}

/******************************************************************************
* Prepares the list of coordination structures.
******************************************************************************/
void StructureAnalysis::initializeListOfStructures()
{
	_coordinationStructures[COORD_OTHER].numNeighbors = 0;
	_latticeStructures[LATTICE_OTHER].coordStructure = &_coordinationStructures[COORD_OTHER];
	_latticeStructures[LATTICE_OTHER].primitiveCell.setZero();
	_latticeStructures[LATTICE_OTHER].primitiveCellInverse.setZero();
	_latticeStructures[LATTICE_OTHER].maxNeighbors = 0;

	// FCC
	Vector3 fccVec[12] = {
			Vector3( 0.5,  0.5,  0.0),
			Vector3( 0.0,  0.5,  0.5),
			Vector3( 0.5,  0.0,  0.5),
			Vector3(-0.5, -0.5,  0.0),
			Vector3( 0.0, -0.5, -0.5),
			Vector3(-0.5,  0.0, -0.5),
			Vector3(-0.5,  0.5,  0.0),
			Vector3( 0.0, -0.5,  0.5),
			Vector3(-0.5,  0.0,  0.5),
			Vector3( 0.5, -0.5,  0.0),
			Vector3( 0.0,  0.5, -0.5),
			Vector3( 0.5,  0.0, -0.5)
	};
	_coordinationStructures[COORD_FCC].numNeighbors = 12;
	for(int ni1 = 0; ni1 < 12; ni1++) {
		_coordinationStructures[COORD_FCC].neighborArray.setNeighborBond(ni1, ni1, false);
		for(int ni2 = ni1 + 1; ni2 < 12; ni2++) {
			bool bonded = (fccVec[ni1] - fccVec[ni2]).length() < (sqrt(0.5f)+1.0)*0.5;
			_coordinationStructures[COORD_FCC].neighborArray.setNeighborBond(ni1, ni2, bonded);
		}
		_coordinationStructures[COORD_FCC].cnaSignatures[ni1] = 0;
	}
	_coordinationStructures[COORD_FCC].latticeVectors.assign(std::begin(fccVec), std::end(fccVec));
	_latticeStructures[LATTICE_FCC].latticeVectors.assign(std::begin(fccVec), std::end(fccVec));
	_latticeStructures[LATTICE_FCC].coordStructure = &_coordinationStructures[COORD_FCC];
	_latticeStructures[LATTICE_FCC].primitiveCell.column(0) = Vector3(0.5,0.5,0.0);
	_latticeStructures[LATTICE_FCC].primitiveCell.column(1) = Vector3(0.0,0.5,0.5);
	_latticeStructures[LATTICE_FCC].primitiveCell.column(2) = Vector3(0.5,0.0,0.5);
	_latticeStructures[LATTICE_FCC].maxNeighbors = 12;

	// HCP
	Vector3 hcpVec[18] = {
			Vector3(sqrt(2.0)/4.0, -sqrt(6.0)/4.0, 0.0),
			Vector3(-sqrt(2.0)/2.0, 0.0, 0.0),
			Vector3(-sqrt(2.0)/4.0, sqrt(6.0)/12.0, -sqrt(3.0)/3.0),
			Vector3(sqrt(2.0)/4.0, sqrt(6.0)/12.0, -sqrt(3.0)/3.0),
			Vector3(0.0, -sqrt(6.0)/6.0, -sqrt(3.0)/3.0),
			Vector3(-sqrt(2.0)/4.0, sqrt(6.0)/4.0, 0.0),
			Vector3(sqrt(2.0)/4.0, sqrt(6.0)/4.0, 0.0),
			Vector3(sqrt(2.0)/2.0, 0.0, 0.0),
			Vector3(-sqrt(2.0)/4.0, -sqrt(6.0)/4.0, 0.0),
			Vector3(0.0, -sqrt(6.0)/6.0, sqrt(3.0)/3.0),
			Vector3(sqrt(2.0)/4.0, sqrt(6.0)/12.0, sqrt(3.0)/3.0),
			Vector3(-sqrt(2.0)/4.0, sqrt(6.0)/12.0, sqrt(3.0)/3.0),

			Vector3(0.0, sqrt(6.0)/6.0, sqrt(3.0)/3.0),
			Vector3(-sqrt(2.0)/4.0, -sqrt(6.0)/12.0, -sqrt(3.0)/3.0),
			Vector3(sqrt(2.0)/4.0, -sqrt(6.0)/12.0, sqrt(3.0)/3.0),
			Vector3(0.0, sqrt(6.0)/6.0, -sqrt(3.0)/3.0),
			Vector3(sqrt(2.0)/4.0, -sqrt(6.0)/12.0, -sqrt(3.0)/3.0),
			Vector3(-sqrt(2.0)/4.0, -sqrt(6.0)/12.0, sqrt(3.0)/3.0)
	};
	_coordinationStructures[COORD_HCP].numNeighbors = 12;
	for(int ni1 = 0; ni1 < 12; ni1++) {
		_coordinationStructures[COORD_HCP].neighborArray.setNeighborBond(ni1, ni1, false);
		for(int ni2 = ni1 + 1; ni2 < 12; ni2++) {
			bool bonded = (hcpVec[ni1] - hcpVec[ni2]).length() < (sqrt(0.5)+1.0)*0.5;
			_coordinationStructures[COORD_HCP].neighborArray.setNeighborBond(ni1, ni2, bonded);
		}
		_coordinationStructures[COORD_HCP].cnaSignatures[ni1] = (hcpVec[ni1].z() == 0) ? 1 : 0;
	}
	_coordinationStructures[COORD_HCP].latticeVectors.assign(std::begin(hcpVec), std::begin(hcpVec) + 12);
	_latticeStructures[LATTICE_HCP].latticeVectors.assign(std::begin(hcpVec), std::end(hcpVec));
	_latticeStructures[LATTICE_HCP].coordStructure = &_coordinationStructures[COORD_HCP];
	_latticeStructures[LATTICE_HCP].primitiveCell.column(0) = Vector3(sqrt(0.5)/2, -sqrt(6.0)/4, 0.0);
	_latticeStructures[LATTICE_HCP].primitiveCell.column(1) = Vector3(sqrt(0.5)/2, sqrt(6.0)/4, 0.0);
	_latticeStructures[LATTICE_HCP].primitiveCell.column(2) = Vector3(0.0, 0.0, sqrt(8.0/6.0));
	_latticeStructures[LATTICE_HCP].maxNeighbors = 12;

	// BCC
	Vector3 bccVec[14] = {
			Vector3( 0.5,  0.5,  0.5),
			Vector3(-0.5,  0.5,  0.5),
			Vector3( 0.5,  0.5, -0.5),
			Vector3(-0.5, -0.5,  0.5),
			Vector3( 0.5, -0.5,  0.5),
			Vector3(-0.5,  0.5, -0.5),
			Vector3(-0.5, -0.5, -0.5),
			Vector3( 0.5, -0.5, -0.5),
			Vector3( 1.0,  0.0,  0.0),
			Vector3(-1.0,  0.0,  0.0),
			Vector3( 0.0,  1.0,  0.0),
			Vector3( 0.0, -1.0,  0.0),
			Vector3( 0.0,  0.0,  1.0),
			Vector3( 0.0,  0.0, -1.0)
	};
	_coordinationStructures[COORD_BCC].numNeighbors = 14;
	for(int ni1 = 0; ni1 < 14; ni1++) {
		_coordinationStructures[COORD_BCC].neighborArray.setNeighborBond(ni1, ni1, false);
		for(int ni2 = ni1 + 1; ni2 < 14; ni2++) {
			bool bonded = (bccVec[ni1] - bccVec[ni2]).length() < (FloatType(1)+sqrt(FloatType(2)))*FloatType(0.5);
			_coordinationStructures[COORD_BCC].neighborArray.setNeighborBond(ni1, ni2, bonded);
		}
		_coordinationStructures[COORD_BCC].cnaSignatures[ni1] = (ni1 < 8) ? 0 : 1;
	}
	_coordinationStructures[COORD_BCC].latticeVectors.assign(std::begin(bccVec), std::end(bccVec));
	_latticeStructures[LATTICE_BCC].latticeVectors.assign(std::begin(bccVec), std::end(bccVec));
	_latticeStructures[LATTICE_BCC].coordStructure = &_coordinationStructures[COORD_BCC];
	_latticeStructures[LATTICE_BCC].primitiveCell.column(0) = Vector3(1.0,0.0,0.0);
	_latticeStructures[LATTICE_BCC].primitiveCell.column(1) = Vector3(0.0,1.0,0.0);
	_latticeStructures[LATTICE_BCC].primitiveCell.column(2) = Vector3(0.5,0.5,0.5);
	_latticeStructures[LATTICE_BCC].maxNeighbors = 14;

	// Cubic diamond
	Vector3 diamondCubicVec[] = {
			Vector3(0.25, 0.25, 0.25),
			Vector3(0.25, -0.25, -0.25),
			Vector3(-0.25, -0.25, 0.25),
			Vector3(-0.25, 0.25, -0.25),

			Vector3(0, -0.5, 0.5),
			Vector3(0.5, 0.5, 0),
			Vector3(-0.5, 0, 0.5),
			Vector3(-0.5, 0.5, 0),
			Vector3(0, 0.5, 0.5),
			Vector3(0.5, -0.5, 0),
			Vector3(0.5, 0, 0.5),
			Vector3(0.5, 0, -0.5),
			Vector3(-0.5, -0.5, 0),
			Vector3(0, -0.5, -0.5),
			Vector3(0, 0.5, -0.5),
			Vector3(-0.5, 0, -0.5),

			Vector3(0.25, -0.25, 0.25),
			Vector3(0.25, 0.25, -0.25),
			Vector3(-0.25, 0.25, 0.25),
			Vector3(-0.25, -0.25, -0.25)
	};
	_coordinationStructures[COORD_CUBIC_DIAMOND].numNeighbors = 16;
	for(int ni1 = 0; ni1 < 16; ni1++) {
		_coordinationStructures[COORD_CUBIC_DIAMOND].neighborArray.setNeighborBond(ni1, ni1, false);
		FloatType cutoff = (ni1 < 4) ? (sqrt(3.0)*0.25+sqrt(0.5))/2 : (1.0+sqrt(0.5))/2;
		for(int ni2 = ni1 + 1; ni2 < 4; ni2++) {
			_coordinationStructures[COORD_CUBIC_DIAMOND].neighborArray.setNeighborBond(ni1, ni2, false);
		}
		for(int ni2 = std::max(ni1 + 1, 4); ni2 < 16; ni2++) {
			bool bonded = (diamondCubicVec[ni1] - diamondCubicVec[ni2]).length() < cutoff;
			_coordinationStructures[COORD_CUBIC_DIAMOND].neighborArray.setNeighborBond(ni1, ni2, bonded);
		}
		_coordinationStructures[COORD_CUBIC_DIAMOND].cnaSignatures[ni1] = (ni1 < 4) ? 0 : 1;
	}
	_coordinationStructures[COORD_CUBIC_DIAMOND].latticeVectors.assign(std::begin(diamondCubicVec), std::begin(diamondCubicVec) + 16);
	_latticeStructures[LATTICE_CUBIC_DIAMOND].latticeVectors.assign(std::begin(diamondCubicVec), std::end(diamondCubicVec));
	_latticeStructures[LATTICE_CUBIC_DIAMOND].coordStructure = &_coordinationStructures[COORD_CUBIC_DIAMOND];
	_latticeStructures[LATTICE_CUBIC_DIAMOND].primitiveCell.column(0) = Vector3(0.5,0.5,0.0);
	_latticeStructures[LATTICE_CUBIC_DIAMOND].primitiveCell.column(1) = Vector3(0.0,0.5,0.5);
	_latticeStructures[LATTICE_CUBIC_DIAMOND].primitiveCell.column(2) = Vector3(0.5,0.0,0.5);
	_latticeStructures[LATTICE_CUBIC_DIAMOND].maxNeighbors = 16;

	// Hexagonal diamond
	Vector3 diamondHexVec[] = {
			Vector3(-sqrt(2.0)/4, sqrt(3.0/2.0)/6, -sqrt(3.0)/12),
			Vector3(0, -sqrt(3.0/2.0)/3, -sqrt(3.0)/12),
			Vector3(sqrt(2.0)/4, sqrt(3.0/2.0)/6, -sqrt(3.0)/12),
			Vector3(0, 0, sqrt(3.0)/4),

			Vector3(sqrt(2.0)/4.0, -sqrt(6.0)/4.0, 0.0),
			Vector3(-sqrt(2.0)/2.0, 0.0, 0.0),
			Vector3(-sqrt(2.0)/4.0, sqrt(6.0)/4.0, 0.0),
			Vector3(sqrt(2.0)/4.0, sqrt(6.0)/4.0, 0.0),
			Vector3(sqrt(2.0)/2.0, 0.0, 0.0),
			Vector3(-sqrt(2.0)/4.0, -sqrt(6.0)/4.0, 0.0),
			Vector3(-sqrt(2.0)/4.0, sqrt(6.0)/12.0, -sqrt(3.0)/3.0),
			Vector3(sqrt(2.0)/4.0, sqrt(6.0)/12.0, -sqrt(3.0)/3.0),
			Vector3(0.0, -sqrt(6.0)/6.0, -sqrt(3.0)/3.0),
			Vector3(0.0, -sqrt(6.0)/6.0, sqrt(3.0)/3.0),
			Vector3(sqrt(2.0)/4.0, sqrt(6.0)/12.0, sqrt(3.0)/3.0),
			Vector3(-sqrt(2.0)/4.0, sqrt(6.0)/12.0, sqrt(3.0)/3.0),

			Vector3(-sqrt(2.0)/4, sqrt(3.0/2.0)/6, sqrt(3.0)/12),
			Vector3(0, -sqrt(3.0/2.0)/3, sqrt(3.0)/12),
			Vector3(sqrt(2.0)/4, sqrt(3.0/2.0)/6, sqrt(3.0)/12),
			Vector3(0, 0, -sqrt(3.0)/4),

			Vector3(-sqrt(2.0)/4, -sqrt(3.0/2.0)/6, -sqrt(3.0)/12),
			Vector3(0, sqrt(3.0/2.0)/3, -sqrt(3.0)/12),
			Vector3(sqrt(2.0)/4, -sqrt(3.0/2.0)/6, -sqrt(3.0)/12),

			Vector3(-sqrt(2.0)/4, -sqrt(3.0/2.0)/6, sqrt(3.0)/12),
			Vector3(0, sqrt(3.0/2.0)/3, sqrt(3.0)/12),
			Vector3(sqrt(2.0)/4, -sqrt(3.0/2.0)/6, sqrt(3.0)/12),

			Vector3(0.0, sqrt(6.0)/6.0, sqrt(3.0)/3.0),
			Vector3(-sqrt(2.0)/4.0, -sqrt(6.0)/12.0, -sqrt(3.0)/3.0),
			Vector3(sqrt(2.0)/4.0, -sqrt(6.0)/12.0, sqrt(3.0)/3.0),
			Vector3(0.0, sqrt(6.0)/6.0, -sqrt(3.0)/3.0),
			Vector3(sqrt(2.0)/4.0, -sqrt(6.0)/12.0, -sqrt(3.0)/3.0),
			Vector3(-sqrt(2.0)/4.0, -sqrt(6.0)/12.0, sqrt(3.0)/3.0)
	};
	_coordinationStructures[COORD_HEX_DIAMOND].numNeighbors = 16;
	for(int ni1 = 0; ni1 < 16; ni1++) {
		_coordinationStructures[COORD_HEX_DIAMOND].neighborArray.setNeighborBond(ni1, ni1, false);
		FloatType cutoff = (ni1 < 4) ? (sqrt(3.0)*0.25+sqrt(0.5))/2 : (1.0+sqrt(0.5))/2;
		for(int ni2 = ni1 + 1; ni2 < 4; ni2++) {
			_coordinationStructures[COORD_HEX_DIAMOND].neighborArray.setNeighborBond(ni1, ni2, false);
		}
		for(int ni2 = std::max(ni1 + 1, 4); ni2 < 16; ni2++) {
			bool bonded = (diamondHexVec[ni1] - diamondHexVec[ni2]).length() < cutoff;
			_coordinationStructures[COORD_HEX_DIAMOND].neighborArray.setNeighborBond(ni1, ni2, bonded);
		}
		_coordinationStructures[COORD_HEX_DIAMOND].cnaSignatures[ni1] = (ni1 < 4) ? 0 : ((diamondHexVec[ni1].z() == 0) ? 2 : 1);
	}
	_coordinationStructures[COORD_HEX_DIAMOND].latticeVectors.assign(std::begin(diamondHexVec), std::begin(diamondHexVec) + 16);
	_latticeStructures[LATTICE_HEX_DIAMOND].latticeVectors.assign(std::begin(diamondHexVec), std::end(diamondHexVec));
	_latticeStructures[LATTICE_HEX_DIAMOND].coordStructure = &_coordinationStructures[COORD_HEX_DIAMOND];
	_latticeStructures[LATTICE_HEX_DIAMOND].primitiveCell.column(0) = Vector3(sqrt(0.5)/2, -sqrt(6.0)/4, 0.0);
	_latticeStructures[LATTICE_HEX_DIAMOND].primitiveCell.column(1) = Vector3(sqrt(0.5)/2, sqrt(6.0)/4, 0.0);
	_latticeStructures[LATTICE_HEX_DIAMOND].primitiveCell.column(2) = Vector3(0.0, 0.0, sqrt(8.0/6.0));
	_latticeStructures[LATTICE_HEX_DIAMOND].maxNeighbors = 16;

	for(auto coordStruct = std::begin(_coordinationStructures); coordStruct != std::end(_coordinationStructures); ++coordStruct) {
		// Find two non-coplanar common neighbors for every neighbor bond.
		for(int neighIndex = 0; neighIndex < coordStruct->numNeighbors; neighIndex++) {
			Matrix3 tm;
			tm.column(0) = coordStruct->latticeVectors[neighIndex];
			bool found = false;
			for(int i1 = 0; i1 < coordStruct->numNeighbors && !found; i1++) {
				if(!coordStruct->neighborArray.neighborBond(neighIndex, i1)) continue;
				tm.column(1) = coordStruct->latticeVectors[i1];
				for(int i2 = i1 + 1; i2 < coordStruct->numNeighbors; i2++) {
					if(!coordStruct->neighborArray.neighborBond(neighIndex, i2)) continue;
					tm.column(2) = coordStruct->latticeVectors[i2];
					if(std::abs(tm.determinant()) > FLOATTYPE_EPSILON) {
						coordStruct->commonNeighbors[neighIndex][0] = i1;
						coordStruct->commonNeighbors[neighIndex][1] = i2;
						found = true;
						break;
					}
				}
			}
			OVITO_ASSERT(found);
		}
	}

	// Generate symmetry information
	for(auto latticeStruct = std::begin(_latticeStructures); latticeStruct != std::end(_latticeStructures); ++latticeStruct) {
		if(latticeStruct->latticeVectors.empty()) continue;

		latticeStruct->primitiveCellInverse = latticeStruct->primitiveCell.inverse();

		const CoordinationStructure& coordStruct = *latticeStruct->coordStructure;
		OVITO_ASSERT(latticeStruct->latticeVectors.size() >= coordStruct.latticeVectors.size());
		OVITO_ASSERT(coordStruct.latticeVectors.size() == coordStruct.numNeighbors);

		// Find three non-coplanar neighbor vectors.
		int nindices[3];
		Matrix3 tm1;
		int n = 0;
		for(int i = 0; i < coordStruct.numNeighbors && n < 3; i++) {
			tm1.column(n) = coordStruct.latticeVectors[i];
			if(n == 1) {
				if(tm1.column(0).cross(tm1.column(1)).squaredLength() <= FLOATTYPE_EPSILON)
					continue;
			}
			else if(n == 2) {
				if(std::abs(tm1.determinant()) <= FLOATTYPE_EPSILON)
					continue;
			}
			nindices[n++] = i;
		}
		OVITO_ASSERT(n == 3);
		OVITO_ASSERT(std::abs(tm1.determinant()) > FLOATTYPE_EPSILON);
		Matrix3 tm1inverse = tm1.inverse();

		// Find symmetry permutations.
		std::vector<int> permutation(latticeStruct->latticeVectors.size());
		std::vector<int> lastPermutation(latticeStruct->latticeVectors.size(), -1);
		std::iota(permutation.begin(), permutation.end(), 0);
		SymmetryPermutation symmetryPermutation;
		do {
			int changedFrom = std::mismatch(permutation.begin(), permutation.end(), lastPermutation.begin()).first - permutation.begin();
			OVITO_ASSERT(changedFrom < coordStruct.numNeighbors);
			std::copy(permutation.begin(), permutation.end(), lastPermutation.begin());
			if(changedFrom <= nindices[2]) {
				Matrix3 tm2;
				tm2.column(0) = latticeStruct->latticeVectors[permutation[nindices[0]]];
				tm2.column(1) = latticeStruct->latticeVectors[permutation[nindices[1]]];
				tm2.column(2) = latticeStruct->latticeVectors[permutation[nindices[2]]];
				symmetryPermutation.transformation = tm2 * tm1inverse;
				if(!symmetryPermutation.transformation.isOrthogonalMatrix()) {
					bitmapSort(permutation.begin() + nindices[2] + 1, permutation.end(), permutation.size());
					continue;
				}
				changedFrom = 0;
			}
			int sortFrom = nindices[2];
			int invalidFrom;
			for(invalidFrom = changedFrom; invalidFrom < coordStruct.numNeighbors; invalidFrom++) {
				Vector3 v = symmetryPermutation.transformation * coordStruct.latticeVectors[invalidFrom];
				if(!v.equals(latticeStruct->latticeVectors[permutation[invalidFrom]]))
					break;
			}
			if(invalidFrom == coordStruct.numNeighbors) {
				std::copy(permutation.begin(), permutation.begin() + coordStruct.numNeighbors, symmetryPermutation.permutation.begin());
				for(const auto& entry : latticeStruct->permutations) {
					OVITO_ASSERT(!entry.transformation.equals(symmetryPermutation.transformation));
				}
				latticeStruct->permutations.push_back(symmetryPermutation);
			}
			else {
				sortFrom = invalidFrom;
			}
			bitmapSort(permutation.begin() + sortFrom + 1, permutation.end(), permutation.size());
		}
		while(std::next_permutation(permutation.begin(), permutation.end()));

		OVITO_ASSERT(latticeStruct->permutations.size() >= 1);
		OVITO_ASSERT(latticeStruct->permutations.front().transformation.equals(Matrix3::Identity()));

		// Determine products of symmetry transformations.
		for(int s1 = 0; s1 < latticeStruct->permutations.size(); s1++) {
			for(int s2 = 0; s2 < latticeStruct->permutations.size(); s2++) {
				Matrix3 product = latticeStruct->permutations[s2].transformation * latticeStruct->permutations[s1].transformation;
				for(int i = 0; i < latticeStruct->permutations.size(); i++) {
					if(latticeStruct->permutations[i].transformation.equals(product)) {
						latticeStruct->permutations[s1].product.push_back(i);
						break;
					}
				}
				OVITO_ASSERT(latticeStruct->permutations[s1].product.size() == s2 + 1);
				Matrix3 inverseProduct = latticeStruct->permutations[s2].transformation.inverse() * latticeStruct->permutations[s1].transformation;
				for(int i = 0; i < latticeStruct->permutations.size(); i++) {
					if(latticeStruct->permutations[i].transformation.equals(product)) {
						latticeStruct->permutations[s1].inverseProduct.push_back(i);
						break;
					}
				}
				OVITO_ASSERT(latticeStruct->permutations[s1].inverseProduct.size() == s2 + 1);
			}
		}
	}
}

/******************************************************************************
* Identifies the atomic structures.
******************************************************************************/
bool StructureAnalysis::identifyStructures(PromiseBase& promise)
{
	// Prepare the neighbor list.
	int maxNeighborListSize = std::min((int)_neighborLists->componentCount() + 1, (int)MAX_NEIGHBORS);
	NearestNeighborFinder neighFinder(maxNeighborListSize);
	if(!neighFinder.prepare(positions(), cell(), _particleSelection.data(), promise))
		return false;

	// Identify local structure around each particle.
	_maximumNeighborDistance = 0;

	return parallelFor(positions()->size(), promise, [this, &neighFinder](size_t index) {
		determineLocalStructure(neighFinder, index);
	});
}

/******************************************************************************
* Determines the coordination structure of a particle.
******************************************************************************/
void StructureAnalysis::determineLocalStructure(NearestNeighborFinder& neighList, size_t particleIndex)
{
	OVITO_ASSERT(_structureTypes->getInt(particleIndex) == COORD_OTHER);

	// Skip atoms that are not included in the analysis.
	if(_particleSelection && _particleSelection->getInt(particleIndex) == 0)
		return;

	// Construct local neighbor list builder.
	NearestNeighborFinder::Query<MAX_NEIGHBORS> neighQuery(neighList);

	// Find N nearest neighbors of current atom.
	neighQuery.findNeighbors(particleIndex);
	int numNeighbors = neighQuery.results().size();
	int neighborIndices[MAX_NEIGHBORS];
	Vector3 neighborVectors[MAX_NEIGHBORS];

	// Number of neighbors to analyze.
	int nn;
	if(_inputCrystalType == LATTICE_FCC || _inputCrystalType == LATTICE_HCP)
		nn = 12;
	else if(_inputCrystalType == LATTICE_BCC)
		nn = 14;
	else if(_inputCrystalType == LATTICE_CUBIC_DIAMOND || _inputCrystalType == LATTICE_HEX_DIAMOND)
		nn = 16;
	else
		return;

	// Early rejection of under-coordinated atoms:
	if(numNeighbors < nn)
		return;

	FloatType localScaling = 0;
	FloatType localCutoff = 0;
	CommonNeighborAnalysisModifier::NeighborBondArray neighborArray;

	if(_inputCrystalType != LATTICE_CUBIC_DIAMOND && _inputCrystalType != LATTICE_HEX_DIAMOND) {

		// Compute local scale factor.
		if(_inputCrystalType == LATTICE_FCC || _inputCrystalType == LATTICE_HCP) {
			for(int n = 0; n < 12; n++)
				localScaling += sqrt(neighQuery.results()[n].distanceSq);
			localScaling /= 12;
			localCutoff = localScaling * (1.0f + sqrt(2.0f)) * 0.5f;
		}
		else if(_inputCrystalType == LATTICE_BCC) {
			for(int n = 0; n < 8; n++)
				localScaling += sqrt(neighQuery.results()[n].distanceSq);
			localScaling /= 8;
			localCutoff = localScaling / (sqrt(3.0)/2.0) * 0.5 * (1.0 + sqrt(2.0));
		}
		FloatType localCutoffSquared =  localCutoff * localCutoff;

		// Make sure the (N+1)-th atom is beyond the cutoff radius (if it exists).
		if(numNeighbors > nn && neighQuery.results()[nn].distanceSq <= localCutoffSquared)
			return;

		// Compute common neighbor bit-flag array.
		for(int ni1 = 0; ni1 < nn; ni1++) {
			neighborIndices[ni1] = neighQuery.results()[ni1].index;
			neighborVectors[ni1] = neighQuery.results()[ni1].delta;
			neighborArray.setNeighborBond(ni1, ni1, false);
			for(int ni2 = ni1+1; ni2 < nn; ni2++)
				neighborArray.setNeighborBond(ni1, ni2, (neighQuery.results()[ni1].delta - neighQuery.results()[ni2].delta).squaredLength() <= localCutoffSquared);
		}
	}
	else {
		// Generate list of second nearest neighbors.
		int outputIndex = 4;
		for(size_t i = 0; i < 4; i++) {
			const Vector3& v0 = neighQuery.results()[i].delta;
			neighborVectors[i] = v0;
			neighborIndices[i] = neighQuery.results()[i].index;
			NearestNeighborFinder::Query<MAX_NEIGHBORS> neighQuery2(neighList);
			neighQuery2.findNeighbors(neighborIndices[i]);
			if(neighQuery2.results().size() < 4) return;
			for(size_t j = 0; j < 4; j++) {
				Vector3 v = v0 + neighQuery2.results()[j].delta;
				if(neighQuery2.results()[j].index == particleIndex && v.isZero())
					continue;
				if(outputIndex == 16)
					return;
				neighborIndices[outputIndex] = neighQuery2.results()[j].index;
				neighborVectors[outputIndex] = v;
				neighborArray.setNeighborBond(i, outputIndex, true);
				outputIndex++;
			}
			if(outputIndex != i*3 + 7)
				return;
		}

		// Compute local scale factor.
		localScaling = 0;
		for(int n = 4; n < 16; n++)
			localScaling += neighborVectors[n].length();
		localScaling /= 12;
		localCutoff = localScaling * FloatType(1.2071068);
		FloatType localCutoffSquared =  localCutoff * localCutoff;

		// Compute common neighbor bit-flag array.
		for(int ni1 = 4; ni1 < nn; ni1++) {
			for(int ni2 = ni1+1; ni2 < nn; ni2++)
				neighborArray.setNeighborBond(ni1, ni2, (neighborVectors[ni1] - neighborVectors[ni2]).squaredLength() <= localCutoffSquared);
		}
	}

	CoordinationStructureType coordinationType;
	int cnaSignatures[MAX_NEIGHBORS];
	if(_inputCrystalType == LATTICE_FCC || _inputCrystalType == LATTICE_HCP) {
		int n421 = 0;
		int n422 = 0;
		for(int ni = 0; ni < nn; ni++) {

			// Determine number of neighbors the two atoms have in common.
			unsigned int commonNeighbors;
			int numCommonNeighbors = CommonNeighborAnalysisModifier::findCommonNeighbors(neighborArray, ni, commonNeighbors, nn);
			if(numCommonNeighbors != 4)
				break;

			// Determine the number of bonds among the common neighbors.
			CommonNeighborAnalysisModifier::CNAPairBond neighborBonds[MAX_NEIGHBORS*MAX_NEIGHBORS];
			int numNeighborBonds = CommonNeighborAnalysisModifier::findNeighborBonds(neighborArray, commonNeighbors, nn, neighborBonds);
			if(numNeighborBonds != 2)
				break;

			// Determine the number of bonds in the longest continuous chain.
			int maxChainLength = CommonNeighborAnalysisModifier::calcMaxChainLength(neighborBonds, numNeighborBonds);
			if(maxChainLength == 1) {
				n421++;
				cnaSignatures[ni] = 0;
			}
			else if(maxChainLength == 2) {
				n422++;
				cnaSignatures[ni] = 1;
			}
			else break;
		}
		if(n421 == 12 && (_identifyPlanarDefects || _inputCrystalType == LATTICE_FCC)) {
			coordinationType = COORD_FCC;
		}
		else if(n421 == 6 && n422 == 6 && (_identifyPlanarDefects || _inputCrystalType == LATTICE_HCP)) {
			coordinationType = COORD_HCP;
		}
		else return;
	}
	else if(_inputCrystalType == LATTICE_BCC) {
		int n444 = 0;
		int n666 = 0;
		for(int ni = 0; ni < nn; ni++) {

			// Determine number of neighbors the two atoms have in common.
			unsigned int commonNeighbors;
			int numCommonNeighbors = CommonNeighborAnalysisModifier::findCommonNeighbors(neighborArray, ni, commonNeighbors, 14);
			if(numCommonNeighbors != 4 && numCommonNeighbors != 6)
				break;

			// Determine the number of bonds among the common neighbors.
			CommonNeighborAnalysisModifier::CNAPairBond neighborBonds[MAX_NEIGHBORS*MAX_NEIGHBORS];
			int numNeighborBonds = CommonNeighborAnalysisModifier::findNeighborBonds(neighborArray, commonNeighbors, 14, neighborBonds);
			if(numNeighborBonds != 4 && numNeighborBonds != 6)
				break;

			// Determine the number of bonds in the longest continuous chain.
			int maxChainLength = CommonNeighborAnalysisModifier::calcMaxChainLength(neighborBonds, numNeighborBonds);
			if(numCommonNeighbors == 4 && numNeighborBonds == 4 && maxChainLength == 4) {
				n444++;
				cnaSignatures[ni] = 1;
			}
			else if(numCommonNeighbors == 6 && numNeighborBonds == 6 && maxChainLength == 6) {
				n666++;
				cnaSignatures[ni] = 0;
			}
			else break;
		}
		if(n666 == 8 && n444 == 6)
			coordinationType = COORD_BCC;
		else
			return;
	}
	else if(_inputCrystalType == LATTICE_CUBIC_DIAMOND || _inputCrystalType == LATTICE_HEX_DIAMOND) {
		for(int ni = 0; ni < 4; ni++) {
			cnaSignatures[ni] = 0;
			unsigned int commonNeighbors;
			if(CommonNeighborAnalysisModifier::findCommonNeighbors(neighborArray, ni, commonNeighbors, nn) != 3)
				return;
		}
		int n543 = 0;
		int n544 = 0;
		for(int ni = 4; ni < nn; ni++) {
			// Determine number of neighbors the two atoms have in common.
			unsigned int commonNeighbors;
			int numCommonNeighbors = CommonNeighborAnalysisModifier::findCommonNeighbors(neighborArray, ni, commonNeighbors, nn);
			if(numCommonNeighbors != 5)
				break;

			// Determine the number of bonds among the common neighbors.
			CommonNeighborAnalysisModifier::CNAPairBond neighborBonds[MAX_NEIGHBORS*MAX_NEIGHBORS];
			int numNeighborBonds = CommonNeighborAnalysisModifier::findNeighborBonds(neighborArray, commonNeighbors, nn, neighborBonds);
			if(numNeighborBonds != 4)
				break;

			// Determine the number of bonds in the longest continuous chain.
			int maxChainLength = CommonNeighborAnalysisModifier::calcMaxChainLength(neighborBonds, numNeighborBonds);
			if(maxChainLength == 3) {
				n543++;
				cnaSignatures[ni] = 1;
			}
			else if(maxChainLength == 4) {
				n544++;
				cnaSignatures[ni] = 2;
			}
			else break;
		}
		if(n543 == 12 && (_identifyPlanarDefects || _inputCrystalType == LATTICE_CUBIC_DIAMOND)) {
			coordinationType = COORD_CUBIC_DIAMOND;
		}
		else if(n543 == 6 && n544 == 6 && (_identifyPlanarDefects || _inputCrystalType == LATTICE_HEX_DIAMOND)) {
			coordinationType = COORD_HEX_DIAMOND;
		}
		else return;
	}

	// Initialize permutation.
	int neighborMapping[MAX_NEIGHBORS];
	int previousMapping[MAX_NEIGHBORS];
	for(int n = 0; n < nn; n++) {
		neighborMapping[n] = n;
		previousMapping[n] = -1;
	}

	// Find first matching neighbor permutation.
	const CoordinationStructure& coordStructure = _coordinationStructures[coordinationType];
	for(;;) {
		int ni1 = 0;
		while(neighborMapping[ni1] == previousMapping[ni1]) {
			ni1++;
			OVITO_ASSERT(ni1 < nn);
		}
		for(; ni1 < nn; ni1++) {
			int atomNeighborIndex1 = neighborMapping[ni1];
			previousMapping[ni1] = atomNeighborIndex1;
			if(cnaSignatures[atomNeighborIndex1] != coordStructure.cnaSignatures[ni1]) {
				break;
			}
			int ni2;
			for(ni2 = 0; ni2 < ni1; ni2++) {
				int atomNeighborIndex2 = neighborMapping[ni2];
				if(neighborArray.neighborBond(atomNeighborIndex1, atomNeighborIndex2) != coordStructure.neighborArray.neighborBond(ni1, ni2)) {
					break;
				}
			}
			if(ni2 != ni1) break;
		}
		if(ni1 == nn) {
			// Assign coordination structure type to atom.
			_structureTypes->setInt(particleIndex, coordinationType);

			// Save the atom's neighbor list.
			for(int i = 0; i < nn; i++) {
				const Vector3& neighborVector = neighborVectors[neighborMapping[i]];
				// Check if neighbor vectors spans more than half of a periodic simulation cell.
				for(size_t dim = 0; dim < 3; dim++) {
					if(cell().pbcFlags()[dim]) {
						if(std::abs(cell().inverseMatrix().prodrow(neighborVector, dim)) >= FloatType(0.5)+FLOATTYPE_EPSILON)
							StructureAnalysis::generateCellTooSmallError(dim);
					}
				}
				setNeighbor(particleIndex, i, neighborIndices[neighborMapping[i]]);
			}

			// Determine maximum neighbor distance.
			// This is a thread-safe implementation.
			FloatType prev_value = _maximumNeighborDistance;
			while(prev_value < localCutoff && !_maximumNeighborDistance.compare_exchange_weak(prev_value, localCutoff)) {}

			return;
		}
		bitmapSort(neighborMapping + ni1 + 1, neighborMapping + nn, nn);
		if(!std::next_permutation(neighborMapping, neighborMapping + nn)) {
			// This should not happen.
			OVITO_ASSERT(false);
			return;
		}
	}
}

/******************************************************************************
* Combines adjacent atoms to clusters.
******************************************************************************/
bool StructureAnalysis::buildClusters(PromiseBase& promise)
{
	promise.setProgressValue(0);
	promise.setProgressMaximum(positions()->size());
	int progressCounter = 0;

	// Iterate over atoms, looking for those that have not been visited yet.
	for(size_t seedAtomIndex = 0; seedAtomIndex < positions()->size(); seedAtomIndex++) {
		if(_atomClusters->getInt(seedAtomIndex) != 0) continue;
		int coordStructureType = _structureTypes->getInt(seedAtomIndex);

		if(coordStructureType == COORD_OTHER) {
			progressCounter++;
			continue;
		}

		// Start a new cluster.
		int latticeStructureType = coordStructureType;
		Cluster* cluster = clusterGraph().createCluster(latticeStructureType);
		OVITO_ASSERT(cluster->id > 0);
		cluster->atomCount = 1;
		_atomClusters->setInt(seedAtomIndex, cluster->id);
		_atomSymmetryPermutations->setInt(seedAtomIndex, 0);
		const CoordinationStructure& coordStructure = _coordinationStructures[coordStructureType];
		const LatticeStructure& latticeStructure = _latticeStructures[latticeStructureType];

		// For calculating the cluster orientation.
		Matrix_3<double> orientationV = Matrix_3<double>::Zero();
		Matrix_3<double> orientationW = Matrix_3<double>::Zero();

		// Add neighboring atoms to the cluster.
		std::deque<int> atomsToVisit(1, seedAtomIndex);
		do {
			int currentAtomIndex = atomsToVisit.front();
			atomsToVisit.pop_front();

			// Update progress indicator.
			if(!promise.setProgressValueIntermittent(++progressCounter))
				return false;

			// Look up symmetry permutation of current atom.
			int symmetryPermutationIndex = _atomSymmetryPermutations->getInt(currentAtomIndex);
			const auto& permutation = latticeStructure.permutations[symmetryPermutationIndex].permutation;

			// Visit neighbors of the current atom.
			for(int neighborIndex = 0; neighborIndex < coordStructure.numNeighbors; neighborIndex++) {

				// An atom should not be a neighbor of itself.
				// We use the minimum image convention for simulation cells with periodic boundary conditions.
				int neighborAtomIndex = getNeighbor(currentAtomIndex, neighborIndex);
				OVITO_ASSERT(neighborAtomIndex != currentAtomIndex);

				// Add vector pair to matrices for computing the cluster orientation.
				const Vector3& latticeVector = latticeStructure.latticeVectors[permutation[neighborIndex]];
				const Vector3& spatialVector = cell().wrapVector(positions()->getPoint3(neighborAtomIndex) - positions()->getPoint3(currentAtomIndex));
				for(size_t i = 0; i < 3; i++) {
					for(size_t j = 0; j < 3; j++) {
						orientationV(i,j) += (double)(latticeVector[j] * latticeVector[i]);
						orientationW(i,j) += (double)(latticeVector[j] * spatialVector[i]);
					}
				}

				// Skip neighbors which are already part of the cluster, or which have a different coordination structure type.
				if(_atomClusters->getInt(neighborAtomIndex) != 0) continue;
				if(_structureTypes->getInt(neighborAtomIndex) != coordStructureType) continue;

				// Select three non-coplanar atoms, which are all neighbors of the current neighbor.
				// One of them is the current central atom, two are common neighbors.
				Matrix3 tm1, tm2;
				bool properOverlap = true;
				for(int i = 0; i < 3; i++) {
					int atomIndex;
					if(i != 2) {
						atomIndex = getNeighbor(currentAtomIndex, coordStructure.commonNeighbors[neighborIndex][i]);
						tm1.column(i) = latticeStructure.latticeVectors[permutation[coordStructure.commonNeighbors[neighborIndex][i]]] - latticeStructure.latticeVectors[permutation[neighborIndex]];
					}
					else {
						atomIndex = currentAtomIndex;
						tm1.column(i) = -latticeStructure.latticeVectors[permutation[neighborIndex]];
					}
					OVITO_ASSERT(numberOfNeighbors(neighborAtomIndex) == coordStructure.numNeighbors);
					int j = findNeighbor(neighborAtomIndex, atomIndex);
					if(j == -1) {
						properOverlap = false;
						break;
					}
					tm2.column(i) = latticeStructure.latticeVectors[j];
				}
				if(!properOverlap) {
					continue;
				}

				// Determine the misorientation matrix.
				OVITO_ASSERT(std::abs(tm1.determinant()) > FLOATTYPE_EPSILON);
				Matrix3 tm2inverse;
				if(!tm2.inverse(tm2inverse)) continue;
				Matrix3 transition = tm1 * tm2inverse;

				// Find the corresponding symmetry permutation.
				for(int i = 0; i < latticeStructure.permutations.size(); i++) {
					if(transition.equals(latticeStructure.permutations[i].transformation, CA_TRANSITION_MATRIX_EPSILON)) {

						// Make the neighbor atom part of the current cluster.
						_atomClusters->setInt(neighborAtomIndex, cluster->id);
						cluster->atomCount++;

						// Save the permutation index.
						_atomSymmetryPermutations->setInt(neighborAtomIndex, i);

						// Recursively continue with the neighbor.
						atomsToVisit.push_back(neighborAtomIndex);

						break;
					}
				}
			}
		}
		while(!atomsToVisit.empty());

		// Compute matrix, which transforms vectors from lattice space to simulation coordinates.
		cluster->orientation = Matrix3(orientationW * orientationV.inverse());

#if 1
		if(latticeStructureType == _inputCrystalType && !_preferredCrystalOrientations.empty()) {
			// Determine the symmetry permutation that leads to the best cluster orientation.
			// The best cluster orientation is the one that forms the smallest angle with one of the
			// preferred crystal orientations.
			FloatType smallestDeviation = std::numeric_limits<FloatType>::max();
			const Matrix3 oldOrientation = cluster->orientation;
			for(int symmetryPermutationIndex = 0; symmetryPermutationIndex < latticeStructure.permutations.size(); symmetryPermutationIndex++) {
				const Matrix3& symmetryTMatrix = latticeStructure.permutations[symmetryPermutationIndex].transformation;
				Matrix3 newOrientation = oldOrientation * symmetryTMatrix.inverse();
				FloatType scaling = std::pow(std::abs(newOrientation.determinant()), 1.0/3.0);
				for(const Matrix3& preferredOrientation : _preferredCrystalOrientations) {
					FloatType deviation = 0;
					for(size_t i = 0; i < 3; i++)
						for(size_t j = 0; j < 3; j++)
							deviation += std::abs(newOrientation(i, j)/scaling - preferredOrientation(i,j));
					if(deviation < smallestDeviation) {
						smallestDeviation = deviation;
						cluster->symmetryTransformation = symmetryPermutationIndex;
						cluster->orientation = newOrientation;
					}
				}
			}
		}
#endif
	}

	// Reorient atoms to align clusters with global coordinate system.
	for(size_t atomIndex = 0; atomIndex < positions()->size(); atomIndex++) {
		int clusterId = _atomClusters->getInt(atomIndex);
		if(clusterId == 0) continue;
		Cluster* cluster = clusterGraph().findCluster(clusterId);
		OVITO_ASSERT(cluster);
		if(cluster->symmetryTransformation == 0) continue;
		const LatticeStructure& latticeStructure = _latticeStructures[cluster->structure];
		int oldSymmetryPermutation = _atomSymmetryPermutations->getInt(atomIndex);
		int newSymmetryPermutation = latticeStructure.permutations[oldSymmetryPermutation].inverseProduct[cluster->symmetryTransformation];
		_atomSymmetryPermutations->setInt(atomIndex, newSymmetryPermutation);
	}

	//qDebug() << "Number of clusters:" << (clusterGraph().clusters().size() - 1);

	return !promise.isCanceled();
}

/******************************************************************************
* Determines the transition matrices between clusters.
******************************************************************************/
bool StructureAnalysis::connectClusters(PromiseBase& promise)
{
	promise.setProgressValue(0);
	promise.setProgressMaximum(positions()->size());

	for(size_t atomIndex = 0; atomIndex < positions()->size(); atomIndex++) {
		int clusterId = _atomClusters->getInt(atomIndex);
		if(clusterId == 0) continue;
		Cluster* cluster1 = clusterGraph().findCluster(clusterId);
		OVITO_ASSERT(cluster1);

		// Update progress indicator.
		if(!promise.setProgressValueIntermittent(atomIndex))
			return false;

		// Look up symmetry permutation of current atom.
		int structureType = _structureTypes->getInt(atomIndex);
		const LatticeStructure& latticeStructure = _latticeStructures[structureType];
		const CoordinationStructure& coordStructure = _coordinationStructures[structureType];
		int symmetryPermutationIndex = _atomSymmetryPermutations->getInt(atomIndex);
		const auto& permutation = latticeStructure.permutations[symmetryPermutationIndex].permutation;

		// Visit neighbors of the current atom.
		for(int ni = 0; ni < coordStructure.numNeighbors; ni++) {
			int neighbor = getNeighbor(atomIndex, ni);

			// Skip neighbor atoms belonging to the same cluster or to no cluster at all.
			int neighborClusterId = _atomClusters->getInt(neighbor);
			if(neighborClusterId == 0 || neighborClusterId == clusterId) {

				// Add this atom to the neighbor's list of neighbors.
				if(neighborClusterId == 0) {
					int otherNeighborListCount = numberOfNeighbors(neighbor);
					if(otherNeighborListCount < _neighborLists->componentCount())
						setNeighbor(neighbor, otherNeighborListCount, atomIndex);
				}

				continue;
			}
			Cluster* cluster2 = clusterGraph().findCluster(neighborClusterId);
			OVITO_ASSERT(cluster2);

			// Skip if there is already a transition between the two clusters.
			if(ClusterTransition* t = cluster1->findTransition(cluster2)) {
				t->area++;
				t->reverse->area++;
				continue;
			}

			// Select three non-coplanar atoms, which are all neighbors of the current neighbor.
			// One of them is the current central atom, two are common neighbors.
			Matrix3 tm1, tm2;
			bool properOverlap = true;
			for(int i = 0; i < 3; i++) {
				int ai;
				if(i != 2) {
					ai = getNeighbor(atomIndex, coordStructure.commonNeighbors[ni][i]);
					tm1.column(i) = latticeStructure.latticeVectors[permutation[coordStructure.commonNeighbors[ni][i]]] - latticeStructure.latticeVectors[permutation[ni]];
				}
				else {
					ai = atomIndex;
					tm1.column(i) = -latticeStructure.latticeVectors[permutation[ni]];
				}
				OVITO_ASSERT(numberOfNeighbors(neighbor) == coordStructure.numNeighbors);
				int j = findNeighbor(neighbor, ai);
				if(j == -1) {
					properOverlap = false;
					break;
				}

				// Look up symmetry permutation of neighbor atom.
				int neighborStructureType = _structureTypes->getInt(neighbor);
				const LatticeStructure& neighborLatticeStructure = _latticeStructures[neighborStructureType];
				int neighborSymmetryPermutationIndex = _atomSymmetryPermutations->getInt(neighbor);
				const auto& neighborPermutation = neighborLatticeStructure.permutations[neighborSymmetryPermutationIndex].permutation;

				tm2.column(i) = neighborLatticeStructure.latticeVectors[neighborPermutation[j]];
			}
			if(!properOverlap)
				continue;

			// Determine the misorientation matrix.
			OVITO_ASSERT(std::abs(tm1.determinant()) > FLOATTYPE_EPSILON);
			//OVITO_ASSERT(std::abs(tm2.determinant()) > FLOATTYPE_EPSILON);
			Matrix3 tm1inverse;
			if(!tm1.inverse(tm1inverse)) continue;
			Matrix3 transition = tm2 * tm1inverse;

			if(transition.isOrthogonalMatrix()) {
				// Create a new transition between clusters.
				ClusterTransition* t = clusterGraph().createClusterTransition(cluster1, cluster2, transition);
				t->area++;
				t->reverse->area++;
			}
		}
	}

	//qDebug() << "Number of cluster transitions:" << clusterGraph().clusterTransitions().size();

	return !promise.isCanceled();
}

/******************************************************************************
* Combines clusters to super clusters.
******************************************************************************/
bool StructureAnalysis::formSuperClusters(PromiseBase& promise)
{
	size_t oldTransitionCount = clusterGraph().clusterTransitions().size();

	for(size_t clusterIndex = 0; clusterIndex < clusterGraph().clusters().size(); clusterIndex++) {
		Cluster* cluster = clusterGraph().clusters()[clusterIndex];
		cluster->rank = 0;
		if(cluster->id == 0) continue;

		if(promise.isCanceled())
			return false;

		OVITO_ASSERT(cluster->parentTransition == nullptr);
		if(cluster->structure != _inputCrystalType) {
			// Merge defect cluster with a parent lattice cluster.
			ClusterTransition* bestMerge = nullptr;
			for(ClusterTransition* t = cluster->transitions; t != nullptr; t = t->next) {
				if(t->cluster2->structure == _inputCrystalType) {
					OVITO_ASSERT(t->distance == 1);
					if(bestMerge == nullptr || bestMerge->area < t->area) {
						bestMerge = t;
					}
				}
			}

			// Create transition between lattice clusters on both sides of the defect.
			for(ClusterTransition* t1 = cluster->transitions; t1 != nullptr; t1 = t1->next) {
				if(t1->cluster2->structure == _inputCrystalType) {
					OVITO_ASSERT(t1->distance == 1);
					for(ClusterTransition* t2 = t1->next; t2 != nullptr; t2 = t2->next) {
						if(t2->cluster2->structure == _inputCrystalType && t2->cluster2 != t1->cluster2 && t2->distance == 1) {

							// Check if the two clusters form a single crystal.
							const LatticeStructure& latticeStructure = this->latticeStructure(t2->cluster2->structure);
							Matrix3 misorientation = t2->tm * t1->reverse->tm;
							for(const SymmetryPermutation& symElement : latticeStructure.permutations) {
								if(symElement.transformation.equals(misorientation, CA_TRANSITION_MATRIX_EPSILON)) {
									clusterGraph().createClusterTransition(t1->cluster2, t2->cluster2, misorientation, 2);
									break;
								}
							}
						}
					}
				}
			}
		}
	}

	size_t newTransitionCount = clusterGraph().clusterTransitions().size();

	auto getParentGrain = [this](Cluster* c) {
		if(c->parentTransition == nullptr) return c;
		ClusterTransition* newParentTransition = c->parentTransition;
		Cluster* parent = newParentTransition->cluster2;
		while(parent->parentTransition != nullptr) {
			newParentTransition = clusterGraph().concatenateClusterTransitions(newParentTransition, parent->parentTransition);
			parent = parent->parentTransition->cluster2;
		}
		c->parentTransition = newParentTransition;
		return parent;
	};

	// Merge crystal-crystal pairs.
	for(size_t index = oldTransitionCount; index < newTransitionCount; index++) {
		ClusterTransition* t = clusterGraph().clusterTransitions()[index];
		OVITO_ASSERT(t->distance == 2);
		OVITO_ASSERT(t->cluster1->structure == _inputCrystalType && t->cluster2->structure == _inputCrystalType);

		Cluster* parentCluster1 = getParentGrain(t->cluster1);
		Cluster* parentCluster2 = getParentGrain(t->cluster2);
		if(parentCluster1 == parentCluster2) continue;

		if(promise.isCanceled())
			return false;

		ClusterTransition* parentTransition = t;
		if(parentCluster2 != t->cluster2) {
			OVITO_ASSERT(t->cluster2->parentTransition->cluster2 == parentCluster2);
			parentTransition = clusterGraph().concatenateClusterTransitions(parentTransition, t->cluster2->parentTransition);
		}
		if(parentCluster1 != t->cluster1) {
			OVITO_ASSERT(t->cluster1->parentTransition->cluster2 == parentCluster1);
			parentTransition = clusterGraph().concatenateClusterTransitions(t->cluster1->parentTransition->reverse, parentTransition);
		}

		if(parentCluster1->rank > parentCluster2->rank) {
			parentCluster2->parentTransition = parentTransition->reverse;
		}
		else {
			parentCluster1->parentTransition = parentTransition;
			if(parentCluster1->rank == parentCluster2->rank)
				parentCluster2->rank++;
		}
	}

	// Compress paths.
	for(Cluster* cluster : clusterGraph().clusters()) {
		getParentGrain(cluster);
	}

	return !promise.isCanceled();
}

}	// End of namespace
}	// End of namespace
}	// End of namespace

