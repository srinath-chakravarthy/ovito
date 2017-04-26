///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2014) Alexander Stukowski
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
#include "ClusterAnalysisModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(ClusterAnalysisModifier, AsynchronousParticleModifier);
DEFINE_FLAGS_PROPERTY_FIELD(ClusterAnalysisModifier, cutoff, "Cutoff", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(ClusterAnalysisModifier, onlySelectedParticles, "OnlySelectedParticles");
DEFINE_PROPERTY_FIELD(ClusterAnalysisModifier, sortBySize, "SortBySize");
SET_PROPERTY_FIELD_LABEL(ClusterAnalysisModifier, cutoff, "Cutoff distance");
SET_PROPERTY_FIELD_LABEL(ClusterAnalysisModifier, onlySelectedParticles, "Use only selected particles");
SET_PROPERTY_FIELD_LABEL(ClusterAnalysisModifier, sortBySize, "Sort clusters by size");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ClusterAnalysisModifier, cutoff, WorldParameterUnit, 0);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
ClusterAnalysisModifier::ClusterAnalysisModifier(DataSet* dataset) : AsynchronousParticleModifier(dataset),
	_cutoff(3.2), _onlySelectedParticles(false), _sortBySize(false), _numClusters(0), _largestClusterSize(0)
{
	INIT_PROPERTY_FIELD(cutoff);
	INIT_PROPERTY_FIELD(onlySelectedParticles);
	INIT_PROPERTY_FIELD(sortBySize);
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
std::shared_ptr<AsynchronousParticleModifier::ComputeEngine> ClusterAnalysisModifier::createEngine(TimePoint time, TimeInterval validityInterval)
{
	// Get the current particle positions.
	ParticlePropertyObject* posProperty = expectStandardProperty(ParticleProperty::PositionProperty);

	// Get simulation cell.
	SimulationCellObject* inputCell = expectSimulationCell();

	// Get particle selection.
	ParticleProperty* selectionProperty = nullptr;
	if(onlySelectedParticles())
		selectionProperty = expectStandardProperty(ParticleProperty::SelectionProperty)->storage();

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<ClusterAnalysisEngine>(validityInterval, posProperty->storage(), inputCell->data(), cutoff(), sortBySize(), selectionProperty);
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void ClusterAnalysisModifier::ClusterAnalysisEngine::perform()
{
	setProgressText(tr("Performing cluster analysis"));

	// Prepare the neighbor finder.
	CutoffNeighborFinder neighborFinder;
	if(!neighborFinder.prepare(_cutoff, positions(), cell(), selection(), *this))
		return;

	size_t particleCount = positions()->size();
	setProgressValue(0);
	setProgressMaximum(particleCount);

	// Initialize.
	std::fill(_particleClusters->dataInt(), _particleClusters->dataInt() + _particleClusters->size(), -1);
	_numClusters = 0;

	std::deque<int> toProcess;
	for(size_t seedParticleIndex = 0; seedParticleIndex < particleCount; seedParticleIndex++) {

		// Skip unselected particles that are not included in the analysis.
		if(selection() && !selection()->getInt(seedParticleIndex)) {
			_particleClusters->setInt(seedParticleIndex, 0);
			continue;
		}

		// Skip particles that have already been assigned to a cluster.
		if(_particleClusters->getInt(seedParticleIndex) != -1)
			continue;

		// Start a new cluster.
		int cluster = ++_numClusters;
		_particleClusters->setInt(seedParticleIndex, cluster);

		// Now recursively iterate over all neighbors of the seed particle and add them to the cluster too.
		OVITO_ASSERT(toProcess.empty());
		toProcess.push_back(seedParticleIndex);

		do {
			incrementProgressValue();
			if(isCanceled())
				return;

			int currentParticle = toProcess.front();
			toProcess.pop_front();
			for(CutoffNeighborFinder::Query neighQuery(neighborFinder, currentParticle); !neighQuery.atEnd(); neighQuery.next()) {
				int neighborIndex = neighQuery.current();
				if(_particleClusters->getInt(neighborIndex) == -1) {
					_particleClusters->setInt(neighborIndex, cluster);
					toProcess.push_back(neighborIndex);
				}
			}
		}
		while(toProcess.empty() == false);
	}

	// Sort clusters by size.
	if(_sortBySize && _numClusters != 0) {

		// Determine cluster sizes.
		std::vector<size_t> clusterSizes(_numClusters + 1, 0);
		for(int id : _particleClusters->constIntRange()) {
			clusterSizes[id]++;
		}

		// Sort clusters by size.
		std::vector<int> mapping(_numClusters + 1);
		std::iota(mapping.begin(), mapping.end(), 0);
		std::sort(mapping.begin() + 1, mapping.end(), [&clusterSizes](int a, int b) {
			return clusterSizes[a] > clusterSizes[b];
		});
		_largestClusterSize = clusterSizes[mapping[1]];
		clusterSizes.clear();
		clusterSizes.shrink_to_fit();

		// Remap cluster IDs.
		std::vector<int> inverseMapping(_numClusters + 1);
		for(size_t i = 0; i <= _numClusters; i++)
			inverseMapping[mapping[i]] = i;
		for(int& id : _particleClusters->intRange())
			id = inverseMapping[id];
	}
}

/******************************************************************************
* Unpacks the results of the computation engine and stores them in the modifier.
******************************************************************************/
void ClusterAnalysisModifier::transferComputationResults(ComputeEngine* engine)
{
	ClusterAnalysisEngine* eng = static_cast<ClusterAnalysisEngine*>(engine);
	_particleClusters = eng->particleClusters();
	_numClusters = eng->numClusters();
	_largestClusterSize = eng->largestClusterSize();
}

/******************************************************************************
* Lets the modifier insert the cached computation results into the
* modification pipeline.
******************************************************************************/
PipelineStatus ClusterAnalysisModifier::applyComputationResults(TimePoint time, TimeInterval& validityInterval)
{
	if(!_particleClusters)
		throwException(tr("No computation results available."));

	if(inputParticleCount() != _particleClusters->size())
		throwException(tr("The number of input particles has changed. The stored results have become invalid."));

	outputStandardProperty(_particleClusters.data());

	output().attributes().insert(QStringLiteral("ClusterAnalysis.cluster_count"), QVariant::fromValue(_numClusters));
	if(sortBySize())
		output().attributes().insert(QStringLiteral("ClusterAnalysis.largest_size"), QVariant::fromValue(_largestClusterSize));

	return PipelineStatus(PipelineStatus::Success, tr("Found %1 clusters").arg(_numClusters));
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void ClusterAnalysisModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	AsynchronousParticleModifier::propertyChanged(field);

	// Recompute modifier results when the parameters have been changed.
	if(field == PROPERTY_FIELD(cutoff) ||
			field == PROPERTY_FIELD(onlySelectedParticles) ||
			field == PROPERTY_FIELD(sortBySize))
		invalidateCachedResults();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
