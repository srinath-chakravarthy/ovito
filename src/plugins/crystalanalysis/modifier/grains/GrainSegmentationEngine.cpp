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
#include <core/utilities/concurrent/ParallelFor.h>
#include <plugins/particles/util/NearestNeighborFinder.h>
#include "GrainSegmentationEngine.h"
#include "GrainSegmentationModifier.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/******************************************************************************
* Constructor.
******************************************************************************/
GrainSegmentationEngine::GrainSegmentationEngine(const TimeInterval& validityInterval,
		ParticleProperty* positions, const SimulationCell& simCell,
		int inputCrystalStructure, FloatType misorientationThreshold,
		FloatType fluctuationTolerance, int minGrainAtomCount) :
	StructureIdentificationModifier::StructureIdentificationEngine(validityInterval, positions, simCell),
	_structureAnalysis(positions, simCell, (StructureAnalysis::LatticeStructureType)inputCrystalStructure, selection(), structures()),
	_inputCrystalStructure(inputCrystalStructure),
	_deformationGradients(new ParticleProperty(positions->size(), qMetaTypeId<FloatType>(), 9, 0, QStringLiteral("Elastic Deformation Gradient"), false)),
	_misorientationThreshold(misorientationThreshold),
	_fluctuationTolerance(fluctuationTolerance),
	_minGrainAtomCount(minGrainAtomCount)
{
	// Set component names of tensor property.
	deformationGradients()->setComponentNames(QStringList() << "XX" << "YX" << "ZX" << "XY" << "YY" << "ZY" << "XZ" << "YZ" << "ZZ");
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void GrainSegmentationEngine::perform()
{
	setProgressText(GrainSegmentationModifier::tr("Performing grain segmentation"));

	beginProgressSubSteps({ 360, 97, 7, 1, 35, 83, 143, 1, 10, 170, 2 });
	if(!_structureAnalysis.identifyStructures(*this))
		return;

	nextProgressSubStep();
	if(!_structureAnalysis.buildClusters(*this))
		return;

	nextProgressSubStep();
	if(!_structureAnalysis.connectClusters(*this))
		return;

	nextProgressSubStep();
	if(!_structureAnalysis.formSuperClusters(*this))
		return;

	nextProgressSubStep();

	// Initialize the working list of grains.
	_grains.resize(positions()->size());

	// Compute the local orientation tensor for all crystalline atoms.
	parallelFor(positions()->size(), *this, [this](size_t particleIndex) {

		Cluster* localCluster = _structureAnalysis.atomCluster(particleIndex);
		if(localCluster->id != 0) {

			Matrix3 idealUnitCellTM = Matrix3::Identity();

			// If the cluster is a defect (stacking fault), find the parent crystal cluster.
			Cluster* parentCluster = nullptr;
			if(localCluster->parentTransition != nullptr) {
				parentCluster = localCluster->parentTransition->cluster2;
				idealUnitCellTM = localCluster->parentTransition->tm;
			}
			else if(localCluster->structure == _inputCrystalStructure) {
				parentCluster = localCluster;
			}

			if(parentCluster != nullptr) {
				OVITO_ASSERT(parentCluster->structure == _inputCrystalStructure);

				// For calculating the cluster orientation.
				Matrix3 orientationV = Matrix3::Zero();
				Matrix3 orientationW = Matrix3::Zero();

				int numneigh = _structureAnalysis.numNeighbors(particleIndex);
				for(int n = 0; n < numneigh; n++) {
					int neighborAtomIndex = _structureAnalysis.getNeighbor(particleIndex, n);
					// Add vector pair to matrices for computing the elastic deformation gradient.
					Vector3 latticeVector = idealUnitCellTM * _structureAnalysis.neighborLatticeVector(particleIndex, n);
					const Vector3& spatialVector = cell().wrapVector(positions()->getPoint3(neighborAtomIndex) - positions()->getPoint3(particleIndex));
					for(size_t i = 0; i < 3; i++) {
						for(size_t j = 0; j < 3; j++) {
							orientationV(i,j) += latticeVector[j] * latticeVector[i];
							orientationW(i,j) += latticeVector[j] * spatialVector[i];
						}
					}
				}

				// Calculate elastic deformation gradient tensor.
				Grain& grain = _grains[particleIndex];
				grain.orientation = orientationW * orientationV.inverse();
				grain.cluster = parentCluster;
				grain.latticeAtomCount = 1;
			}
		}
	});
	if(isCanceled()) return;
	nextProgressSubStep();

	// Build grain graph.
	std::vector<GrainGraphEdge> bulkEdges;
	setProgressRange(_grains.size());
	for(int atomA = 0; atomA < _grains.size(); atomA++) {
		if(!setProgressValueIntermittent(atomA)) return;
		Grain& grainA = _grains[atomA];

		// If the current atom is a crystalline atom recognized by the atomic structure identification algorithm,
		// then we connect it with its neighbors.

		// Skip non-crystalline atoms.
		if(grainA.cluster == nullptr) continue;

		// Iterate over all neighbors of the atom.
		int numNeighbors = _structureAnalysis.numNeighbors(atomA);
		for(int ni = 0; ni < numNeighbors; ni++) {
			// Lookup neighbor atom from neighbor list.
			int atomB = _structureAnalysis.getNeighbor(atomA, ni);

			Grain& grainB = _grains[atomB];
			if(grainB.cluster != nullptr) {
				// This test ensures that we will create only one edge per pair of neighbor atoms.
				if(atomB <= atomA)
					continue;

				// Connect the two atoms with an edge.
				GrainGraphEdge edge = { atomA, atomB, calculateMisorientation(grainA, grainB) };
				bulkEdges.push_back(edge);
			}
			else {
				// Add isolated GB atoms to an adjacent lattice grain.
				if(grainB.parent->cluster > grainA.cluster) {
					grainB.parent->atomCount--;
					grainB.parent = &grainB;
				}
				if(grainB.isRoot())
					grainA.join(grainB);
			}
		}
	}
	nextProgressSubStep();

	// Sort edges in order of ascending misorientation.
	std::sort(bulkEdges.begin(), bulkEdges.end());

	// Merge grains.
	for(const GrainGraphEdge& edge : bulkEdges) {
		mergeTest(parentGrain(edge.a), parentGrain(edge.b), false);
		if(isCanceled()) return;
	}
	for(const GrainGraphEdge& edge : bulkEdges) {
		mergeTest(parentGrain(edge.a), parentGrain(edge.b), true);
		if(isCanceled()) return;
	}
	nextProgressSubStep();

	// Dissolve crystal grains that are too small (i.e. number of atoms below the threshold set by user).
	// Also dissolve grains that consist of stacking fault atoms only.
	for(Grain& atomicGrain : _grains) {
		Grain& rootGrain = parentGrain(atomicGrain);
		if(rootGrain.latticeAtomCount < _minGrainAtomCount || rootGrain.cluster->structure != _inputCrystalStructure) {
			atomicGrain.cluster = nullptr;                    // Dissolve grain.
			atomicGrain.latticeAtomCount = 0;
		}
		else
			atomicGrain.parent = &rootGrain;	// Path compression
	}
	if(isCanceled()) return;
	nextProgressSubStep();

	// Prepare the neighbor list builder.
	NearestNeighborFinder neighborFinder(12);
	if(!neighborFinder.prepare(positions(), cell(), nullptr, this))
		return;

	nextProgressSubStep();
	// Add non-crystalline grain boundary atoms to the grains.
	for(;;) {
		bool done = true;
		boost::dynamic_bitset<> mergedAtoms(positions()->size());
		for(size_t atomA = 0; atomA < positions()->size(); atomA++) {
			if(isCanceled()) return;
			Grain& grainA = parentGrain(atomA);
			if(grainA.cluster != nullptr) {
				int numNeighbors = std::min(12, _structureAnalysis.numNeighbors(atomA));
				for(int ni = 0; ni < numNeighbors; ni++) {
					int atomB = _structureAnalysis.getNeighbor(atomA, ni);

					if(mergedAtoms.test(atomB))
						continue;

					Grain &grainB = parentGrain(atomB);
					if(mergeTest(grainA, grainB, true)) {
						mergedAtoms.set(atomA);
						done = false;
					}
				}
			}
			else {
				NearestNeighborFinder::Query<12> neighQuery(neighborFinder);
				neighQuery.findNeighbors(neighborFinder.particlePos(atomA));
				for(int i = 0; i < neighQuery.results().size(); i++) {
					int atomB = neighQuery.results()[i].index;

					if(mergedAtoms.test(atomB))
						continue;

					Grain &grainB = parentGrain(atomB);
					if(mergeTest(grainA, grainB, true)) {
						mergedAtoms.set(atomA);
						done = false;
					}
				}
			}
		}
		if(done) break;
	}
	nextProgressSubStep();

	// Now assign final contiguous IDs to parent grains.
	_grainCount = assignIdsToGrains();

	qDebug() << "Number of grains:" << _grainCount;

	if(isCanceled()) return;
	for(size_t atomIndex = 0; atomIndex < _grains.size(); atomIndex++) {
		atomClusters()->setInt(atomIndex, parentGrain(atomIndex).id);
	}

	endProgressSubSteps();
}

/******************************************************************************
* Calculates the misorientation angle between two lattice orientations.
******************************************************************************/
FloatType GrainSegmentationEngine::calculateMisorientation(const Grain& grainA, const Grain& grainB, Matrix3* alignmentTM)
{
	Cluster* clusterA = grainA.cluster;
	Cluster* clusterB = grainB.cluster;
	OVITO_ASSERT(clusterA != nullptr && clusterB != nullptr);
	Matrix3 inverseOrientationA = grainA.orientation.inverse();

	if(clusterB == clusterA) {
		if(alignmentTM) alignmentTM->setIdentity();
		return angleFromMatrix(grainB.orientation * inverseOrientationA);
	}
	else if(clusterA->structure == clusterB->structure) {
		const StructureAnalysis::LatticeStructure& latticeStructure = _structureAnalysis.latticeStructure(clusterA->structure);
		FloatType smallestAngle = FLOATTYPE_MAX;
		for(const StructureAnalysis::SymmetryPermutation& sge : latticeStructure.permutations) {
			FloatType angle = angleFromMatrix(grainB.orientation * sge.transformation * inverseOrientationA);
			if(angle < smallestAngle) {
				smallestAngle = angle;
				if(alignmentTM)
					*alignmentTM = sge.transformation;
			}
		}
		return smallestAngle;
	}
	else {
		if(ClusterTransition* t = clusterGraph()->determineClusterTransition(clusterA, clusterB)) {
			if(alignmentTM) *alignmentTM = t->tm;
			return angleFromMatrix(grainB.orientation * t->tm * inverseOrientationA);
		}
		return FLOATTYPE_MAX;
	}
}

/******************************************************************************
* Computes the angle of rotation from a rotation matrix.
******************************************************************************/
FloatType GrainSegmentationEngine::angleFromMatrix(const Matrix3& tm)
{
	FloatType trace = tm(0,0) + tm(1,1) + tm(2,2) - FloatType(1);
	Vector3 axis(tm(2,1) - tm(1,2), tm(0,2) - tm(2,0), tm(1,0) - tm(0,1));
	FloatType angle = atan2(axis.length(), trace);
	if(angle > FLOATTYPE_PI)
		return (FloatType(2) * FLOATTYPE_PI) - angle;
	else
		return angle;
}

/******************************************************************************
* Tests if two grain should be merged and merges them if deemed necessary.
******************************************************************************/
bool GrainSegmentationEngine::mergeTest(Grain& grainA, Grain& grainB, bool allowForFluctuations)
{
	if(&grainA == &grainB)
		return false;
	if(grainA.cluster == nullptr && grainB.cluster == nullptr)
		return false;

	if(grainA.cluster != nullptr && grainB.cluster != nullptr) {
		Matrix3 alignmentTM;
		FloatType misorientation = calculateMisorientation(grainA, grainB, &alignmentTM);

		if(allowForFluctuations)
			misorientation -= _fluctuationTolerance * sqrt(FloatType(1) / FloatType(grainA.latticeAtomCount) + FloatType(1) / FloatType(grainB.latticeAtomCount));

		if(misorientation >= _misorientationThreshold &&
		   grainA.latticeAtomCount >= _minGrainAtomCount && grainB.latticeAtomCount >= _minGrainAtomCount)
			return false;

		// Join the two grains.
		if(grainA.rank > grainB.rank) {
			grainA.join(grainB, alignmentTM);
		}
		else {
			grainB.join(grainA, alignmentTM.inverse());
			if(grainA.rank == grainB.rank)
				grainB.rank++;
		}
	}
	else {
		// Join the crystal grain and the cluster of disordered atoms.
		if(grainA.cluster != nullptr)
			grainA.join(grainB);
		else
			grainB.join(grainA);
	}

	return true;
}

/******************************************************************************
* Assigns contiguous IDs to all parent grains.
******************************************************************************/
size_t GrainSegmentationEngine::assignIdsToGrains()
{
	size_t numGrains = 0;
	for(size_t atomIndex = 0; atomIndex < _grains.size(); atomIndex++) {
		Grain& atomicGrain = _grains[atomIndex];
		Grain& rootGrain = parentGrain(atomIndex);
		if(rootGrain.cluster != nullptr) {
			OVITO_ASSERT(rootGrain.atomCount >= _minGrainAtomCount);
			if(atomicGrain.isRoot()) {
				rootGrain.id = ++numGrains;
			}
		}
		else {
			rootGrain.id = 0;
		}
	}
	return numGrains;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace

