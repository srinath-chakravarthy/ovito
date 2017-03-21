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

#pragma once


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
		StructuralClusteringEngine(const TimeInterval& validityInterval, ParticleProperty* positions, const SimulationCell& simCell, FloatType faceThreshold, FloatType rmsdThreshold) :
			ComputeEngine(validityInterval),
			_positions(positions), _simCell(simCell),
			_faceThreshold(faceThreshold),
			_rmsdThreshold(rmsdThreshold),
			_particleClusters(new ParticleProperty(positions->size(), ParticleProperty::ClusterProperty, 0, false)),
			_coordinationNumbers(new ParticleProperty(positions->size(), ParticleProperty::CoordinationProperty, 0, false)) {}

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

		/// Returns the property storage that contains the computed coordination numbers.
		ParticleProperty* coordinationNumbers() const { return _coordinationNumbers.data(); }

	private:

		FloatType _faceThreshold;
		FloatType _rmsdThreshold;
		SimulationCell _simCell;
		size_t _numClusters;
		QExplicitlySharedDataPointer<ParticleProperty> _positions;
		QExplicitlySharedDataPointer<ParticleProperty> _particleClusters;
		QExplicitlySharedDataPointer<ParticleProperty> _coordinationNumbers;
	};

	/// Controls the threshold for Voronoi faces
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, faceThreshold, setFaceThreshold);

	/// Controls how similar two structures need to be.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, rmsdThreshold, setRmsdThreshold);

	/// This stores the cached results of the modifier.
	QExplicitlySharedDataPointer<ParticleProperty> _particleClusters;

	/// The number of clusters identified during the last evaluation of the modifier.
	size_t _numClusters;

	/// This stores the cached results of the modifier.
	QExplicitlySharedDataPointer<ParticleProperty> _coordinationNumbers;

private:

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Structural clustering");
	Q_CLASSINFO("ModifierCategory", "Analysis");
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


