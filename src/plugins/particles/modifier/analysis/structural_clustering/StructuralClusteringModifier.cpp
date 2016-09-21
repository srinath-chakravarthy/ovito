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

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(Particles, StructuralClusteringModifier, AsynchronousParticleModifier);
DEFINE_FLAGS_PROPERTY_FIELD(StructuralClusteringModifier, _numNeighbors, "NumNeighbors", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(StructuralClusteringModifier, _cutoff, "Cutoff", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(StructuralClusteringModifier, _rmsdThreshold, "RMSDThreshold", PROPERTY_FIELD_MEMORIZE);
SET_PROPERTY_FIELD_LABEL(StructuralClusteringModifier, _numNeighbors, "Number of neighbors");
SET_PROPERTY_FIELD_LABEL(StructuralClusteringModifier, _cutoff, "Cutoff distance");
SET_PROPERTY_FIELD_LABEL(StructuralClusteringModifier, _rmsdThreshold, "RMSD threshold");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(StructuralClusteringModifier, _numNeighbors, IntegerParameterUnit, 3, COPR_MAX_POINTS);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(StructuralClusteringModifier, _cutoff, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(StructuralClusteringModifier, _rmsdThreshold, FloatParameterUnit, 0);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
StructuralClusteringModifier::StructuralClusteringModifier(DataSet* dataset) : AsynchronousParticleModifier(dataset),
	_numClusters(0), _numNeighbors(12), _cutoff(3.2), _rmsdThreshold(0.1)
{
	INIT_PROPERTY_FIELD(StructuralClusteringModifier::_numNeighbors);
	INIT_PROPERTY_FIELD(StructuralClusteringModifier::_cutoff);
	INIT_PROPERTY_FIELD(StructuralClusteringModifier::_rmsdThreshold);
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void StructuralClusteringModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	AsynchronousParticleModifier::propertyChanged(field);

	// Recompute modifier results when the parameters have been changed.
	if(field == PROPERTY_FIELD(StructuralClusteringModifier::_numNeighbors) ||
		field == PROPERTY_FIELD(StructuralClusteringModifier::_cutoff) ||
		field == PROPERTY_FIELD(StructuralClusteringModifier::_rmsdThreshold))
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
	return std::make_shared<StructuralClusteringEngine>(validityInterval, posProperty->storage(), inputCell->data(), numNeighbors(), cutoff(), rmsdThreshold());
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void StructuralClusteringModifier::StructuralClusteringEngine::perform()
{
	setProgressText(tr("Performing structural clustering analysis"));

	// Allocate memory for neighbor lists.
	std::vector<Vector_3<double>> neighborVectors(positions()->size() * _maxNeighbors);
	std::vector<int> neighborCounts(positions()->size());
	std::vector<int> neighborIndices(positions()->size() * _maxNeighbors);

	// Prepare the neighbor list builder.
	NearestNeighborFinder neighFinder(_maxNeighbors);
	if(!neighFinder.prepare(positions(), cell(), nullptr, this))
		return;

	// Build neighbor lists.
	if(!parallelFor(positions()->size(), *this, [this, &neighFinder, &neighborVectors, &neighborIndices, &neighborCounts](size_t index) {
		// Construct local neighbor list builder.
		NearestNeighborFinder::Query<COPR_MAX_POINTS> neighQuery(neighFinder);

		// Find N nearest neighbors of current atom.
		neighQuery.findNeighbors(neighFinder.particlePos(index));
		int numNeighbors = 0;

		// Store the atom's neighbor list for later use.
		FloatType scale = 0;
		for(; numNeighbors < neighQuery.results().size(); numNeighbors++) {

			// Only consider neighbors up to the cutoff radius.
			if(neighQuery.results()[numNeighbors].distanceSq > _cutoff*_cutoff)
				break;

			neighborVectors[index * _maxNeighbors + numNeighbors] = (Vector_3<double>)neighQuery.results()[numNeighbors].delta;
			neighborIndices[index * _maxNeighbors + numNeighbors] = neighQuery.results()[numNeighbors].index;
			scale += sqrt(neighQuery.results()[numNeighbors].distanceSq);
		}
		if(numNeighbors < _maxNeighbors)
			neighborIndices[index * _maxNeighbors + numNeighbors] = -1;

		// Normalize neighbor vectors.
		if(numNeighbors >= 3) {
			scale /= numNeighbors;
			for(int i = 0; i < numNeighbors; i++)
				neighborVectors[index * _maxNeighbors + i] /= scale;
		}

		neighborCounts[index] = numNeighbors;

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

			for(int n = 0; n < neighborCounts[currentParticle]; n++) {
				int neighborIndex = neighborIndices[currentParticle * _maxNeighbors + n];
				if(neighborIndex == -1) break;
				if(_particleClusters->getInt(neighborIndex) == -1) {

					// Both atoms must have exactly the same number of neighbors.
					if(neighborCounts[currentParticle] == neighborCounts[neighborIndex]) {

						int num_nodes_explored;
						double rmsd;
						uint8_t best_permutation[COPR_MAX_POINTS];

						bool match_found = copr_register_points_dfs(neighborCounts[currentParticle],
								reinterpret_cast<double (*)[3]>(&neighborVectors[currentParticle * _maxNeighbors]),
								reinterpret_cast<double (*)[3]>(&neighborVectors[neighborIndex * _maxNeighbors]),
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

	output().attributes().insert(QStringLiteral("StructuralClustering.cluster_count"), QVariant::fromValue(_numClusters));

	return PipelineStatus(PipelineStatus::Success, tr("Found %1 clusters").arg(_numClusters));
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
