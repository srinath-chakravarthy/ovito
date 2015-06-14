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
StructureAnalysis::StructureAnalysis(ParticleProperty* positions, const SimulationCell& simCell, LatticeStructureType inputCrystalType, ParticleProperty* outputStructures) :
	_positions(positions), _simCell(simCell),
	_inputCrystalType(inputCrystalType),
	_structureTypes(outputStructures),
	_neighborLists(new ParticleProperty(positions->size(), qMetaTypeId<int>(), MAX_NEIGHBORS, 0, QStringLiteral("Neighbors"), false)),
	_neighborCounts(new ParticleProperty(positions->size(), ParticleProperty::CoordinationProperty, 0, true)),
	_atomClusters(new ParticleProperty(positions->size(), ParticleProperty::ClusterProperty, 0, true)),
	_atomSymmetryPermutations(new ParticleProperty(positions->size(), qMetaTypeId<int>(), 1, 0, QStringLiteral("SymmetryPermutations"), false)),
	_clusterGraph(new ClusterGraph())
{
	static bool initialized = false;
	if(!initialized) {
		initializeCoordinationStructures();
		initialized = true;
	}

	// Reset atomic structure types.
	std::fill(_structureTypes->dataInt(), _structureTypes->dataInt() + _structureTypes->size(), LATTICE_OTHER);
}

/******************************************************************************
* Prepares the list of coordination structures.
******************************************************************************/
void StructureAnalysis::initializeCoordinationStructures()
{
	_coordinationStructures[COORD_OTHER].numNeighbors = 0;
	_latticeStructures[LATTICE_OTHER].coordStructure = &_coordinationStructures[COORD_OTHER];

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
			bool bonded = (bccVec[ni1] - bccVec[ni2]).length() < (1.0+sqrt(2.0))*0.5;
			_coordinationStructures[COORD_BCC].neighborArray.setNeighborBond(ni1, ni2, bonded);
		}
		_coordinationStructures[COORD_BCC].cnaSignatures[ni1] = (ni1 < 8) ? 0 : 1;
	}
	_coordinationStructures[COORD_BCC].latticeVectors.assign(std::begin(bccVec), std::end(bccVec));
	_latticeStructures[LATTICE_BCC].latticeVectors.assign(std::begin(bccVec), std::end(bccVec));
	_latticeStructures[LATTICE_BCC].coordStructure = &_coordinationStructures[COORD_BCC];

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
		Matrix3 t;
		do {
			int changedFrom = std::mismatch(permutation.begin(), permutation.end(), lastPermutation.begin()).first - permutation.begin();
			OVITO_ASSERT(changedFrom < coordStruct.numNeighbors);
			std::copy(permutation.begin(), permutation.end(), lastPermutation.begin());
			if(changedFrom <= nindices[2]) {
				Matrix3 tm2;
				tm2.column(0) = latticeStruct->latticeVectors[permutation[nindices[0]]];
				tm2.column(1) = latticeStruct->latticeVectors[permutation[nindices[1]]];
				tm2.column(2) = latticeStruct->latticeVectors[permutation[nindices[2]]];
				t = tm2 * tm1inverse;
				if(!t.isOrthogonalMatrix()) {
					bitmapSort(permutation.begin() + nindices[2] + 1, permutation.end(), permutation.size());
					continue;
				}
				changedFrom = 0;
			}
			int sortFrom = nindices[2];
			int invalidFrom;
			for(invalidFrom = changedFrom; invalidFrom < coordStruct.numNeighbors; invalidFrom++) {
				Vector3 v = t * coordStruct.latticeVectors[invalidFrom];
				if(!v.equals(latticeStruct->latticeVectors[permutation[invalidFrom]]))
					break;
			}
			if(invalidFrom == coordStruct.numNeighbors) {
				std::array<int, MAX_NEIGHBORS> p;
				std::copy(permutation.begin(), permutation.begin() + coordStruct.numNeighbors, p.begin());
				for(const auto& entry : latticeStruct->permutations) {
					OVITO_ASSERT(!entry.first.equals(t));
				}
				latticeStruct->permutations.push_back(std::make_pair(t, p));
			}
			else {
				sortFrom = invalidFrom;
			}
			bitmapSort(permutation.begin() + sortFrom + 1, permutation.end(), permutation.size());
		}
		while(std::next_permutation(permutation.begin(), permutation.end()));
	}
}

