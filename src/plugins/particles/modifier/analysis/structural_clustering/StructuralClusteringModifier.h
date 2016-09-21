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

#ifndef __OVITO_STRUCTURAL_CLUSTERING_MODIFIER_H
#define __OVITO_STRUCTURAL_CLUSTERING_MODIFIER_H

#include <plugins/particles/Particles.h>
#include <plugins/particles/data/ParticleProperty.h>
#include "../../AsynchronousParticleModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * \brief Groups particles into sets based on a local structure comparison.
 */
class OVITO_PARTICLES_EXPORT StructuralClusteringModifier : public AsynchronousParticleModifier
{
public:

	/// Constructor.
	Q_INVOKABLE StructuralClusteringModifier(DataSet* dataset);

	/// Returns the number of neighbors that will be taken into account.
	int numNeighbors() const { return _numNeighbors; }

	/// Sets the number of neighbors to take into account.
	void setNumNeighbors(int n) { _numNeighbors = n; }

	/// Returns the cutoff radius used to build the neighbor lists for the analysis.
	FloatType cutoff() const { return _cutoff; }

	/// Sets the cutoff radius used to build the neighbor lists for the analysis.
	void setCutoff(FloatType newCutoff) { _cutoff = newCutoff; }

	/// Returns how similar two structures need to be.
	FloatType rmsdThreshold() const { return _rmsdThreshold; }

	/// Sets how similar two structures need to be.
	void setRmsdThreshold(FloatType threshold) { _rmsdThreshold = threshold; }

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
	class StructuralClusteringEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		StructuralClusteringEngine(const TimeInterval& validityInterval, ParticleProperty* positions, const SimulationCell& simCell, int numNeighbors, FloatType cutoff, FloatType rmsdThreshold) :
			ComputeEngine(validityInterval),
			_positions(positions), _simCell(simCell),
			_cutoff(cutoff),
			_rmsdThreshold(rmsdThreshold),
			_maxNeighbors(numNeighbors),
			_particleClusters(new ParticleProperty(positions->size(), ParticleProperty::ClusterProperty, 0, false)) {}

		/// Computes the modifier's results and stores them in this object for later retrieval.
		virtual void perform() override;

		/// Returns the property storage that contains the input particle positions.
		ParticleProperty* positions() const { return _positions.data(); }

		/// Returns the simulation cell data.
		const SimulationCell& cell() const { return _simCell; }

		/// Returns the property storage that contains the computed cluster number of each particle.
		ParticleProperty* particleClusters() const { return _particleClusters.data(); }

		/// Returns the number of clusters.
		size_t numClusters() const { return _numClusters; }

	private:

		FloatType _cutoff;
		FloatType _rmsdThreshold;
		int _maxNeighbors;
		SimulationCell _simCell;
		size_t _numClusters;
		QExplicitlySharedDataPointer<ParticleProperty> _positions;
		QExplicitlySharedDataPointer<ParticleProperty> _particleClusters;
	};

	/// The number of neighbors to take into account.
	PropertyField<int> _numNeighbors;

	/// Controls the cutoff radius for the neighbor lists.
	PropertyField<FloatType> _cutoff;

	/// Controls how similar two structures need to be.
	PropertyField<FloatType> _rmsdThreshold;

	/// This stores the cached results of the modifier.
	QExplicitlySharedDataPointer<ParticleProperty> _particleClusters;

	/// The number of clusters identified during the last evaluation of the modifier.
	size_t _numClusters;

private:

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Structural clustering");
	Q_CLASSINFO("ModifierCategory", "Analysis");

	DECLARE_PROPERTY_FIELD(_cutoff);
	DECLARE_PROPERTY_FIELD(_numNeighbors);
	DECLARE_PROPERTY_FIELD(_rmsdThreshold);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace

#endif // __OVITO_STRUCTURAL_CLUSTERING_MODIFIER_H
