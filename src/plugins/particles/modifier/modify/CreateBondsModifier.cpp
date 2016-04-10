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
#include <core/utilities/concurrent/ParallelFor.h>
#include <plugins/particles/util/CutoffNeighborFinder.h>
#include <plugins/particles/objects/ParticleTypeProperty.h>
#include "CreateBondsModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(Particles, CreateBondsModifier, AsynchronousParticleModifier);
DEFINE_PROPERTY_FIELD(CreateBondsModifier, _cutoffMode, "CutoffMode");
DEFINE_FLAGS_PROPERTY_FIELD(CreateBondsModifier, _uniformCutoff, "UniformCutoff", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(CreateBondsModifier, _onlyIntraMoleculeBonds, "OnlyIntraMoleculeBonds", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_REFERENCE_FIELD(CreateBondsModifier, _bondsDisplay, "BondsDisplay", BondsDisplay, PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);
SET_PROPERTY_FIELD_LABEL(CreateBondsModifier, _cutoffMode, "Cutoff mode");
SET_PROPERTY_FIELD_LABEL(CreateBondsModifier, _uniformCutoff, "Cutoff radius");
SET_PROPERTY_FIELD_LABEL(CreateBondsModifier, _onlyIntraMoleculeBonds, "No bonds between different molecules");
SET_PROPERTY_FIELD_LABEL(CreateBondsModifier, _bondsDisplay, "Bonds display");
SET_PROPERTY_FIELD_UNITS(CreateBondsModifier, _uniformCutoff, WorldParameterUnit);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
CreateBondsModifier::CreateBondsModifier(DataSet* dataset) : AsynchronousParticleModifier(dataset),
	_cutoffMode(UniformCutoff), _uniformCutoff(3.2), _onlyIntraMoleculeBonds(false)
{
	INIT_PROPERTY_FIELD(CreateBondsModifier::_cutoffMode);
	INIT_PROPERTY_FIELD(CreateBondsModifier::_uniformCutoff);
	INIT_PROPERTY_FIELD(CreateBondsModifier::_onlyIntraMoleculeBonds);
	INIT_PROPERTY_FIELD(CreateBondsModifier::_bondsDisplay);

	// Create the display object for bonds rendering and assign it to the data object.
	_bondsDisplay = new BondsDisplay(dataset);
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void CreateBondsModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	AsynchronousParticleModifier::propertyChanged(field);

	// Recompute results when the parameters have been changed.
	if(field == PROPERTY_FIELD(CreateBondsModifier::_uniformCutoff) || field == PROPERTY_FIELD(CreateBondsModifier::_cutoffMode)
			 || field == PROPERTY_FIELD(CreateBondsModifier::_onlyIntraMoleculeBonds))
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
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
std::shared_ptr<AsynchronousParticleModifier::ComputeEngine> CreateBondsModifier::createEngine(TimePoint time, TimeInterval validityInterval)
{
	// Get modifier input.
	ParticlePropertyObject* posProperty = expectStandardProperty(ParticleProperty::PositionProperty);
	SimulationCellObject* simCell = expectSimulationCell();

	// Build table of pair-wise cutoff radii.
	ParticleTypeProperty* typeProperty = nullptr;
	std::vector<std::vector<FloatType>> pairCutoffTable;
	if(cutoffMode() == PairCutoff) {
		typeProperty = dynamic_object_cast<ParticleTypeProperty>(expectStandardProperty(ParticleProperty::ParticleTypeProperty));
		if(typeProperty) {
			for(PairCutoffsList::const_iterator entry = pairCutoffs().begin(); entry != pairCutoffs().end(); ++entry) {
				FloatType cutoff = entry.value();
				if(cutoff > 0.0f) {
					ParticleType* ptype1 = typeProperty->particleType(entry.key().first);
					ParticleType* ptype2 = typeProperty->particleType(entry.key().second);
					if(ptype1 && ptype2 && ptype1->id() >= 0 && ptype2->id() >= 0) {
						if((int)pairCutoffTable.size() <= std::max(ptype1->id(), ptype2->id())) pairCutoffTable.resize(std::max(ptype1->id(), ptype2->id()) + 1);
						if((int)pairCutoffTable[ptype1->id()].size() <= ptype2->id()) pairCutoffTable[ptype1->id()].resize(ptype2->id() + 1, FloatType(0));
						if((int)pairCutoffTable[ptype2->id()].size() <= ptype1->id()) pairCutoffTable[ptype2->id()].resize(ptype1->id() + 1, FloatType(0));
						pairCutoffTable[ptype1->id()][ptype2->id()] = cutoff * cutoff;
						pairCutoffTable[ptype2->id()][ptype1->id()] = cutoff * cutoff;
					}
				}
			}
		}
	}

	// Get molecule IDs.
	ParticlePropertyObject* moleculeProperty = onlyIntraMoleculeBonds() ? inputStandardProperty(ParticleProperty::MoleculeProperty) : nullptr;

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<BondsEngine>(validityInterval, posProperty->storage(),
			typeProperty ? typeProperty->storage() : nullptr, simCell->data(), cutoffMode(),
			uniformCutoff(), std::move(pairCutoffTable), moleculeProperty ? moleculeProperty->storage() : nullptr);
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void CreateBondsModifier::BondsEngine::perform()
{
	setProgressText(tr("Generating bonds"));

	// Determine maximum cutoff.
	FloatType maxCutoff = _uniformCutoff;
	if(_particleTypes) {
		OVITO_ASSERT(_particleTypes->size() == _positions->size());
		for(const auto& innerList : _pairCutoffs)
			for(const auto& cutoff : innerList)
				if(cutoff > maxCutoff) maxCutoff = cutoff;
	}

	// Prepare the neighbor list.
	CutoffNeighborFinder neighborFinder;
	if(!neighborFinder.prepare(maxCutoff, _positions.data(), _simCell, nullptr, this))
		return;

	// Generate (half) bonds.
	size_t particleCount = _positions->size();
	setProgressRange(particleCount);
	if(!_particleTypes) {
		for(size_t particleIndex = 0; particleIndex < particleCount; particleIndex++) {
			for(CutoffNeighborFinder::Query neighborQuery(neighborFinder, particleIndex); !neighborQuery.atEnd(); neighborQuery.next()) {
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
				if(_moleculeIDs && _moleculeIDs->getInt(particleIndex) != _moleculeIDs->getInt(neighborQuery.current()))
					continue;
				int type1 = _particleTypes->getInt(particleIndex);
				int type2 = _particleTypes->getInt(neighborQuery.current());
				if(type1 >= 0 && type1 < (int)_pairCutoffs.size() && type2 >= 0 && type2 < (int)_pairCutoffs[type1].size()) {
					if(neighborQuery.distanceSquared() <= _pairCutoffs[type1][type2])
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
	if(!_bonds)
		throwException(tr("No computation results available."));

	// Create the output data object.
	OORef<BondsObject> bondsObj(new BondsObject(dataset(), _bonds.data()));
	bondsObj->setDisplayObject(_bondsDisplay);

	// Insert output object into the pipeline.
	output().addObject(bondsObj);

	// If the number of bonds is unusually high, we better turn off bonds display to prevent the program from freezing.
	size_t bondsCount = _bonds->size();
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
