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
#include <plugins/particles/objects/SurfaceMesh.h>
#include <plugins/particles/util/NearestNeighborFinder.h>
#include <plugins/particles/modifier/analysis/cna/CommonNeighborAnalysisModifier.h>
#include <plugins/crystalanalysis/util/DelaunayTessellation.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include "DislocationAnalysisEngine.h"
#include "DislocationAnalysisModifier.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/// Contains the known coordination structures.
DislocationAnalysisEngine::CoordinationStructure DislocationAnalysisEngine::_coordinationStructures[NUM_COORD_TYPES];

/// Contains the known lattice types.
DislocationAnalysisEngine::LatticeStructure DislocationAnalysisEngine::_latticeStructures[NUM_LATTICE_TYPES];


/// Fast sorting function for an array of (bounded) integers.
/// Sorts values in descending order.
template<typename iterator>
void bitmapSort(iterator begin, iterator end, int max)
{
	OVITO_ASSERT(max < 32);
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
DislocationAnalysisEngine::DislocationAnalysisEngine(const TimeInterval& validityInterval, ParticleProperty* positions, const SimulationCell& simCell) :
	AsynchronousParticleModifier::ComputeEngine(validityInterval),
	_positions(positions), _simCell(simCell), _isDefectRegionEverywhere(false), _defectMesh(new HalfEdgeMesh()),
	_structureTypes(new ParticleProperty(positions->size(), ParticleProperty::StructureTypeProperty, 0, true)),
	_neighborLists(new ParticleProperty(positions->size(), qMetaTypeId<int>(), MAX_NEIGHBORS, 0, QStringLiteral("Neighbors"), false)),
	_atomClusters(new ParticleProperty(positions->size(), ParticleProperty::ClusterProperty, 0, true)),
	_atomLatticeVectors(new ParticleProperty(positions->size(), qMetaTypeId<int>(), MAX_NEIGHBORS, 0, QStringLiteral("LatticeVectors"), false))
{
	static bool initialized = false;
	if(!initialized) {
		initializeCoordinationStructures();
		initialized = true;
	}
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void DislocationAnalysisEngine::perform()
{
	setProgressText(DislocationAnalysisModifier::tr("Dislocation analysis: Structure identification step"));

	// Prepare the neighbor list.
	NearestNeighborFinder neighFinder(MAX_NEIGHBORS);
	if(!neighFinder.prepare(positions(), cell(), this))
		return;

	// Identify local structure around each particle.
	parallelFor(positions()->size(), *this, [this, &neighFinder](size_t index) {
		determineLocalStructure(neighFinder, index);
	});

	setProgressText(DislocationAnalysisModifier::tr("Dislocation analysis: Clustering step"));

	buildClusters();
}

/******************************************************************************
* Determines the coordination structure of a particle.
******************************************************************************/
void DislocationAnalysisEngine::determineLocalStructure(NearestNeighborFinder& neighList, size_t particleIndex)
{
	OVITO_ASSERT(_structureTypes->getInt(particleIndex) == COORD_OTHER);

	// Construct local neighbor list builder.
	NearestNeighborFinder::Query<MAX_NEIGHBORS> neighQuery(neighList);

	// Find N nearest neighbors of current atom.
	neighQuery.findNeighbors(neighList.particlePos(particleIndex));
	int numNeighbors = neighQuery.results().size();

	// Number of neighbors to analyze.
	int nn = 12; // For FCC and HCP atoms

	// Early rejection of under-coordinated atoms:
	if(numNeighbors < nn)
		return;

	// Compute local scale factor.
	FloatType localScaling = 0;
	for(int n = 0; n < nn; n++)
		localScaling += sqrt(neighQuery.results()[n].distanceSq);
	localScaling /= nn;
	FloatType localCutoff = localScaling * (1.0f + sqrt(2.0f)) * 0.5f;
	FloatType localCutoffSquared =  localCutoff * localCutoff;

	// Make sure the (N+1)-th atom is beyond the cutoff radius (if it exists).
	if(numNeighbors > nn && neighQuery.results()[nn].distanceSq <= localCutoffSquared)
		return;

	// Compute common neighbor bit-flag array.
	CommonNeighborAnalysisModifier::NeighborBondArray neighborArray;
	for(int ni1 = 0; ni1 < nn; ni1++) {
		neighborArray.setNeighborBond(ni1, ni1, false);
		for(int ni2 = ni1+1; ni2 < nn; ni2++)
			neighborArray.setNeighborBond(ni1, ni2, (neighQuery.results()[ni1].delta - neighQuery.results()[ni2].delta).squaredLength() <= localCutoffSquared);
	}

	int n421 = 0;
	int n422 = 0;
	int cnaSignatures[MAX_NEIGHBORS];
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
	CoordinationStructureType coordinationType;
	if(n421 == 12) {
		coordinationType = COORD_FCC;
	}
	else if(n421 == 6 && n422 == 6) {
		coordinationType = COORD_HCP;
	}
	else return;

	// Initialize permutation.
	int neighborIndices[MAX_NEIGHBORS];
	int previousIndices[MAX_NEIGHBORS];
	for(int n = 0; n < nn; n++) {
		neighborIndices[n] = n;
		previousIndices[n] = -1;
	}

	// Find first matching neighbor permutation.
	const CoordinationStructure& coordStructure = _coordinationStructures[coordinationType];
	for(;;) {

		bool check = false;
		int ni1, ni2;
		for(ni1 = 0; ni1 < nn; ni1++) {
			int atomNeighborIndex1 = neighborIndices[ni1];
			if(atomNeighborIndex1 != previousIndices[ni1]) check = true;
			previousIndices[ni1] = atomNeighborIndex1;
			if(check) {
				if(cnaSignatures[atomNeighborIndex1] != coordStructure.cnaSignatures[ni1])
					break;
				for(ni2 = 0; ni2 < ni1; ni2++) {
					int atomNeighborIndex2 = neighborIndices[ni2];
					if(neighborArray.neighborBond(atomNeighborIndex1, atomNeighborIndex2) != coordStructure.neighborArray.neighborBond(ni1, ni2))
						break;
				}
				if(ni2 != ni1) break;
			}
		}
		if(ni1 == nn) {
			_structureTypes->setInt(particleIndex, coordinationType);
			int* neighborList = _neighborLists->dataInt() + _neighborLists->componentCount() * particleIndex;
			for(int i = 0; i < nn; i++)
				*neighborList++ = neighQuery.results()[neighborIndices[i]].index;
			return;
		}
		bitmapSort(neighborIndices + ni1 + 1, neighborIndices + nn, nn);
		if(!std::next_permutation(neighborIndices, neighborIndices + nn)) {
			// This should not happen.
			OVITO_ASSERT(false);
			return;
		}
	}
}

/******************************************************************************
* Prepares the list of coordination structures.
******************************************************************************/
void DislocationAnalysisEngine::initializeCoordinationStructures()
{
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
			bool bonded = (fccVec[ni1] - fccVec[ni2]).squaredLength() < (sqrt(0.5f)+1.0)*0.5;
			_coordinationStructures[COORD_FCC].neighborArray.setNeighborBond(ni1, ni2, bonded);
		}
		_coordinationStructures[COORD_FCC].cnaSignatures[ni1] = 0;
	}
	_coordinationStructures[COORD_FCC].latticeVectors.assign(std::begin(fccVec), std::end(fccVec));
	_latticeStructures[LATTICE_FCC].latticeVectors.assign(std::begin(fccVec), std::end(fccVec));

	Vector3 hcpVec[12] = {
			Vector3(sqrt(2.0)/4.0, -sqrt(6.0)/4.0, 0.0),
			Vector3(-sqrt(2.0)/4.0, sqrt(6.0)/4.0, 0.0),
			Vector3(-sqrt(2.0)/2.0, 0.0, 0.0),
			Vector3(sqrt(2.0)/4.0, sqrt(6.0)/4.0, 0.0),
			Vector3(sqrt(2.0)/2.0, 0.0, 0.0),
			Vector3(-sqrt(2.0)/4.0, -sqrt(6.0)/4.0, 0.0),
			Vector3(0.0, -sqrt(6.0)/6.0, sqrt(3.0)/3.0),
			Vector3(-sqrt(2.0)/4.0, sqrt(6.0)/12.0, -sqrt(3.0)/3.0),
			Vector3(sqrt(2.0)/4.0, sqrt(6.0)/12.0, sqrt(3.0)/3.0),
			Vector3(0.0, -sqrt(6.0)/6.0, -sqrt(3.0)/3.0),
			Vector3(sqrt(2.0)/4.0, sqrt(6.0)/12.0, -sqrt(3.0)/3.0),
			Vector3(-sqrt(2.0)/4.0, sqrt(6.0)/12.0, sqrt(3.0)/3.0)
	};
	_coordinationStructures[COORD_HCP].numNeighbors = 12;
	for(int ni1 = 0; ni1 < 12; ni1++) {
		_coordinationStructures[COORD_HCP].neighborArray.setNeighborBond(ni1, ni1, false);
		for(int ni2 = ni1 + 1; ni2 < 12; ni2++) {
			bool bonded = (hcpVec[ni1] - hcpVec[ni2]).squaredLength() < (sqrt(0.5)+1.0)*0.5;
			_coordinationStructures[COORD_HCP].neighborArray.setNeighborBond(ni1, ni2, bonded);
		}
		_coordinationStructures[COORD_HCP].cnaSignatures[ni1] = (ni1 < 6) ? 1 : 0;
	}
	_coordinationStructures[COORD_HCP].latticeVectors.assign(std::begin(hcpVec), std::end(hcpVec));
	_latticeStructures[LATTICE_HCP].latticeVectors.assign(std::begin(hcpVec), std::end(hcpVec));
	for(int i = 6; i < 12; i++)
		_latticeStructures[LATTICE_HCP].latticeVectors.push_back(Vector3(hcpVec[i].x(), hcpVec[i].y(), -hcpVec[i].z()));

	_coordinationStructures[COORD_BCC].numNeighbors = 0;

	// Determine two non-coplanar common neighbors for every neighbor.
	for(auto coordStruct = std::begin(_coordinationStructures); coordStruct != std::end(_coordinationStructures); ++coordStruct) {
		for(int neighIndex = 0; neighIndex < coordStruct->numNeighbors; neighIndex++) {
			bool found = false;
			Matrix3 tm;
			tm.column(0) = coordStruct->latticeVectors[neighIndex];
			for(int i1 = 0; i1 < coordStruct->numNeighbors && !found; i1++) {
				if(!coordStruct->neighborArray.neighborBond(neighIndex, i1)) continue;
				tm.column(1) = coordStruct->latticeVectors[i1];
				for(int i2 = i1 + 1; i2 < coordStruct->numNeighbors; i2++) {
					if(!coordStruct->neighborArray.neighborBond(neighIndex, i2)) continue;
					tm.column(2) = coordStruct->latticeVectors[i2];
					if(std::abs(tm.determinant()) > FLOATTYPE_EPSILON) {
						coordStruct->commonNeighbors[neighIndex][0] = neighIndex;
						coordStruct->commonNeighbors[neighIndex][1] = i1;
						coordStruct->commonNeighbors[neighIndex][2] = i2;
						found = true;
						break;
					}
				}
			}
			OVITO_ASSERT(found);
		}
	}
}