/******************************************************************************
* Identifies the atomic structures.
******************************************************************************/
bool StructureAnalysis::identifyStructures(FutureInterfaceBase& progress)
{
	// Prepare the neighbor list.
	NearestNeighborFinder neighFinder(MAX_NEIGHBORS);
	if(!neighFinder.prepare(positions(), cell(), &progress))
		return false;

	// Identify local structure around each particle.
	_maximumNeighborDistance = 0;

	return parallelFor(positions()->size(), progress, [this, &neighFinder](size_t index) {
		determineLocalStructure(neighFinder, index);
	});
}

/******************************************************************************
* Determines the coordination structure of a particle.
******************************************************************************/
void StructureAnalysis::determineLocalStructure(NearestNeighborFinder& neighList, size_t particleIndex)
{
	OVITO_ASSERT(_structureTypes->getInt(particleIndex) == COORD_OTHER);

	// Construct local neighbor list builder.
	NearestNeighborFinder::Query<MAX_NEIGHBORS> neighQuery(neighList);

	// Find N nearest neighbors of current atom.
	neighQuery.findNeighbors(neighList.particlePos(particleIndex));
	int numNeighbors = neighQuery.results().size();
	int neighborIndices[MAX_NEIGHBORS];

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
			neighborArray.setNeighborBond(ni1, ni1, false);
			for(int ni2 = ni1+1; ni2 < nn; ni2++)
				neighborArray.setNeighborBond(ni1, ni2, (neighQuery.results()[ni1].delta - neighQuery.results()[ni2].delta).squaredLength() <= localCutoffSquared);
		}
	}
	else {
		// Generate list of second nearest neighbors.
		std::array<Vector3,16> neighborVectors;
		int outputIndex = 4;
		for(size_t i = 0; i < 4; i++) {
			const Vector3& v0 = neighQuery.results()[i].delta;
			neighborVectors[i] = v0;
			neighborIndices[i] = neighQuery.results()[i].index;
			NearestNeighborFinder::Query<MAX_NEIGHBORS> neighQuery2(neighList);
			neighQuery2.findNeighbors(neighList.particlePos(neighborIndices[i]));
			if(neighQuery2.results().size() < 4) return;
			for(size_t j = 0; j < 4; j++) {
				Vector3 v = v0 + neighQuery2.results()[j].delta;
				if(neighQuery2.results()[j].index == particleIndex) continue;
				if(outputIndex == 16) return;
				neighborIndices[outputIndex] = neighQuery2.results()[j].index;
				neighborVectors[outputIndex] = v;
				neighborArray.setNeighborBond(i, outputIndex, true);
				outputIndex++;
			}
			if(outputIndex != i*3 + 7) return;
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
		if(n421 == 12) {
			coordinationType = COORD_FCC;
		}
		else if(n421 == 6 && n422 == 6) {
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
		if(n543 == 12) {
			coordinationType = COORD_CUBIC_DIAMOND;
		}
		else if(n543 == 6 && n544 == 6) {
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
		bool check = false;
		int ni1, ni2;
		for(ni1 = 0; ni1 < nn; ni1++) {
			int atomNeighborIndex1 = neighborMapping[ni1];
			if(atomNeighborIndex1 != previousMapping[ni1]) check = true;
			previousMapping[ni1] = atomNeighborIndex1;
			if(check) {
				if(cnaSignatures[atomNeighborIndex1] != coordStructure.cnaSignatures[ni1]) {
					break;
				}
				for(ni2 = 0; ni2 < ni1; ni2++) {
					int atomNeighborIndex2 = neighborMapping[ni2];
					if(neighborArray.neighborBond(atomNeighborIndex1, atomNeighborIndex2) != coordStructure.neighborArray.neighborBond(ni1, ni2)) {
						break;
					}
				}
				if(ni2 != ni1) break;
			}
		}
		if(ni1 == nn) {
			// Assign coordination structure type to atom.
			_structureTypes->setInt(particleIndex, coordinationType);

			// Save the atom's neighbor list.
			int* neighborList = _neighborLists->dataInt() + _neighborLists->componentCount() * particleIndex;
			for(int i = 0; i < nn; i++)
				*neighborList++ = neighborIndices[neighborMapping[i]];
			_neighborCounts->setInt(particleIndex, nn);

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
bool StructureAnalysis::buildClusters(FutureInterfaceBase& progress)
{
	progress.setProgressRange(positions()->size());
	int progressCounter = 0;

	// Continue finding atoms, which haven't been visited yet, and which are not part of a cluster.
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
			if(!progress.setProgressValueIntermittent(++progressCounter))
				return false;

			// Look up symmetry permutation of current atom.
			int symmetryPermutationIndex = _atomSymmetryPermutations->getInt(currentAtomIndex);
			const std::array<int, MAX_NEIGHBORS>& permutation = latticeStructure.permutations[symmetryPermutationIndex].second;

			// Visit neighbors of the current atom.
			const int* neighborList = _neighborLists->constDataInt() + currentAtomIndex * _neighborLists->componentCount();
			const int* neighborAtomIndex = neighborList;
			for(int neighborIndex = 0; neighborIndex < coordStructure.numNeighbors; neighborIndex++, ++neighborAtomIndex) {

				// An atom should not be a neighbor of itself.
				// We use the minimum image convention for simulation cells with periodic boundary conditions.
				if(*neighborAtomIndex == currentAtomIndex)
					throw Exception(DislocationAnalysisModifier::tr("Cannot perform dislocation analysis. Simulation cell is too small. Please extend it first using the 'Show periodic images' modifier."));

				// Add vector pair to matrices for computing the cluster orientation.
				const Vector3& latticeVector = latticeStructure.latticeVectors[permutation[neighborIndex]];
				const Vector3& spatialVector = cell().wrapVector(positions()->getPoint3(*neighborAtomIndex) - positions()->getPoint3(currentAtomIndex));
				for(size_t i = 0; i < 3; i++) {
					for(size_t j = 0; j < 3; j++) {
						orientationV(i,j) += (double)(latticeVector[j] * latticeVector[i]);
						orientationW(i,j) += (double)(latticeVector[j] * spatialVector[i]);
					}
				}

				// Skip neighbors which are already part of the cluster, or which have a different coordination structure type.
				if(_atomClusters->getInt(*neighborAtomIndex) != 0) continue;
				if(_structureTypes->getInt(*neighborAtomIndex) != coordStructureType) continue;

				// Get neighbor list of neighboring atom.
				const int* otherNeighborList = _neighborLists->constDataInt() + (*neighborAtomIndex) * _neighborLists->componentCount();

				// Select three non-coplanar atoms, which are all neighbors of the current neighbor.
				// One of them is the current central atom, two are common neighbors.
				Matrix3 tm1, tm2;
				bool properOverlap = true;
				for(int i = 0; i < 3; i++) {
					int atomIndex;
					if(i != 2) {
						atomIndex = neighborList[coordStructure.commonNeighbors[neighborIndex][i]];
						tm1.column(i) = latticeStructure.latticeVectors[permutation[coordStructure.commonNeighbors[neighborIndex][i]]] - latticeStructure.latticeVectors[permutation[neighborIndex]];
					}
					else {
						atomIndex = currentAtomIndex;
						tm1.column(i) = -latticeStructure.latticeVectors[permutation[neighborIndex]];
					}
					int j = std::find(otherNeighborList, otherNeighborList + coordStructure.numNeighbors, atomIndex) - otherNeighborList;
					if(j >= coordStructure.numNeighbors) {
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
				OVITO_ASSERT(std::abs(tm2.determinant()) > FLOATTYPE_EPSILON);
				Matrix3 tm2inverse;
				if(!tm2.inverse(tm2inverse)) continue;
				Matrix3 transition = tm1 * tm2inverse;

				// Find the corresponding symmetry permutation.
				for(int i = 0; i < latticeStructure.permutations.size(); i++) {
					if(transition.equals(latticeStructure.permutations[i].first)) {

						// Make the neighbor atom part of the current cluster.
						_atomClusters->setInt(*neighborAtomIndex, cluster->id);
						cluster->atomCount++;

						// Save the permutation index.
						_atomSymmetryPermutations->setInt(*neighborAtomIndex, i);

						// Recursively continue with the neighbor.
						atomsToVisit.push_back(*neighborAtomIndex);

						break;
					}
				}
			}
		}
		while(!atomsToVisit.empty());

		// Compute matrix, which transforms vectors from lattice space to simulation coordinates.
		cluster->orientation = Matrix3(orientationW * orientationV.inverse());
	}

	qDebug() << "Number of clusters:" << (clusterGraph().clusters().size() - 1);

	return true;
}

/******************************************************************************
* Determines the transition matrices between clusters.
******************************************************************************/
bool StructureAnalysis::connectClusters(FutureInterfaceBase& progress)
{
	progress.setProgressRange(positions()->size());

	for(size_t atomIndex = 0; atomIndex < positions()->size(); atomIndex++) {
		int clusterId = _atomClusters->getInt(atomIndex);
		if(clusterId == 0) continue;
		Cluster* cluster1 = clusterGraph().findCluster(clusterId);
		OVITO_ASSERT(cluster1);

		// Update progress indicator.
		if(!progress.setProgressValueIntermittent(atomIndex))
			return false;

		// Look up symmetry permutation of current atom.
		int structureType = _structureTypes->getInt(atomIndex);
		const LatticeStructure& latticeStructure = _latticeStructures[structureType];
		const CoordinationStructure& coordStructure = _coordinationStructures[structureType];
		int symmetryPermutationIndex = _atomSymmetryPermutations->getInt(atomIndex);
		const std::array<int, MAX_NEIGHBORS>& permutation = latticeStructure.permutations[symmetryPermutationIndex].second;

		// Visit neighbors of the current atom.
		const int* neighborList = _neighborLists->constDataInt() + atomIndex * _neighborLists->componentCount();
		for(int ni = 0; ni < coordStructure.numNeighbors; ni++) {
			int neighbor = neighborList[ni];

			// Skip neighbor atoms belonging to the same cluster or to no cluster at all.
			int neighborClusterId = _atomClusters->getInt(neighbor);
			if(neighborClusterId == 0 || neighborClusterId == clusterId) {

				// Add this atom to the neighbor's list of neighbors.
				if(neighborClusterId == 0) {
					int* otherNeighborList = _neighborLists->dataInt() + neighbor * _neighborLists->componentCount();
					int otherNeighborListCount = _neighborCounts->getInt(neighbor);
					if(otherNeighborListCount < _neighborLists->componentCount()) {
						otherNeighborList[otherNeighborListCount++] = atomIndex;
						_neighborCounts->setInt(neighbor, otherNeighborListCount);
					}
				}

				continue;
			}
			Cluster* cluster2 = clusterGraph().findCluster(neighborClusterId);
			OVITO_ASSERT(cluster2);

			// Skip if there is already a transition between the two clusters.
			if(cluster1->findTransition(cluster2))
				continue;

			// Get neighbor list of neighboring atom.
			const int* otherNeighborList = _neighborLists->constDataInt() + neighbor * _neighborLists->componentCount();

			// Select three non-coplanar atoms, which are all neighbors of the current neighbor.
			// One of them is the current central atom, two are common neighbors.
			Matrix3 tm1, tm2;
			bool properOverlap = true;
			for(int i = 0; i < 3; i++) {
				int ai;
				if(i != 2) {
					ai = neighborList[coordStructure.commonNeighbors[ni][i]];
					tm1.column(i) = latticeStructure.latticeVectors[permutation[coordStructure.commonNeighbors[ni][i]]] - latticeStructure.latticeVectors[permutation[ni]];
				}
				else {
					ai = atomIndex;
					tm1.column(i) = -latticeStructure.latticeVectors[permutation[ni]];
				}
				int j = std::find(otherNeighborList, otherNeighborList + coordStructure.numNeighbors, ai) - otherNeighborList;
				if(j >= coordStructure.numNeighbors) {
					properOverlap = false;
					break;
				}

				// Look up symmetry permutation of neighbor atom.
				int neighborStructureType = _structureTypes->getInt(neighbor);
				const LatticeStructure& neighborLatticeStructure = _latticeStructures[neighborStructureType];
				int neighborSymmetryPermutationIndex = _atomSymmetryPermutations->getInt(neighbor);
				const std::array<int, MAX_NEIGHBORS>& neighborPermutation = neighborLatticeStructure.permutations[neighborSymmetryPermutationIndex].second;

				tm2.column(i) = neighborLatticeStructure.latticeVectors[neighborPermutation[j]];
			}
			if(!properOverlap)
				continue;

			// Determine the misorientation matrix.
			OVITO_ASSERT(std::abs(tm1.determinant()) > FLOATTYPE_EPSILON);
			OVITO_ASSERT(std::abs(tm2.determinant()) > FLOATTYPE_EPSILON);
			Matrix3 tm1inverse;
			if(!tm1.inverse(tm1inverse)) continue;
			Matrix3 transition = tm2 * tm1inverse;

			if(transition.isOrthogonalMatrix()) {
				// Create a new transition between clusters.
				clusterGraph().createClusterTransition(cluster1, cluster2, transition);
			}
		}
	}

	qDebug() << "Number of cluster transitions:" << clusterGraph().clusterTransitions().size();

	return true;
}


}	// End of namespace
}	// End of namespace
}	// End of namespace

