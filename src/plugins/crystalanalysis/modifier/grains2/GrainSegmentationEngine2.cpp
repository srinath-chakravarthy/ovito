///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
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
#include <plugins/crystalanalysis/util/DelaunayTessellation.h>
#include <plugins/crystalanalysis/util/ManifoldConstructionHelper.h>
#include "GrainSegmentationEngine2.h"
#include "GrainSegmentationModifier2.h"

#include <ptm/index_ptm.h>
#include <ptm/qcprot/quat.hpp>

#include <boost/range/algorithm/fill.hpp>
#include <boost/range/algorithm/replace.hpp>
#include <boost/range/algorithm/count.hpp>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/******************************************************************************
* Constructor.
******************************************************************************/
GrainSegmentationEngine2::GrainSegmentationEngine2(const TimeInterval& validityInterval,
		ParticleProperty* positions, const SimulationCell& simCell,
		const QVector<bool>& typesToIdentify, ParticleProperty* selection,
		int inputCrystalStructure, FloatType rmsdCutoff, int numOrientationSmoothingIterations,
		FloatType orientationSmoothingWeight, FloatType misorientationThreshold, int minGrainAtomCount,
		FloatType probeSphereRadius, int meshSmoothingLevel) :
	StructureIdentificationModifier::StructureIdentificationEngine(validityInterval, positions, simCell, typesToIdentify, selection),
	_atomClusters(new ParticleProperty(positions->size(), ParticleProperty::ClusterProperty, 0, true)),
	_rmsd(new ParticleProperty(positions->size(), qMetaTypeId<FloatType>(), 1, 0, GrainSegmentationModifier2::tr("RMSD"), false)),
	_rmsdCutoff(rmsdCutoff),
	_inputCrystalStructure(inputCrystalStructure),
	_numOrientationSmoothingIterations(numOrientationSmoothingIterations),
	_orientationSmoothingWeight(orientationSmoothingWeight),
	_orientations(new ParticleProperty(positions->size(), ParticleProperty::OrientationProperty, 0, true)),
	_misorientationThreshold(misorientationThreshold),
	_minGrainAtomCount(minGrainAtomCount),
	_probeSphereRadius(probeSphereRadius),
	_meshSmoothingLevel(meshSmoothingLevel),
	_latticeNeighborBonds(new BondsStorage()),
	_neighborDisorientationAngles(new BondProperty(0, qMetaTypeId<FloatType>(), 1, 0, GrainSegmentationModifier2::tr("Disorientation"), false)),
	_defectDistances(new ParticleProperty(positions->size(), qMetaTypeId<int>(), 1, 0, GrainSegmentationModifier2::tr("Defect distance"), false)),
	_defectDistanceMaxima(new ParticleProperty(positions->size(), qMetaTypeId<int>(), 1, 0, GrainSegmentationModifier2::tr("Distance transform maxima"), true)),
	_vertexColors(new ParticleProperty(positions->size(), qMetaTypeId<int>(), 1, 0, GrainSegmentationModifier2::tr("Vertex color"), true)),
	_edgeCapacity(new BondProperty(0, qMetaTypeId<FloatType>(), 1, 0, GrainSegmentationModifier2::tr("Capacity"), true)),
	_residualEdgeCapacity(new BondProperty(0, qMetaTypeId<FloatType>(), 1, 0, GrainSegmentationModifier2::tr("Residual capacity"), true))
{
	// Allocate memory for neighbor lists.
	_neighborLists = new ParticleProperty(positions->size(), qMetaTypeId<int>(), PTM_MAX_NBRS, 0, QStringLiteral("Neighbors"), false);
	std::fill(_neighborLists->dataInt(), _neighborLists->dataInt() + _neighborLists->size() * _neighborLists->componentCount(), -1);
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void GrainSegmentationEngine2::perform()
{
	setProgressText(GrainSegmentationModifier2::tr("Performing grain segmentation"));

	// Prepare the neighbor list.
	NearestNeighborFinder neighFinder(MAX_NEIGHBORS);
	if(!neighFinder.prepare(positions(), cell(), selection(), this))
		return;

	// Create output storage.
	ParticleProperty* output = structures();

	setProgressRange(positions()->size());
	setProgressValue(0);

	// Perform analysis on each particle.
	setProgressText(GrainSegmentationModifier2::tr("Grain segmentation - structure identification"));
	parallelForChunks(positions()->size(), *this, [this, &neighFinder, output](size_t startIndex, size_t count, FutureInterfaceBase& progress) {

		// Initialize thread-local storage for PTM routine.
		ptm_local_handle_t ptm_local_handle = ptm_initialize_local();

		size_t endIndex = startIndex + count;
		for(size_t index = startIndex; index < endIndex; index++) {

			// Update progress indicator.
			if((index % 256) == 0)
				progress.incrementProgressValue(256);

			// Break out of loop when operation was canceled.
			if(progress.isCanceled())
				break;

			// Skip particles that are not included in the analysis.
			if(selection() && !selection()->getInt(index)) {
				output->setInt(index, OTHER);
				//_rmsd->setFloat(index, 0);
				continue;
			}

			// Find nearest neighbors.
			NearestNeighborFinder::Query<MAX_NEIGHBORS> neighQuery(neighFinder);
			neighQuery.findNeighbors(neighFinder.particlePos(index));
			int numNeighbors = neighQuery.results().size();
			OVITO_ASSERT(numNeighbors <= MAX_NEIGHBORS);

			// Bring neighbor coordinates into a form suitable for the PTM library.
			double points[(MAX_NEIGHBORS+1) * 3];
			points[0] = points[1] = points[2] = 0;
			for(int i = 0; i < numNeighbors; i++) {
				points[i*3 + 3] = neighQuery.results()[i].delta.x();
				points[i*3 + 4] = neighQuery.results()[i].delta.y();
				points[i*3 + 5] = neighQuery.results()[i].delta.z();
			}

			// Determine which structures to look for. This depends on how
			// much neighbors are present.
			int32_t flags = 0;
			if(numNeighbors >= 6 && typesToIdentify()[SC]) flags |= PTM_CHECK_SC;
			if(numNeighbors >= 12) {
				if(typesToIdentify()[FCC]) flags |= PTM_CHECK_FCC;
				if(typesToIdentify()[HCP]) flags |= PTM_CHECK_HCP;
				if(typesToIdentify()[ICO]) flags |= PTM_CHECK_ICO;
			}
			if(numNeighbors >= 14 && typesToIdentify()[BCC]) flags |= PTM_CHECK_BCC;

			// Call PTM library to identify local structure.
			int32_t type, alloy_type;
			double scale;
			double rmsd;
			double q[4];
			int8_t mapping[PTM_MAX_NBRS + 1];
			ptm_index(ptm_local_handle, numNeighbors + 1, points, nullptr, flags, true,
					&type, &alloy_type, &scale, &rmsd, q,
					nullptr, nullptr,
					nullptr, nullptr, mapping, nullptr, nullptr);

			// Convert PTM classification to our own scheme and store computed quantities.
			if(type == PTM_MATCH_NONE) {
				output->setInt(index, OTHER);
				_rmsd->setFloat(index, 0);
			}
			else {
				if(type == PTM_MATCH_SC) output->setInt(index, SC);
				else if(type == PTM_MATCH_FCC) output->setInt(index, FCC);
				else if(type == PTM_MATCH_HCP) output->setInt(index, HCP);
				else if(type == PTM_MATCH_ICO) output->setInt(index, ICO);
				else if(type == PTM_MATCH_BCC) output->setInt(index, BCC);
				else OVITO_ASSERT(false);
				_rmsd->setFloat(index, rmsd);
				_orientations->setQuaternion(index, Quaternion((FloatType)q[1], (FloatType)q[2], (FloatType)q[3], (FloatType)q[0]));

				// Store neighbor list.
				for(int j = 0; j < ptm_num_nbrs[type]; j++) {
					OVITO_ASSERT(j < _neighborLists->componentCount());
					OVITO_ASSERT(mapping[j + 1] >= 1);
					OVITO_ASSERT(mapping[j + 1] <= numNeighbors);
					_neighborLists->setIntComponent(index, j, neighQuery.results()[mapping[j + 1] - 1].index);

					const Vector3& neighborVector = neighQuery.results()[mapping[j + 1] - 1].delta;
					// Check if neighbor vector spans more than half of a periodic simulation cell.
					for(size_t dim = 0; dim < 3; dim++) {
						if(cell().pbcFlags()[dim]) {
							if(std::abs(cell().inverseMatrix().prodrow(neighborVector, dim)) >= FloatType(0.5)+FLOATTYPE_EPSILON) {
								static const QString axes[3] = { QStringLiteral("X"), QStringLiteral("Y"), QStringLiteral("Z") };
								throw Exception(GrainSegmentationModifier2::tr("Simulation box is too short along cell vector %1 (%2) to perform analysis. "
										"Please extend it first using the 'Show periodic images' modifier.").arg(dim+1).arg(axes[dim]));
							}
						}
					}
				}
			}
		}

		// Release thread-local storage of PTM routine.
		ptm_uninitialize_local(ptm_local_handle);
	});
	if(isCanceled() || output->size() == 0)
		return;

	// Determine histogram bin size based on maximum RMSD value.
	_rmsdHistogramData.resize(100);
	_rmsdHistogramBinSize = *std::max_element(_rmsd->constDataFloat(), _rmsd->constDataFloat() + output->size()) * 1.01f;
	_rmsdHistogramBinSize /= _rmsdHistogramData.size();
	if(_rmsdHistogramBinSize <= 0) _rmsdHistogramBinSize = 1;

	// Build RMSD histogram.
	for(size_t index = 0; index < output->size(); index++) {
		if(output->getInt(index) != OTHER) {
			OVITO_ASSERT(_rmsd->getFloat(index) >= 0);
			int binIndex = _rmsd->getFloat(index) / _rmsdHistogramBinSize;
			if(binIndex < _rmsdHistogramData.size())
				_rmsdHistogramData[binIndex]++;
		}
	}

	// Apply RMSD cutoff.
	if(_rmsdCutoff > 0) {
		for(size_t index = 0; index < output->size(); index++) {
			if(output->getInt(index) != OTHER) {
				if(_rmsd->getFloat(index) > _rmsdCutoff)
					output->setInt(index, OTHER);
			}
		}
	}

	// Lattice orientation smoothing.
	setProgressText(GrainSegmentationModifier2::tr("Grain segmentation - orientation smoothing"));
	setProgressRange(_numOrientationSmoothingIterations);
	QExplicitlySharedDataPointer<ParticleProperty> newOrientations(new ParticleProperty(positions()->size(), ParticleProperty::OrientationProperty, 0, false));
	for(int iter = 0; iter < _numOrientationSmoothingIterations; iter++) {
		setProgressValue(iter);
		for(size_t index = 0; index < output->size(); index++) {
			if(isCanceled()) return;

			int structureType = output->getInt(index);
			if(structureType != OTHER) {

				Quaternion& qavg = newOrientations->dataQuaternion()[index];
				qavg = Quaternion(0,0,0,0);

				const Quaternion& orient0 = _orientations->getQuaternion(index);
				double q0[4] = {orient0.w(), orient0.x(), orient0.y(), orient0.z()};
				double qinv[4] = {-q0[0], q0[1], q0[2], q0[3]};

				int nnbr = 0;
				for(size_t c = 0; c < _neighborLists->componentCount(); c++) {
					int neighborIndex = _neighborLists->getIntComponent(index, c);
					if(neighborIndex == -1) break;

					if(output->getInt(neighborIndex) != structureType) continue;

					const Quaternion& orient_nbr = _orientations->getQuaternion(neighborIndex);
					double qnbr[4] = {orient_nbr.w(), orient_nbr.x(), orient_nbr.y(), orient_nbr.z()};
					double qrot[4];
					quat_rot(qinv, qnbr, qrot);

					if (structureType == SC || structureType == FCC || structureType == BCC)
						rotate_quaternion_into_cubic_fundamental_zone(qrot);
					else if (structureType == HCP)
						rotate_quaternion_into_hcp_fundamental_zone(qrot);

					double qclosest[4];
					quat_rot(q0, qrot, qclosest);
					double theta = quat_misorientation(q0, qclosest);
					if(theta < 10 * M_PI / 180.0) {
						qavg.w() += (FloatType)qclosest[0];
						qavg.x() += (FloatType)qclosest[1];
						qavg.y() += (FloatType)qclosest[2];
						qavg.z() += (FloatType)qclosest[3];
						nnbr++;
					}
				}

				if(nnbr != 0)
					qavg.normalize();
				for(size_t i = 0; i < 4; i++)
					qavg[i] = orient0[i] + _orientationSmoothingWeight * qavg[i];
				qavg.normalize();
			}
			else {
				newOrientations->setQuaternion(index, _orientations->getQuaternion(index));
			}
		}
		newOrientations.swap(_orientations);
	}

	// Initialize distance transform calculation.
	boost::fill(defectDistances()->intRange(), 0);

	// Generate bonds (edges) between neighboring lattice atoms.
	setProgressText(GrainSegmentationModifier2::tr("Grain segmentation - edge generation"));
	setProgressValue(0);
	setProgressRange(output->size());
	for(size_t index = 0; index < output->size(); index++) {
		if(!incrementProgressValue()) return;
		int structureType = output->getInt(index);
		if(structureType != OTHER) {
			for(size_t c = 0; c < _neighborLists->componentCount(); c++) {
				int neighborIndex = _neighborLists->getIntComponent(index, c);
				if(neighborIndex == -1) break;

				// Only create bonds between likewise neighbors.
				if(output->getInt(neighborIndex) != structureType) {

					// Mark this atom as border atom for the distance transform calculation, because
					// it has a non-lattice atom as neighbor.
					defectDistances()->setInt(index, 1);
					continue;
				}

				// Skip every other half-bond, because we create two half-bonds below.
				if(positions()->getPoint3(index) > positions()->getPoint3(neighborIndex))
					continue;

				// Determine PBC bond shift using minimum image convention.
				Vector3 delta = positions()->getPoint3(index) - positions()->getPoint3(neighborIndex);
				Vector_3<int8_t> pbcShift = Vector_3<int8_t>::Zero();
				for(size_t dim = 0; dim < 3; dim++) {
					if(cell().pbcFlags()[dim])
						pbcShift[dim] = (int8_t)floor(cell().inverseMatrix().prodrow(delta, dim) + FloatType(0.5));
				}

				// Create two half-bonds.
				_latticeNeighborBonds->push_back(Bond{ pbcShift, (unsigned int)index, (unsigned int)neighborIndex });
				_latticeNeighborBonds->push_back(Bond{ -pbcShift, (unsigned int)neighborIndex, (unsigned int)index });
			}
		}
	}

	// Compute disorientation angles of edges.
	setProgressText(GrainSegmentationModifier2::tr("Grain segmentation - misorientation calculation"));
	setProgressValue(0);
	setProgressRange(_latticeNeighborBonds->size());
	_neighborDisorientationAngles->resize(_latticeNeighborBonds->size(), false);
	auto disorientationAngleIter = _neighborDisorientationAngles->dataFloat();
	for(const Bond& bond : *_latticeNeighborBonds) {
		if(!incrementProgressValue()) return;

		const Quaternion& qA = _orientations->getQuaternion(bond.index1);
		const Quaternion& qB = _orientations->getQuaternion(bond.index2);

		int structureType = output->getInt(bond.index1);
		double orientA[4] = { qA.w(), qA.x(), qA.y(), qA.z() };
		double orientB[4] = { qB.w(), qB.x(), qB.y(), qB.z() };
		if(structureType == SC || structureType == FCC || structureType == BCC)
			*disorientationAngleIter = (FloatType)quat_disorientation_cubic(orientA, orientB);
		else if(structureType == HCP)
			*disorientationAngleIter = (FloatType)quat_disorientation_hcp(orientA, orientB);
		else
			*disorientationAngleIter = FLOATTYPE_MAX;

		// Lattice atoms that possess a high disorientation edge are treated like defects
		// when computing the distance transform.
		if(*disorientationAngleIter > _misorientationThreshold) {
			defectDistances()->setInt(bond.index1, 1);
			defectDistances()->setInt(bond.index2, 1);
		}

		++disorientationAngleIter;
	}

	setProgressText(GrainSegmentationModifier2::tr("Grain segmentation - distance transform"));
	setProgressRange(0);

	// This is used in the following for fast lookup of bonds incident on an atom.
	ParticleBondMap bondMap(*_latticeNeighborBonds);

	// Build initial list of border atoms (distance==1).
	std::vector<size_t> distanceSortedAtoms;
	for(size_t particleIndex = 0; particleIndex < output->size(); particleIndex++) {
		if(defectDistances()->getInt(particleIndex) == 1)
			distanceSortedAtoms.push_back(particleIndex);
	}

	// Distance transform calculation.
	bool done;
	size_t lastCount = 0;
	for(int currentDistance = 2; ; currentDistance++) {
		size_t currentCount = distanceSortedAtoms.size();
		for(size_t i = lastCount; i < currentCount; i++) {
			if(isCanceled()) return;
			for(size_t bondIndex : bondMap.bondsOfParticle(distanceSortedAtoms[i])) {
				const Bond& bond = (*_latticeNeighborBonds)[bondIndex];
				if(defectDistances()->getInt(bond.index2) == 0) {
					defectDistances()->setInt(bond.index2, currentDistance);
					distanceSortedAtoms.push_back(bond.index2);
				}
			}
		}
		if(distanceSortedAtoms.size() == currentCount)
			break;
		lastCount = currentCount;
	}

	// Create clusters by gradually filling up the distance transform basins.
	int numClusters = 0;
	std::deque<size_t> currentQueue;
	for(auto seedAtomIndex = distanceSortedAtoms.rbegin(); seedAtomIndex != distanceSortedAtoms.rend(); ++seedAtomIndex) {

		// First check if atom is a really a seed atom, i.e. it's not already part of a cluster.
		if(atomClusters()->getInt(*seedAtomIndex) != 0)
			continue;
		int currentDistance = defectDistances()->getInt(*seedAtomIndex);

		// Expand existing clusters up to the current water level.
		while(!currentQueue.empty()) {
			size_t currentParticle = currentQueue.front();
			if(defectDistances()->getInt(currentParticle) < currentDistance)
				break;
			currentQueue.pop_front();

			int clusterID = atomClusters()->getInt(currentParticle);
			for(size_t bondIndex : bondMap.bondsOfParticle(currentParticle)) {
				const Bond& bond = (*_latticeNeighborBonds)[bondIndex];
				if(atomClusters()->getInt(bond.index2) != 0) continue;
				if(_neighborDisorientationAngles->getFloat(bondIndex) > _misorientationThreshold) continue;

				atomClusters()->setInt(bond.index2, clusterID);
				if(defectDistances()->getInt(bond.index2) >= currentDistance) {
					// Put atoms that are below the current water level to the front of the queue.
					currentQueue.push_front(bond.index2);
				}
				else {
					// Put atoms that are just above the current water level to the end of the queue.
					currentQueue.push_back(bond.index2);
				}
			}
		}

		// Start a new cluster, unless atom has already become part of an existing cluster in the meantime.
		if(atomClusters()->getInt(*seedAtomIndex) == 0) {
			currentQueue.push_front(*seedAtomIndex);
			atomClusters()->setInt(*seedAtomIndex, ++numClusters);
		}
	}
	OVITO_ASSERT(currentQueue.size() <= 1);
	qDebug() << "Initial number of clusters:" << numClusters;

	// Calculate average orientation of each cluster.
	std::vector<QuaternionT<double>> clusterOrientations(numClusters, QuaternionT<double>(0,0,0,0));
	std::vector<int> firstClusterAtom(numClusters, -1);
	std::vector<int> clusterSizes(numClusters, 0);
	for(size_t particleIndex = 0; particleIndex < output->size(); particleIndex++) {
		int clusterId = _atomClusters->getInt(particleIndex);
		if(clusterId == 0) continue;

		// Cluster IDs start at 1. Need to subtract 1 to get cluster index.
		clusterId--;

		clusterSizes[clusterId]++;
		if(firstClusterAtom[clusterId] == -1)
			firstClusterAtom[clusterId] = particleIndex;

		const Quaternion& orient0 = _orientations->getQuaternion(firstClusterAtom[clusterId]);
		const Quaternion& orient = _orientations->getQuaternion(particleIndex);

		double q0[4] = { orient0.w(), orient0.x(), orient0.y(), orient0.z() };
		double qinv[4] = {-q0[0], q0[1], q0[2], q0[3]};
		double qnbr[4] = { orient.w(), orient.x(), orient.y(), orient.z() };
		double qrot[4];
		quat_rot(qinv, qnbr, qrot);

		int structureType = output->getInt(particleIndex);
		if(structureType == SC || structureType == FCC || structureType == BCC)
			rotate_quaternion_into_cubic_fundamental_zone(qrot);
		else if(structureType == HCP)
			rotate_quaternion_into_hcp_fundamental_zone(qrot);

		double qclosest[4];
		quat_rot(q0, qrot, qclosest);

		QuaternionT<double>& qavg = clusterOrientations[clusterId];
		qavg.w() += qclosest[0];
		qavg.x() += qclosest[1];
		qavg.y() += qclosest[2];
		qavg.z() += qclosest[3];
	}
	for(auto& qavg : clusterOrientations) {
		OVITO_ASSERT(qavg != QuaternionT<double>(0,0,0,0));
		qavg.normalize();
	}

#if 0
	// Dissolve grains that are too small (i.e. number of atoms below the threshold set by user).
	std::vector<int> clusterRemapping(numClusters);
	int newNumClusters = 0;
	for(int i = 0; i < numClusters; i++) {
		if(clusterSizes[i] >= _minGrainAtomCount) {
			clusterSizes[newNumClusters] = clusterSizes[i];
			clusterOrientations[newNumClusters] = clusterOrientations[i];
			clusterRemapping[i] = ++newNumClusters;
		}
		else {
			clusterRemapping[i] = 0;
		}
	}

	// Compress cluster ID range and relabel atoms.
	numClusters = newNumClusters;
	clusterSizes.resize(newNumClusters);
	clusterOrientations.resize(newNumClusters);
	for(size_t particleIndex = 0; particleIndex < output->size(); particleIndex++) {
		int clusterId = _atomClusters->getInt(particleIndex);
		if(clusterId == 0) continue;
		clusterId = clusterRemapping[clusterId - 1];
		_atomClusters->setInt(particleIndex, clusterId);
	}
#endif

	// For output, convert edge disorientation angles from radians to degrees.
	for(FloatType& angle : _neighborDisorientationAngles->floatRange())
		angle *= FloatType(180) / FLOATTYPE_PI;
}

/** Find the most common element in the [first, last) range.

	O(n) in time; O(1) in space.

	[first, last) must be valid sorted range.
	Elements must be equality comparable.
*/
template <class ForwardIterator>
ForwardIterator most_common(ForwardIterator first, ForwardIterator last)
{
	ForwardIterator it(first), max_it(first);
	size_t count = 0, max_count = 0;
	for( ; first != last; ++first) {
		if(*it == *first)
			count++;
		else {
			it = first;
			count = 1;
		}
		if(count > max_count) {
			max_count = count;
			max_it = it;
		}
	}
	return max_it;
}

/******************************************************************************
* Builds the triangle mesh for the grain boundaries.
******************************************************************************/
bool GrainSegmentationEngine2::buildPartitionMesh()
{
	double alpha = _probeSphereRadius * _probeSphereRadius;
	FloatType ghostLayerSize = _probeSphereRadius * 3.0f;

	// Check if combination of radius parameter and simulation cell size is valid.
	for(size_t dim = 0; dim < 3; dim++) {
		if(cell().pbcFlags()[dim]) {
			int stencilCount = (int)ceil(ghostLayerSize / cell().matrix().column(dim).dot(cell().cellNormalVector(dim)));
			if(stencilCount > 1)
				throw Exception(GrainSegmentationModifier2::tr("Cannot generate Delaunay tessellation. Simulation cell is too small or probe sphere radius parameter is too large."));
		}
	}

	_mesh = new PartitionMeshData();

	// If there are too few particles, don't build Delaunay tessellation.
	// It is going to be invalid anyway.
	size_t numInputParticles = positions()->size();
	if(selection())
		numInputParticles = positions()->size() - std::count(selection()->constDataInt(), selection()->constDataInt() + selection()->size(), 0);
	if(numInputParticles <= 3)
		return true;

	// The algorithm is divided into several sub-steps.
	// Assign weights to sub-steps according to estimated runtime.
	beginProgressSubSteps({ 20, 10, 1 });

	// Generate Delaunay tessellation.
	DelaunayTessellation tessellation;
	if(!tessellation.generateTessellation(cell(), positions()->constDataPoint3(), positions()->size(), ghostLayerSize,
			selection() ? selection()->constDataInt() : nullptr, this))
		return false;

	nextProgressSubStep();

	// Determines the grain a Delaunay cell belongs to.
	auto tetrahedronRegion = [this, &tessellation](DelaunayTessellation::CellHandle cell) {
		std::array<int,4> clusters;
		for(int v = 0; v < 4; v++)
			clusters[v] = atomClusters()->getInt(tessellation.vertexIndex(tessellation.cellVertex(cell, v)));
		std::sort(std::begin(clusters), std::end(clusters));
		return (*most_common(std::begin(clusters), std::end(clusters)) + 1);
	};

	// Assign triangle faces to grains.
	auto prepareMeshFace = [this, &tessellation](PartitionMeshData::Face* face, const std::array<int,3>& vertexIndices, const std::array<DelaunayTessellation::VertexHandle,3>& vertexHandles, DelaunayTessellation::CellHandle cell) {
		face->region = tessellation.getUserField(cell) - 1;
	};

	// Cross-links adjacent manifolds.
	auto linkManifolds = [](PartitionMeshData::Edge* edge1, PartitionMeshData::Edge* edge2) {
		OVITO_ASSERT(edge1->nextManifoldEdge == nullptr || edge1->nextManifoldEdge == edge2);
		OVITO_ASSERT(edge2->nextManifoldEdge == nullptr || edge2->nextManifoldEdge == edge1);
		OVITO_ASSERT(edge2->vertex2() == edge1->vertex1());
		OVITO_ASSERT(edge2->vertex1() == edge1->vertex2());
		OVITO_ASSERT(edge1->face()->oppositeFace == nullptr || edge1->face()->oppositeFace == edge2->face());
		OVITO_ASSERT(edge2->face()->oppositeFace == nullptr || edge2->face()->oppositeFace == edge1->face());
		edge1->nextManifoldEdge = edge2;
		edge2->nextManifoldEdge = edge1;
		edge1->face()->oppositeFace = edge2->face();
		edge2->face()->oppositeFace = edge1->face();
	};

	ManifoldConstructionHelper<PartitionMeshData, true, true> manifoldConstructor(tessellation, *_mesh, alpha, positions());
	if(!manifoldConstructor.construct(tetrahedronRegion, this, prepareMeshFace, linkManifolds))
		return false;
	_spaceFillingGrain = manifoldConstructor.spaceFillingRegion();

	nextProgressSubStep();

	std::vector<PartitionMeshData::Edge*> visitedEdges;
	std::vector<PartitionMeshData::Vertex*> visitedVertices;
	size_t oldVertexCount = _mesh->vertices().size();
	for(size_t vertexIndex = 0; vertexIndex < oldVertexCount; vertexIndex++) {
		if(isCanceled())
			return false;

		PartitionMeshData::Vertex* vertex = _mesh->vertices()[vertexIndex];
		visitedEdges.clear();
		// Visit all manifolds that this vertex is part of.
		for(PartitionMeshData::Edge* startEdge = vertex->edges(); startEdge != nullptr; startEdge = startEdge->nextVertexEdge()) {
			if(std::find(visitedEdges.cbegin(), visitedEdges.cend(), startEdge) != visitedEdges.cend()) continue;
			// Traverse the manifold around the current vertex edge by edge.
			// Detect if there are two edges connecting to the same neighbor vertex.
			visitedVertices.clear();
			PartitionMeshData::Edge* endEdge = startEdge;
			PartitionMeshData::Edge* currentEdge = startEdge;
			do {
				OVITO_ASSERT(currentEdge->vertex1() == vertex);
				OVITO_ASSERT(std::find(visitedEdges.cbegin(), visitedEdges.cend(), currentEdge) == visitedEdges.cend());

				if(std::find(visitedVertices.cbegin(), visitedVertices.cend(), currentEdge->vertex2()) != visitedVertices.cend()) {
					// Encountered the same neighbor vertex twice.
					// That means the manifold is self-intersecting and we should split the central vertex

					// Retrieve the other edge where the manifold intersects itself.
					auto iter = std::find_if(visitedEdges.rbegin(), visitedEdges.rend(), [currentEdge](PartitionMeshData::Edge* e) {
						return e->vertex2() == currentEdge->vertex2();
					});
					OVITO_ASSERT(iter != visitedEdges.rend());
					PartitionMeshData::Edge* otherEdge = *iter;

					// Rewire edges to produce two separate manifolds.
					PartitionMeshData::Edge* oppositeEdge1 = otherEdge->unlinkFromOppositeEdge();
					PartitionMeshData::Edge* oppositeEdge2 = currentEdge->unlinkFromOppositeEdge();
					currentEdge->linkToOppositeEdge(oppositeEdge1);
					otherEdge->linkToOppositeEdge(oppositeEdge2);

					// Split the vertex.
					PartitionMeshData::Vertex* newVertex = _mesh->createVertex(vertex->pos());

					// Transfer one group of manifolds to the new vertex.
					std::vector<PartitionMeshData::Edge*> transferredEdges;
					std::deque<PartitionMeshData::Edge*> edgesToBeVisited;
					edgesToBeVisited.push_back(otherEdge);
					do {
						PartitionMeshData::Edge* edge = edgesToBeVisited.front();
						edgesToBeVisited.pop_front();
						PartitionMeshData::Edge* iterEdge = edge;
						do {
							PartitionMeshData::Edge* iterEdge2 = iterEdge;
							do {
								if(std::find(transferredEdges.cbegin(), transferredEdges.cend(), iterEdge2) == transferredEdges.cend()) {
									vertex->transferEdgeToVertex(iterEdge2, newVertex);
									transferredEdges.push_back(iterEdge2);
									edgesToBeVisited.push_back(iterEdge2);
								}
								iterEdge2 = iterEdge2->oppositeEdge()->nextManifoldEdge;
								OVITO_ASSERT(iterEdge2 != nullptr);
							}
							while(iterEdge2 != iterEdge);
							iterEdge = iterEdge->prevFaceEdge()->oppositeEdge();
						}
						while(iterEdge != edge);
					}
					while(!edgesToBeVisited.empty());

					if(otherEdge == endEdge) {
						endEdge = currentEdge;
					}
				}
				visitedVertices.push_back(currentEdge->vertex2());
				visitedEdges.push_back(currentEdge);

				currentEdge = currentEdge->prevFaceEdge()->oppositeEdge();
			}
			while(currentEdge != endEdge);
		}
	}

	// Smooth the generated triangle mesh.
	PartitionMesh::smoothMesh(*_mesh, cell(), _meshSmoothingLevel, this);

	// Make sure every mesh vertex is only part of one surface manifold.
	_mesh->duplicateSharedVertices();

	endProgressSubSteps();

	return true;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
