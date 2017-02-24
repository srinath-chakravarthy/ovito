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

#pragma once


#include <plugins/particles/Particles.h>
#include <plugins/particles/data/BondsStorage.h>
#include "../AsynchronousParticleModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Selection)

/**
 * \brief Extends the current particle selection by adding particles to the selection
 *        that are neighbors of an already selected particle.
 */
class OVITO_PARTICLES_EXPORT ExpandSelectionModifier : public AsynchronousParticleModifier
{
public:

	enum ExpansionMode {
		BondedNeighbors,	///< Expands the selection to particles that are bonded to an already selected particle.
		CutoffRange,		///< Expands the selection to particles that within a cutoff range of an already selected particle.
		NearestNeighbors,	///< Expands the selection to the N nearest particles of already selected particles.
	};
	Q_ENUMS(ExpansionMode);

	/// Compile-time constant for the maximum number of nearest neighbors that can be taken into account.
	enum { MAX_NEAREST_NEIGHBORS = 30 };

public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE ExpandSelectionModifier(DataSet* dataset);

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

	/// Abstract base class for compute engines.
	class ExpandSelectionEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		ExpandSelectionEngine(const TimeInterval& validityInterval, ParticleProperty* positions, const SimulationCell& simCell, ParticleProperty* inputSelection, int numIterations) :
			ComputeEngine(validityInterval),
			_numIterations(numIterations),
			_positions(positions), _simCell(simCell),
			_inputSelection(inputSelection),
			_outputSelection(inputSelection) {}

		ParticleProperty* outputSelection() { return _outputSelection.data(); }
		size_t numSelectedParticlesInput() const { return _numSelectedParticlesInput; }
		size_t numSelectedParticlesOutput() const { return _numSelectedParticlesOutput; }

		/// Computes the modifier's results and stores them in this object for later retrieval.
		virtual void perform() override;

		/// Performs one iteration of the expansion.
		virtual void expandSelection() = 0;

	protected:
		int _numIterations;
		SimulationCell _simCell;
		QExplicitlySharedDataPointer<ParticleProperty> _positions;
		QExplicitlySharedDataPointer<ParticleProperty> _inputSelection;
		QExplicitlySharedDataPointer<ParticleProperty> _outputSelection;
		size_t _numSelectedParticlesInput;
		size_t _numSelectedParticlesOutput;
	};

	/// Computes the expanded selection by using the nearest neighbor criterion.
	class ExpandSelectionNearestEngine : public ExpandSelectionEngine
	{
	public:
		/// Constructor.
		ExpandSelectionNearestEngine(const TimeInterval& validityInterval, ParticleProperty* positions, const SimulationCell& simCell, ParticleProperty* inputSelection, int numIterations, int numNearestNeighbors) :
			ExpandSelectionEngine(validityInterval, positions, simCell, inputSelection, numIterations), _numNearestNeighbors(numNearestNeighbors) {}

		/// Expands the selection by one step.
		virtual void expandSelection() override;

	private:
		int _numNearestNeighbors;
	};

	/// Computes the expanded selection when using a cutoff range criterion.
	class ExpandSelectionCutoffEngine : public ExpandSelectionEngine
	{
	public:
		/// Constructor.
		ExpandSelectionCutoffEngine(const TimeInterval& validityInterval, ParticleProperty* positions, const SimulationCell& simCell, ParticleProperty* inputSelection, int numIterations, FloatType cutoff) :
			ExpandSelectionEngine(validityInterval, positions, simCell, inputSelection, numIterations), _cutoffRange(cutoff) {}

		/// Expands the selection by one step.
		virtual void expandSelection() override;

	private:
		FloatType _cutoffRange;
	};

	/// Computes the expanded selection when using bonds.
	class ExpandSelectionBondedEngine : public ExpandSelectionEngine
	{
	public:
		/// Constructor.
		ExpandSelectionBondedEngine(const TimeInterval& validityInterval, ParticleProperty* positions, const SimulationCell& simCell, ParticleProperty* inputSelection, int numIterations, BondsStorage* bonds) :
			ExpandSelectionEngine(validityInterval, positions, simCell, inputSelection, numIterations), _bonds(bonds) {}

		/// Expands the selection by one step.
		virtual void expandSelection() override;

	private:
		QExplicitlySharedDataPointer<BondsStorage> _bonds;
	};

private:

	/// The expansion mode.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(ExpansionMode, mode, setMode);

	/// The selection cutoff range.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, cutoffRange, setCutoffRange);

	/// The number of nearest neighbors to select.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, numNearestNeighbors, setNumNearestNeighbors);

	/// The number of expansion steps to perform.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, numberOfIterations, setNumberOfIterations);

	/// This stores the cached results of the modifier.
	QExplicitlySharedDataPointer<ParticleProperty> _outputSelection;

	/// Number of selected particles in the modifier's input.
	size_t _numSelectedParticlesInput;

	/// Number of selected particles in the modifier's output.
	size_t _numSelectedParticlesOutput;

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Expand selection");
	Q_CLASSINFO("ModifierCategory", "Selection");
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