/******************************************************************************
* Combines adjacent atoms to clusters.
******************************************************************************/
void DislocationAnalysisEngine::buildClusters()
{
	setProgressValue(0);
	setProgressRange(positions()->size());

	//boost::dynamic_bitset<> visitedAtoms(positions()->size());

	// Continue finding atoms, which haven't been visited yet, and which are not part of a cluster.
	int clusterCount = 0;
	for(size_t seedAtomIndex = 0; seedAtomIndex < positions()->size(); seedAtomIndex++) {
		//if(visitedAtoms.test(seedAtomIndex)) continue;
		if(_atomClusters->getInt(seedAtomIndex) != 0) continue;
		int coordStructureType = _structureTypes->getInt(seedAtomIndex);
		if(coordStructureType == COORD_OTHER) {
			incrementProgressValue();
			continue;
		}

		// Start a new cluster.
		clusterCount++;
		_atomClusters->setInt(seedAtomIndex, clusterCount);
		const CoordinationStructure& coordStructure = _coordinationStructures[coordStructureType];
		int latticeStructureType = coordStructureType;
		const LatticeStructure& latticeStructure = _latticeStructures[latticeStructureType];
		int* atomLatticeVectors = _atomLatticeVectors->dataInt() + seedAtomIndex * _atomLatticeVectors->componentCount();
		for(int i = 0; i < coordStructure.numNeighbors; i++)
			*atomLatticeVectors++ = i;

		// Add neighboring atoms to the cluster.
		std::deque<int> atomsToVisit(1, seedAtomIndex);
		do {
			int currentAtomIndex = atomsToVisit.front();
			atomsToVisit.pop_front();
			incrementProgressValue();
			if(isCanceled()) return;

			const int* neighborList = _neighborLists->dataInt() + currentAtomIndex * _neighborLists->componentCount();
			const int* neighborAtomIndex = neighborList;
			for(int neighborIndex = 0; neighborIndex < coordStructure.numNeighbors; neighborIndex++, ++neighborAtomIndex) {
				if(*neighborAtomIndex == currentAtomIndex)
					throw Exception(DislocationAnalysisModifier::tr("Cannot perform dislocation analysis. Simulation cell is too small. Please extend it using the 'Show periodic images' modifier."));

				// Skip neighbors which are already part of a cluster, or which have a different coordination structure type.
				if(_atomClusters->getInt(*neighborAtomIndex) != 0) continue;
				if(_structureTypes->getInt(*neighborAtomIndex) != coordStructureType) continue;
				int* otherNeighborList = _neighborLists->dataInt() + (*neighborAtomIndex) * _neighborLists->componentCount();

				// Select three non-coplanar atoms, which are all neighbors of the current neighbor.
				// One of the is always the current central atom.
				Matrix3 tm1, tm2;
				for(int i = 0; i < 3; i++) {
					int atomIndex = neighborList[coordStructure.commonNeighbors[neighborIndex][i]];
					int j = std::find(otherNeighborList, otherNeighborList + coordStructure.numNeighbors, atomIndex) - otherNeighborList;
					if(j >= coordStructure.numNeighbors) {
						qDebug() << "i=" << i;
						OVITO_ASSERT(false);
						tm1.column(i).setZero();
						tm2.column(i).setZero();
						continue;
					}
					tm1.column(i) = latticeStructure.latticeVectors[coordStructure.commonNeighbors[neighborIndex][i]];
					tm2.column(i) = latticeStructure.latticeVectors[j];
				}

				// Determine the misorientation matrix.
				OVITO_ASSERT(std::abs(tm1.determinant()) > FLOATTYPE_EPSILON);
				OVITO_ASSERT(std::abs(tm2.determinant()) > FLOATTYPE_EPSILON);
				Matrix3 tm2inverse;
				if(!tm2.inverse(tm2inverse)) continue;
				Matrix3 transition = tm1 * tm2inverse;
			}
		}
		while(!atomsToVisit.empty());
	}
}


}	// End of namespace
}	// End of namespace
}	// End of namespace

