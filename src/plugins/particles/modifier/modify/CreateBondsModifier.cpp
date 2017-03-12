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
#include <plugins/particles/util/CutoffNeighborFinder.h>
#include <plugins/particles/objects/ParticleTypeProperty.h>
#include <plugins/particles/objects/BondsDisplay.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include "CreateBondsModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(CreateBondsModifier, AsynchronousParticleModifier);
DEFINE_PROPERTY_FIELD(CreateBondsModifier, cutoffMode, "CutoffMode");
DEFINE_FLAGS_PROPERTY_FIELD(CreateBondsModifier, uniformCutoff, "UniformCutoff", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(CreateBondsModifier, minimumCutoff, "MinimumCutoff");
DEFINE_FLAGS_PROPERTY_FIELD(CreateBondsModifier, onlyIntraMoleculeBonds, "OnlyIntraMoleculeBonds", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_REFERENCE_FIELD(CreateBondsModifier, bondsDisplay, "BondsDisplay", BondsDisplay, PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);
SET_PROPERTY_FIELD_LABEL(CreateBondsModifier, cutoffMode, "Cutoff mode");
SET_PROPERTY_FIELD_LABEL(CreateBondsModifier, uniformCutoff, "Cutoff radius");
SET_PROPERTY_FIELD_LABEL(CreateBondsModifier, minimumCutoff, "Lower cutoff");
SET_PROPERTY_FIELD_LABEL(CreateBondsModifier, onlyIntraMoleculeBonds, "No bonds between different molecules");
SET_PROPERTY_FIELD_LABEL(CreateBondsModifier, bondsDisplay, "Bonds display");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(CreateBondsModifier, uniformCutoff, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(CreateBondsModifier, minimumCutoff, WorldParameterUnit, 0);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
CreateBondsModifier::CreateBondsModifier(DataSet* dataset) : AsynchronousParticleModifier(dataset),
	_cutoffMode(UniformCutoff), _uniformCutoff(3.2), _onlyIntraMoleculeBonds(false), _minimumCutoff(0)
{
	INIT_PROPERTY_FIELD(cutoffMode);
	INIT_PROPERTY_FIELD(uniformCutoff);
	INIT_PROPERTY_FIELD(onlyIntraMoleculeBonds);
	INIT_PROPERTY_FIELD(bondsDisplay);
	INIT_PROPERTY_FIELD(minimumCutoff);

	// Create the display object for bonds rendering and assign it to the data object.
	setBondsDisplay(new BondsDisplay(dataset));
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void CreateBondsModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	AsynchronousParticleModifier::propertyChanged(field);

	// Recompute results when the parameters have been changed.
	if(field == PROPERTY_FIELD(uniformCutoff) || field == PROPERTY_FIELD(cutoffMode)
			 || field == PROPERTY_FIELD(onlyIntraMoleculeBonds)
			 || field == PROPERTY_FIELD(minimumCutoff))
		invalidateCachedResults();
}

/******************************************************************************
* Sets the cutoff radii for pairs of particle types.
******************************************************************************/
void CreateBondsModifier::setPairCutoffs(const PairCutoffsList& pairCutoffs)
{
	// Make the property change undoable.
	dataset()->undoStack().undoablePropertyChange<PairCutoffsList>(this,
			&CreateBondsModifier::pairCutoffs, &CreateBondsModifier::setPairCutoffs);

	_pairCutoffs = pairCutoffs;

	invalidateCachedResults();
	notifyDependents(ReferenceEvent::TargetChanged);
}

/******************************************************************************
* Sets the cutoff radius for a pair of particle types.
******************************************************************************/
void CreateBondsModifier::setPairCutoff(const QString& typeA, const QString& typeB, FloatType cutoff)
{
	PairCutoffsList newList = pairCutoffs();
	if(cutoff > 0) {
		newList[qMakePair(typeA, typeB)] = cutoff;
		newList[qMakePair(typeB, typeA)] = cutoff;
	}
	else {
		newList.remove(qMakePair(typeA, typeB));
		newList.remove(qMakePair(typeB, typeA));
	}
	setPairCutoffs(newList);
}

/******************************************************************************
* Returns the pair-wise cutoff radius for a pair of particle types.
******************************************************************************/
FloatType CreateBondsModifier::getPairCutoff(const QString& typeA, const QString& typeB) const
{
	auto iter = pairCutoffs().find(qMakePair(typeA, typeB));
	if(iter != pairCutoffs().end()) return iter.value();
	iter = pairCutoffs().find(qMakePair(typeB, typeA));
	if(iter != pairCutoffs().end()) return iter.value();
	return 0;
}

/******************************************************************************
* Saves the class' contents to the given stream.
******************************************************************************/
void CreateBondsModifier::saveToStream(ObjectSaveStream& stream)
{
	AsynchronousParticleModifier::saveToStream(stream);

	stream.beginChunk(0x01);
	stream << _pairCutoffs;
	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void CreateBondsModifier::loadFromStream(ObjectLoadStream& stream)
{
	AsynchronousParticleModifier::loadFromStream(stream);

	stream.expectChunk(0x01);
	stream >> _pairCutoffs;
	stream.closeChunk();
}

/******************************************************************************
* Creates a copy of this object.
******************************************************************************/
OORef<RefTarget> CreateBondsModifier::clone(bool deepCopy, CloneHelper& cloneHelper)
{
	// Let the base class create an instance of this class.
	OORef<CreateBondsModifier> clone = static_object_cast<CreateBondsModifier>(AsynchronousParticleModifier::clone(deepCopy, cloneHelper));
	clone->_pairCutoffs = this->_pairCutoffs;
	return clone;
}

/******************************************************************************
* Handles reference events sent by reference targets of this object.
******************************************************************************/
bool CreateBondsModifier::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	// Do not propagate messages from the attached display object.
	if(source == bondsDisplay())
		return false;

	return AsynchronousParticleModifier::referenceEvent(source, event);
}

/******************************************************************************
* Resets the modifier's result cache.
******************************************************************************/
void CreateBondsModifier::invalidateCachedResults()
{
	AsynchronousParticleModifier::invalidateCachedResults();

	// Reset all bonds when the input has changed.
	_bonds.reset();
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a pipeline.
******************************************************************************/
void CreateBondsModifier::initializeModifier(PipelineObject* pipeline, ModifierApplication* modApp)
{
	AsynchronousParticleModifier::initializeModifier(pipeline, modApp);

	// Adopt the upstream BondsDisplay object if there already is one.
	PipelineFlowState input = getModifierInput(modApp);
	if(BondsObject* bondsObj = input.findObject<BondsObject>()) {
		for(DisplayObject* displayObj : bondsObj->displayObjects()) {
			if(BondsDisplay* bondsDisplay = dynamic_object_cast<BondsDisplay>(displayObj)) {
				_bondsDisplay = bondsDisplay;
				break;
			}
		}
	}
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
std::shared_ptr<AsynchronousParticleModifier::ComputeEngine> CreateBondsModifier::createEngine(TimePoint time, TimeInterval validityInterval)
{
	// Get modifier input.
	ParticlePropertyObject* posProperty = expectStandardProperty(ParticleProperty::PositionProperty);
	SimulationCellObject* simCell = expectSimulationCell();

	// The neighbor list cutoff.
	FloatType maxCutoff = uniformCutoff();

	// Build table of pair-wise cutoff radii.
	ParticleTypeProperty* typeProperty = nullptr;
	std::vector<std::vector<FloatType>> pairCutoffSquaredTable;
	if(cutoffMode() == PairCutoff) {
		typeProperty = dynamic_object_cast<ParticleTypeProperty>(expectStandardProperty(ParticleProperty::ParticleTypeProperty));
		if(typeProperty) {
			maxCutoff = 0;
			for(PairCutoffsList::const_iterator entry = pairCutoffs().begin(); entry != pairCutoffs().end(); ++entry) {
				FloatType cutoff = entry.value();
				if(cutoff > 0) {
					ParticleType* ptype1 = typeProperty->particleType(entry.key().first);
					ParticleType* ptype2 = typeProperty->particleType(entry.key().second);
					if(ptype1 && ptype2 && ptype1->id() >= 0 && ptype2->id() >= 0) {
						if((int)pairCutoffSquaredTable.size() <= std::max(ptype1->id(), ptype2->id())) pairCutoffSquaredTable.resize(std::max(ptype1->id(), ptype2->id()) + 1);
						if((int)pairCutoffSquaredTable[ptype1->id()].size() <= ptype2->id()) pairCutoffSquaredTable[ptype1->id()].resize(ptype2->id() + 1, FloatType(0));
						if((int)pairCutoffSquaredTable[ptype2->id()].size() <= ptype1->id()) pairCutoffSquaredTable[ptype2->id()].resize(ptype1->id() + 1, FloatType(0));
						pairCutoffSquaredTable[ptype1->id()][ptype2->id()] = cutoff * cutoff;
						pairCutoffSquaredTable[ptype2->id()][ptype1->id()] = cutoff * cutoff;
						if(cutoff > maxCutoff) maxCutoff = cutoff;
					}
				}
			}
			if(maxCutoff <= 0)
				throwException(tr("At least one positive bond cutoff must be set for a valid pair of particle types."));
		}
	}

	// Get molecule IDs.
	ParticlePropertyObject* moleculeProperty = onlyIntraMoleculeBonds() ? inputStandardProperty(ParticleProperty::MoleculeProperty) : nullptr;

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<BondsEngine>(validityInterval, posProperty->storage(),
			typeProperty ? typeProperty->storage() : nullptr, simCell->data(), cutoffMode(),
			maxCutoff, minimumCutoff(), std::move(pairCutoffSquaredTable), moleculeProperty ? moleculeProperty->storage() : nullptr);
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void CreateBondsModifier::BondsEngine::perform()
{
	setProgressText(tr("Generating bonds"));

	// Prepare the neighbor list.
	CutoffNeighborFinder neighborFinder;
	if(!neighborFinder.prepare(_maxCutoff, _positions.data(), _simCell, nullptr, *this))
		return;

	FloatType minCutoffSquared = _minCutoff * _minCutoff;

	// Generate (half) bonds.
	size_t particleCount = _positions->size();
	setProgressMaximum(particleCount);
	if(!_particleTypes) {
		for(size_t particleIndex = 0; particleIndex < particleCount; particleIndex++) {
			for(CutoffNeighborFinder::Query neighborQuery(neighborFinder, particleIndex); !neighborQuery.atEnd(); neighborQuery.next()) {
				if(neighborQuery.distanceSquared() < minCutoffSquared)
					continue;
				if(_moleculeIDs && _moleculeIDs->getInt(particleIndex) != _moleculeIDs->getInt(neighborQuery.current()))
					continue;
				_bonds->push_back({ neighborQuery.unwrappedPbcShift(), (unsigned int)particleIndex, (unsigned int)neighborQuery.current() });
			}
			// Update progress indicator.
			if(!setProgressValueIntermittent(particleIndex))
				return;
		}
	}
	else {
		for(size_t particleIndex = 0; particleIndex < particleCount; particleIndex++) {
			for(CutoffNeighborFinder::Query neighborQuery(neighborFinder, particleIndex); !neighborQuery.atEnd(); neighborQuery.next()) {
				if(neighborQuery.distanceSquared() < minCutoffSquared)
					continue;
				if(_moleculeIDs && _moleculeIDs->getInt(particleIndex) != _moleculeIDs->getInt(neighborQuery.current()))
					continue;
				int type1 = _particleTypes->getInt(particleIndex);
				int type2 = _particleTypes->getInt(neighborQuery.current());
				if(type1 >= 0 && type1 < (int)_pairCutoffsSquared.size() && type2 >= 0 && type2 < (int)_pairCutoffsSquared[type1].size()) {
					if(neighborQuery.distanceSquared() <= _pairCutoffsSquared[type1][type2])
						_bonds->push_back({ neighborQuery.unwrappedPbcShift(), (unsigned int)particleIndex, (unsigned int)neighborQuery.current() });
				}
			}
			// Update progress indicator.
			if(!setProgressValueIntermittent(particleIndex))
				return;
		}
	}
	setProgressValue(particleCount);
}

/******************************************************************************
* Unpacks the results of the computation engine and stores them in the modifier.
******************************************************************************/
void CreateBondsModifier::transferComputationResults(ComputeEngine* engine)
{
	_bonds = static_cast<BondsEngine*>(engine)->bonds();
}

/******************************************************************************
* Lets the modifier insert the cached computation results into the
* modification pipeline.
******************************************************************************/
PipelineStatus CreateBondsModifier::applyComputationResults(TimePoint time, TimeInterval& validityInterval)
{
	// Check if the modifier's compute engine has finished executing.
	if(!_bonds)
		throwException(tr("No computation results available."));

	// Add our bonds to the system.
	addBonds(_bonds.data(), _bondsDisplay);

	size_t bondsCount = _bonds->size();
	output().attributes().insert(QStringLiteral("CreateBonds.num_bonds"), QVariant::fromValue(bondsCount / 2));

	// If the number of bonds is unusually high, we better turn off bonds display to prevent the program from freezing.
	if(bondsCount > 1000000) {
		bondsDisplay()->setEnabled(false);
		return PipelineStatus(PipelineStatus::Warning, tr("Created %1 bonds. Automatically disabled display of such a large number of bonds to prevent the program from freezing.").arg(bondsCount));
	}

	return PipelineStatus(PipelineStatus::Success, tr("Created %1 bonds.").arg(bondsCount / 2));
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
