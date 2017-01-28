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

#include <plugins/particles/Particles.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include <plugins/particles/util/NearestNeighborFinder.h>
#include "StructuralClusteringModifier.h"

#include <copr/index_copr.hpp>
#include <copr/coordination.hpp>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(StructuralClusteringModifier, AsynchronousParticleModifier);
DEFINE_FLAGS_PROPERTY_FIELD(StructuralClusteringModifier, faceThreshold, "FaceThreshold", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(StructuralClusteringModifier, rmsdThreshold, "RMSDThreshold", PROPERTY_FIELD_MEMORIZE);
SET_PROPERTY_FIELD_LABEL(StructuralClusteringModifier, faceThreshold, "Voronoi face threshold");
SET_PROPERTY_FIELD_LABEL(StructuralClusteringModifier, rmsdThreshold, "RMSD threshold");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(StructuralClusteringModifier, faceThreshold, FloatParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(StructuralClusteringModifier, rmsdThreshold, FloatParameterUnit, 0);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
StructuralClusteringModifier::StructuralClusteringModifier(DataSet* dataset) : AsynchronousParticleModifier(dataset),
	_numClusters(0), _faceThreshold(0.02), _rmsdThreshold(0.1)
{
	INIT_PROPERTY_FIELD(faceThreshold);
	INIT_PROPERTY_FIELD(rmsdThreshold);
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void StructuralClusteringModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	AsynchronousParticleModifier::propertyChanged(field);

	// Recompute modifier results when the parameters have been changed.
	if(	field == PROPERTY_FIELD(faceThreshold) ||
		field == PROPERTY_FIELD(rmsdThreshold))
		invalidateCachedResults();
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
std::shared_ptr<AsynchronousParticleModifier::ComputeEngine> StructuralClusteringModifier::createEngine(TimePoint time, TimeInterval validityInterval)
{
	// Get the particle positions.
	ParticlePropertyObject* posProperty = expectStandardProperty(ParticleProperty::PositionProperty);

	// Get simulation cell.
	SimulationCellObject* inputCell = expectSimulationCell();

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<StructuralClusteringEngine>(validityInterval, posProperty->storage(), inputCell->data(), faceThreshold(), rmsdThreshold());
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void StructuralClusteringModifier::StructuralClusteringEngine::perform()
{
	setProgressText(tr("Performing structural clustering"));

	// Allocate memory for neighbor lists.
	std::vector<Vector_3<double>> neighborVectors(positions()->size() * COPR_MAX_POINTS);
	std::vector<int> neighborIndices(positions()->size() * COPR_MAX_POINTS);

	// Prepare the neighbor list builder.
	NearestNeighborFinder neighFinder(COPR_MAX_POINTS);
	if(!neighFinder.prepare(positions(), cell(), nullptr, this))
		return;

	// Build neighbor lists.
	setProgressRange(positions()->size());
	setProgressValue(0);
	if(!parallelForChunks(positions()->size(), *this, [this, &neighFinder, &neighborVectors, &neighborIndices](size_t startIndex, size_t count, FutureInterfaceBase& progress) {

		// Initialize thread-local storage
		void* local_handle = copr_voronoi_initialize_local();

		// Construct local neighbor list builder.
		NearestNeighborFinder::Query<COPR_MAX_POINTS> neighQuery(neighFinder);

		size_t endIndex = startIndex + count;
		for(size_t index = startIndex; index < endIndex; index++) {

			// Update progress indicator.
			if((index % 256) == 0)
				progress.incrementProgressValue(256);

			// Break out of loop when operation was canceled.
			if(progress.isCanceled())
				break;

			// Find N nearest neighbors of current atom.
			neighQuery.findNeighbors(neighFinder.particlePos(index));

			// Bring neighbor coordinates into a form suitable for the COPR library.
			double points[COPR_MAX_POINTS+1][3];
			points[0][0] = points[0][1] = points[0][2] = 0;
			for(int i = 0; i < neighQuery.results().size(); i++) {
				points[i+1][0] = neighQuery.results()[i].delta.x();
				points[i+1][1] = neighQuery.results()[i].delta.y();
				points[i+1][2] = neighQuery.results()[i].delta.z();
			}

			bool is_neighbour[COPR_MAX_POINTS+1] = { false };
			int ret = calculate_coordination(local_handle, neighQuery.results().size() + 1, points, _faceThreshold, is_neighbour);

			// Store the atom's neighbor list for later use.
			FloatType scale = 0;
			int numNeighbors = 0;
			for(int i = 0; i < neighQuery.results().size(); i++) {
				//if(index == 18-1)
				//	qDebug() << i << is_neighbour[i+1] << neighQuery.results()[i].distanceSq;
				if(is_neighbour[i+1]) {
					neighborVectors[index * COPR_MAX_POINTS + numNeighbors] = (Vector_3<double>)neighQuery.results()[i].delta;
					neighborIndices[index * COPR_MAX_POINTS + numNeighbors] = neighQuery.results()[i].index;
					scale += sqrt(neighQuery.results()[i].distanceSq);
					numNeighbors++;
				}
			}

			// Normalize neighbor vectors.
			if(numNeighbors >= 3) {
				scale /= numNeighbors;
				for(int i = 0; i < numNeighbors; i++)
					neighborVectors[index * COPR_MAX_POINTS + i] /= scale;
			}

			coordinationNumbers()->setInt(index, numNeighbors);
		}

		// Release thread-local storage
		copr_voronoi_uninitialize_local(local_handle);

	})) return;

	// Perform clustering.
	setProgressRange(positions()->size());
	setProgressValue(0);
	_numClusters = 0;
	std::fill(_particleClusters->dataInt(), _particleClusters->dataInt() + _particleClusters->size(), -1);
	std::deque<int> toProcess;
	for(size_t seedParticleIndex = 0; seedParticleIndex < positions()->size(); seedParticleIndex++) {

		// Skip particles that have already been assigned to a cluster.
		if(_particleClusters->getInt(seedParticleIndex) != -1)
			continue;

		// Start a new cluster.
		int cluster = ++_numClusters;
		_particleClusters->setInt(seedParticleIndex, cluster);

		// Now recursively visit all neighbors of the seed particle and add them to the cluster too.
		OVITO_ASSERT(toProcess.empty());
		toProcess.push_back(seedParticleIndex);

		do {
			incrementProgressValue();
			if(isCanceled())
				return;

			size_t currentParticle = toProcess.front();
			toProcess.pop_front();

			for(int n = 0; n < coordinationNumbers()->getInt(currentParticle); n++) {
				int neighborIndex = neighborIndices[currentParticle * COPR_MAX_POINTS + n];
				if(_particleClusters->getInt(neighborIndex) == -1) {

					// Both atoms must have exactly the same number of neighbors.
					if(coordinationNumbers()->getInt(currentParticle) == coordinationNumbers()->getInt(neighborIndex)) {

						int num_nodes_explored;
						double rmsd;
						uint8_t best_permutation[COPR_MAX_POINTS];

						bool match_found = copr_register_points_dfs(coordinationNumbers()->getInt(currentParticle),
								reinterpret_cast<double (*)[3]>(&neighborVectors[currentParticle * COPR_MAX_POINTS]),
								reinterpret_cast<double (*)[3]>(&neighborVectors[neighborIndex * COPR_MAX_POINTS]),
								_rmsdThreshold, false, best_permutation, &num_nodes_explored, &rmsd, nullptr, nullptr);

						if(match_found) {

							// Make neighbor atom part of the cluster and continue recursive traversal.
							_particleClusters->setInt(neighborIndex, cluster);
							toProcess.push_back(neighborIndex);
						}
					}
				}
			}
		}
		while(toProcess.empty() == false);
	}

}

/******************************************************************************
* Unpacks the results of the computation engine and stores them in the modifier.
******************************************************************************/
void StructuralClusteringModifier::transferComputationResults(ComputeEngine* engine)
{
	StructuralClusteringEngine* eng = static_cast<StructuralClusteringEngine*>(engine);
	_particleClusters = eng->particleClusters();
	_numClusters = eng->numClusters();
	_coordinationNumbers = eng->coordinationNumbers();
}

/******************************************************************************
* Lets the modifier insert the cached computation results into the
* modification pipeline.
******************************************************************************/
PipelineStatus StructuralClusteringModifier::applyComputationResults(TimePoint time, TimeInterval& validityInterval)
{
	if(!_particleClusters)
		throwException(tr("No computation results available."));

	if(inputParticleCount() != _particleClusters->size())
		throwException(tr("The number of input particles has changed. The stored results have become invalid."));

	outputStandardProperty(_particleClusters.data());
	outputStandardProperty(_coordinationNumbers.data());

	output().attributes().insert(QStringLiteral("StructuralClustering.cluster_count"), QVariant::fromValue(_numClusters));

	return PipelineStatus(PipelineStatus::Success, tr("Found %1 clusters").arg(_numClusters));
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
