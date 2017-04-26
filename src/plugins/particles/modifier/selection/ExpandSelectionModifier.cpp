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

#include <plugins/particles/Particles.h>
#include <plugins/particles/util/CutoffNeighborFinder.h>
#include <plugins/particles/util/NearestNeighborFinder.h>
#include <plugins/particles/objects/BondsObject.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include "ExpandSelectionModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Selection)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(ExpandSelectionModifier, ParticleModifier);
DEFINE_FLAGS_PROPERTY_FIELD(ExpandSelectionModifier, mode, "Mode", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(ExpandSelectionModifier, cutoffRange, "Cutoff", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(ExpandSelectionModifier, numNearestNeighbors, "NumNearestNeighbors", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(ExpandSelectionModifier, numberOfIterations, "NumIterations");
SET_PROPERTY_FIELD_LABEL(ExpandSelectionModifier, mode, "Mode");
SET_PROPERTY_FIELD_LABEL(ExpandSelectionModifier, cutoffRange, "Cutoff distance");
SET_PROPERTY_FIELD_LABEL(ExpandSelectionModifier, numNearestNeighbors, "N");
SET_PROPERTY_FIELD_LABEL(ExpandSelectionModifier, numberOfIterations, "Number of iterations");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ExpandSelectionModifier, cutoffRange, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(ExpandSelectionModifier, numNearestNeighbors, IntegerParameterUnit, 1, ExpandSelectionModifier::MAX_NEAREST_NEIGHBORS);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ExpandSelectionModifier, numberOfIterations, IntegerParameterUnit, 1);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
ExpandSelectionModifier::ExpandSelectionModifier(DataSet* dataset) : AsynchronousParticleModifier(dataset),
	_mode(CutoffRange), _cutoffRange(3.2), _numNearestNeighbors(1), _numberOfIterations(1)
{
	INIT_PROPERTY_FIELD(mode);
	INIT_PROPERTY_FIELD(cutoffRange);
	INIT_PROPERTY_FIELD(numNearestNeighbors);
	INIT_PROPERTY_FIELD(numberOfIterations);
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
std::shared_ptr<AsynchronousParticleModifier::ComputeEngine> ExpandSelectionModifier::createEngine(TimePoint time, TimeInterval validityInterval)
{
	// Get the current positions.
	ParticlePropertyObject* posProperty = expectStandardProperty(ParticleProperty::PositionProperty);

	// Get the current selection.
	ParticlePropertyObject* inputSelection = expectStandardProperty(ParticleProperty::SelectionProperty);

	// Get simulation cell.
	SimulationCellObject* inputCell = expectSimulationCell();

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	if(mode() == CutoffRange) {
		return std::make_shared<ExpandSelectionCutoffEngine>(validityInterval, posProperty->storage(), inputCell->data(), inputSelection->storage(), numberOfIterations(), cutoffRange());
	}
	else if(mode() == NearestNeighbors) {
		return std::make_shared<ExpandSelectionNearestEngine>(validityInterval, posProperty->storage(), inputCell->data(), inputSelection->storage(), numberOfIterations(), numNearestNeighbors());
	}
	else if(mode() == BondedNeighbors) {
		BondsObject* bonds = input().findObject<BondsObject>();
		if(!bonds)
			throwException(tr("Modifier's input does not contain any bonds."));
		return std::make_shared<ExpandSelectionBondedEngine>(validityInterval, posProperty->storage(), inputCell->data(), inputSelection->storage(), numberOfIterations(), bonds->storage());
	}
	else {
		throwException(tr("Invalid selection expansion mode."));
	}
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void ExpandSelectionModifier::ExpandSelectionEngine::perform()
{
	setProgressText(tr("Expanding particle selection"));

	_numSelectedParticlesInput = _inputSelection->size() - std::count(_inputSelection->constDataInt(), _inputSelection->constDataInt() + _inputSelection->size(), 0);

	beginProgressSubSteps(_numIterations);
	for(int i = 0; i < _numIterations; i++) {
		if(i != 0) nextProgressSubStep();
		_inputSelection = _outputSelection;
		_outputSelection.detach();
		expandSelection();
		if(isCanceled()) return;
	}
	endProgressSubSteps();

	_numSelectedParticlesOutput = _outputSelection->size() - std::count(_outputSelection->constDataInt(), _outputSelection->constDataInt() + _inputSelection->size(), 0);
}

/******************************************************************************
* Performs one iteration of the selection expansion.
******************************************************************************/
void ExpandSelectionModifier::ExpandSelectionNearestEngine::expandSelection()
{
	if(_numNearestNeighbors > MAX_NEAREST_NEIGHBORS)
		throw Exception(tr("Invalid parameter. The expand selection modifier can expand the selection only to the %1 nearest neighbors of particles. This limit is set at compile time.").arg(MAX_NEAREST_NEIGHBORS));

	// Prepare the neighbor list.
	NearestNeighborFinder neighFinder(_numNearestNeighbors);
	if(!neighFinder.prepare(_positions.data(), _simCell, nullptr, *this))
		return;

	OVITO_ASSERT(_inputSelection->constDataInt() != _outputSelection->dataInt());
	parallelFor(_positions->size(), *this, [&neighFinder, this](size_t index) {
		if(!_inputSelection->getInt(index)) return;

		NearestNeighborFinder::Query<MAX_NEAREST_NEIGHBORS> neighQuery(neighFinder);
		neighQuery.findNeighbors(index);
		OVITO_ASSERT(neighQuery.results().size() <= _numNearestNeighbors);

		for(auto n = neighQuery.results().begin(); n != neighQuery.results().end(); ++n) {
			_outputSelection->setInt(n->index, 1);
		}
	});
}

/******************************************************************************
* Performs one iteration of the selection expansion.
******************************************************************************/
void ExpandSelectionModifier::ExpandSelectionBondedEngine::expandSelection()
{
	OVITO_ASSERT(_inputSelection->constDataInt() != _outputSelection->dataInt());

	unsigned int particleCount = _inputSelection->size();
	parallelFor(_bonds->size(), *this, [this,particleCount](size_t index) {

		const Bond& bond = (*_bonds)[index];
		if(bond.index1 >= particleCount || bond.index2 >= particleCount)
			return;
		if(!_inputSelection->getInt(bond.index1))
			return;

		_outputSelection->setInt(bond.index2, 1);
	});
}

/******************************************************************************
* Performs one iteration of the selection expansion.
******************************************************************************/
void ExpandSelectionModifier::ExpandSelectionCutoffEngine::expandSelection()
{
	// Prepare the neighbor list.
	CutoffNeighborFinder neighborListBuilder;
	if(!neighborListBuilder.prepare(_cutoffRange, _positions.data(), _simCell, nullptr, *this))
		return;

	OVITO_ASSERT(_inputSelection->constDataInt() != _outputSelection->dataInt());
	parallelFor(_positions->size(), *this, [&neighborListBuilder, this](size_t index) {
		if(!_inputSelection->getInt(index)) return;
		for(CutoffNeighborFinder::Query neighQuery(neighborListBuilder, index); !neighQuery.atEnd(); neighQuery.next()) {
			_outputSelection->setInt(neighQuery.current(), 1);
		}
	});
}

/******************************************************************************
* Unpacks the results of the computation engine and stores them in the modifier.
******************************************************************************/
void ExpandSelectionModifier::transferComputationResults(ComputeEngine* engine)
{
	ExpandSelectionEngine* eng = static_cast<ExpandSelectionEngine*>(engine);
	_outputSelection = eng->outputSelection();
	_numSelectedParticlesInput = eng->numSelectedParticlesInput();
	_numSelectedParticlesOutput = eng->numSelectedParticlesOutput();
}

/******************************************************************************
* Lets the modifier insert the cached computation results into the
* modification pipeline.
******************************************************************************/
PipelineStatus ExpandSelectionModifier::applyComputationResults(TimePoint time, TimeInterval& validityInterval)
{
	if(!_outputSelection)
		throwException(tr("No modifier results available."));

	if(inputParticleCount() != _outputSelection->size())
		throwException(tr("The number of input particles has changed. The stored results have become invalid."));

	outputStandardProperty(_outputSelection.data());

	QString msg = tr("Added %1 particles to selection.\n"
			"Old selection count was: %2\n"
			"New selection count is: %3")
					.arg(_numSelectedParticlesOutput - _numSelectedParticlesInput)
					.arg(_numSelectedParticlesInput)
					.arg(_numSelectedParticlesOutput);

	return PipelineStatus(PipelineStatus::Success, msg);
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void ExpandSelectionModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	AsynchronousParticleModifier::propertyChanged(field);

	// Recompute modifier results when the parameters have been changed.
	if(field == PROPERTY_FIELD(mode)
			|| field == PROPERTY_FIELD(cutoffRange)
			|| field == PROPERTY_FIELD(numNearestNeighbors)
			|| field == PROPERTY_FIELD(numberOfIterations))
		invalidateCachedResults();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
