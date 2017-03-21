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
#include <plugins/particles/modifier/AsynchronousParticleModifier.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * \brief Calculates the centro-symmetry parameter (CSP) for particles.
 */
class OVITO_PARTICLES_EXPORT CentroSymmetryModifier : public AsynchronousParticleModifier
{
public:

	/// The maximum number of neighbors that can be taken into account to compute the CSP.
	enum { MAX_CSP_NEIGHBORS = 32 };

public:

	/// Constructor.
	Q_INVOKABLE CentroSymmetryModifier(DataSet* dataset);

protected:

	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// Creates a computation engine that will compute the modifier's results.
	virtual std::shared_ptr<ComputeEngine> createEngine(TimePoint time, TimeInterval validityInterval) override;

	/// Unpacks the results of the computation engine and stores them in the modifier.
	virtual void transferComputationResults(ComputeEngine* engine) override;

	/// Lets the modifier insert the cached computation results into the modification pipeline.
	virtual PipelineStatus applyComputationResults(TimePoint time, TimeInterval& validityInterval) override;

	/// Computes the centrosymmetry parameter of a single particle.
	static FloatType computeCSP(NearestNeighborFinder& neighList, size_t particleIndex);

private:

	/// Computes the modifier's results.
	class CentroSymmetryEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		CentroSymmetryEngine(const TimeInterval& validityInterval, ParticleProperty* positions, const SimulationCell& simCell, int nneighbors) :
			ComputeEngine(validityInterval),
			_nneighbors(nneighbors),
			_positions(positions),
			_simCell(simCell),
			_csp(new ParticleProperty(positions->size(), ParticleProperty::CentroSymmetryProperty, 0, false)) {}

		/// Computes the modifier's results and stores them in this object for later retrieval.
		virtual void perform() override;

		/// Returns the property storage that contains the input particle positions.
		ParticleProperty* positions() const { return _positions.data(); }

		/// Returns the property storage that contains the computed per-particle CSP values.
		ParticleProperty* csp() const { return _csp.data(); }

		/// Returns the simulation cell data.
		const SimulationCell& cell() const { return _simCell; }

	private:

		int _nneighbors;
		SimulationCell _simCell;
		QExplicitlySharedDataPointer<ParticleProperty> _positions;
		QExplicitlySharedDataPointer<ParticleProperty> _csp;
	};

	/// This stores the cached results of the modifier, i.e. the CSP values computed for the particles.
	QExplicitlySharedDataPointer<ParticleProperty> _cspValues;

	/// Specifies the number of nearest neighbors to take into account when computing the CSP.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, numNeighbors, setNumNeighbors);

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Centrosymmetry parameter");
	Q_CLASSINFO("ModifierCategory", "Analysis");
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


