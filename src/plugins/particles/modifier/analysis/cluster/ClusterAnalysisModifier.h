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

#pragma once


#include <plugins/particles/Particles.h>
#include <plugins/particles/data/ParticleProperty.h>
#include <plugins/particles/util/CutoffNeighborFinder.h>
#include "../../AsynchronousParticleModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * \brief This modifier determines decomposes the particle set into clusters.
 */
class OVITO_PARTICLES_EXPORT ClusterAnalysisModifier : public AsynchronousParticleModifier
{
public:

	/// Constructor.
	Q_INVOKABLE ClusterAnalysisModifier(DataSet* dataset);

	/// Returns the number of clusters found during the last successful evaluation of the modifier.
	size_t clusterCount() const { return _numClusters; }

protected:

	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// Creates a computation engine that will compute the modifier's results.
	virtual std::shared_ptr<ComputeEngine> createEngine(TimePoint time, TimeInterval validityInterval) override;

	/// Unpacks the results of the computation engine and stores them in the modifier.
	virtual void transferComputationResults(ComputeEngine* engine) override;

	/// Lets the modifier insert the cached computation results into the modification pipeline.
	virtual PipelineStatus applyComputationResults(TimePoint time, TimeInterval& validityInterval) override;

private:

	/// Computes the modifier's results.
	class ClusterAnalysisEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		ClusterAnalysisEngine(const TimeInterval& validityInterval, ParticleProperty* positions, const SimulationCell& simCell, FloatType cutoff, bool sortBySize, ParticleProperty* selection) :
			ComputeEngine(validityInterval),
			_positions(positions), _simCell(simCell), _cutoff(cutoff),
			_sortBySize(sortBySize),
			_selection(selection),
			_largestClusterSize(0),
			_particleClusters(new ParticleProperty(positions->size(), ParticleProperty::ClusterProperty, 0, false)) {}

		/// Computes the modifier's results and stores them in this object for later retrieval.
		virtual void perform() override;

		/// Returns the property storage that contains the input particle positions.
		ParticleProperty* positions() const { return _positions.data(); }

		/// Returns the simulation cell data.
		const SimulationCell& cell() const { return _simCell; }

		/// Returns the property storage that contains the computed cluster number of each particle.
		ParticleProperty* particleClusters() const { return _particleClusters.data(); }

		/// Returns the cutoff radius.
		FloatType cutoff() const { return _cutoff; }

		/// Returns the property storage that contains the particle selection (optional).
		ParticleProperty* selection() const { return _selection.data(); }

		/// Returns the number of clusters.
		size_t numClusters() const { return _numClusters; }

		/// Returns the size of the largest cluster.
		size_t largestClusterSize() const { return _largestClusterSize; }

	private:

		FloatType _cutoff;
		SimulationCell _simCell;
		bool _sortBySize;
		size_t _numClusters;
		size_t _largestClusterSize;
		QExplicitlySharedDataPointer<ParticleProperty> _positions;
		QExplicitlySharedDataPointer<ParticleProperty> _selection;
		QExplicitlySharedDataPointer<ParticleProperty> _particleClusters;
	};

	/// This stores the cached results of the modifier.
	QExplicitlySharedDataPointer<ParticleProperty> _particleClusters;

	/// Controls the cutoff radius for the neighbor lists.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, cutoff, setCutoff);

	/// Controls whether analysis should take into account only selected particles.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, onlySelectedParticles, setOnlySelectedParticles);

	/// Controls the sorting of cluster IDs by size.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, sortBySize, setSortBySize);

	/// The number of clusters identified during the last evaluation of the modifier.
	size_t _numClusters;

	/// The size of the largest cluster.
	size_t _largestClusterSize;

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Cluster analysis");
	Q_CLASSINFO("ModifierCategory", "Analysis");
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


