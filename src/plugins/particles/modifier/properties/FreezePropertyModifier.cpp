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
#include <core/animation/AnimationSettings.h>
#include <core/scene/pipeline/PipelineObject.h>
#include <core/scene/pipeline/PipelineEvalRequest.h>
#include <core/utilities/concurrent/TaskManager.h>
#include "FreezePropertyModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Properties)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(FreezePropertyModifier, ParticleModifier);
DEFINE_PROPERTY_FIELD(FreezePropertyModifier, sourceProperty, "SourceProperty");
DEFINE_PROPERTY_FIELD(FreezePropertyModifier, destinationProperty, "DestinationProperty");
DEFINE_FLAGS_VECTOR_REFERENCE_FIELD(FreezePropertyModifier, cachedDisplayObjects, "CachedDisplayObjects", DisplayObject, PROPERTY_FIELD_NEVER_CLONE_TARGET|PROPERTY_FIELD_NO_CHANGE_MESSAGE|PROPERTY_FIELD_NO_UNDO|PROPERTY_FIELD_NO_SUB_ANIM);
SET_PROPERTY_FIELD_LABEL(FreezePropertyModifier, sourceProperty, "Property");
SET_PROPERTY_FIELD_LABEL(FreezePropertyModifier, destinationProperty, "Destination property");

OVITO_BEGIN_INLINE_NAMESPACE(Internal)
	IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(SavedParticleProperty, RefTarget);
	DEFINE_REFERENCE_FIELD(SavedParticleProperty, property, "Property", ParticlePropertyObject);
	DEFINE_REFERENCE_FIELD(SavedParticleProperty, identifiers, "Identifiers", ParticlePropertyObject);
OVITO_END_INLINE_NAMESPACE

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
FreezePropertyModifier::FreezePropertyModifier(DataSet* dataset) : ParticleModifier(dataset)
{
	INIT_PROPERTY_FIELD(sourceProperty);
	INIT_PROPERTY_FIELD(destinationProperty);
	INIT_PROPERTY_FIELD(cachedDisplayObjects);
}

/******************************************************************************
* This modifies the input object.
******************************************************************************/
PipelineStatus FreezePropertyModifier::modifyParticles(TimePoint time, TimeInterval& validityInterval)
{
	if(sourceProperty().isNull())
		return PipelineStatus(PipelineStatus::Warning, tr("No source property selected."));
	if(destinationProperty().isNull())
		return PipelineStatus(PipelineStatus::Error, tr("No output property selected."));

	// Retrieve the property values stored in the ModifierApplication.
	SavedParticleProperty* savedProperty = dynamic_object_cast<SavedParticleProperty>(modifierApplication()->modifierData());
	if(!savedProperty || !savedProperty->property())
		throwException(tr("No stored values available. Please take a new snapshot of the current property values."));

	// Get the particle property that will be overwritten by the stored one.
	ParticlePropertyObject* outputProperty;
	if(destinationProperty().type() != ParticleProperty::UserProperty) {
		outputProperty = outputStandardProperty(destinationProperty().type(), true);
		if(outputProperty->dataType() != savedProperty->property()->dataType()
			|| outputProperty->componentCount() != savedProperty->property()->componentCount())
			throwException(tr("Types of source property and output property are not compatible. Cannot restore saved property values."));
	}
	else {
		outputProperty = outputCustomProperty(destinationProperty().name(), 
			savedProperty->property()->dataType(), savedProperty->property()->componentCount(),
			0, true);
	}
	OVITO_ASSERT(outputProperty->stride() == savedProperty->property()->stride());

	// Check if particle IDs are present and if the order of particles has changed
	// since we took the snapshot of the property values.
	ParticlePropertyObject* idProperty = inputStandardProperty(ParticleProperty::IdentifierProperty);
	if(savedProperty->identifiers() && idProperty && 
			(idProperty->size() != savedProperty->identifiers()->size() || 
			!std::equal(idProperty->constDataInt(), idProperty->constDataInt() + idProperty->size(), savedProperty->identifiers()->constDataInt()))) {

		// Build ID-to-index map.
		std::unordered_map<int,int> idmap;
		int index = 0;
		for(int id : savedProperty->identifiers()->constIntRange()) {
			if(!idmap.insert(std::make_pair(id,index)).second)
				throwException(tr("Detected duplicate particle ID %1 in saved snapshot. Cannot apply saved property values.").arg(id));
			index++;
		}

		// Copy and reorder property data.
		const int* id = idProperty->constDataInt();
		char* dest = static_cast<char*>(outputProperty->data());
		const char* src = static_cast<const char*>(savedProperty->property()->constData());
		size_t stride = outputProperty->stride();
		for(size_t index = 0; index < outputProperty->size(); index++, ++id, dest += stride) {
			auto mapEntry = idmap.find(*id);
			if(mapEntry == idmap.end())
				throwException(tr("Detected new particle ID %1, which didn't exist when the snapshot was created. Cannot restore saved property values.").arg(*id));
			memcpy(dest, src + stride * mapEntry->second, stride);
		}

		outputProperty->changed();
	}
	else {
		// Make sure the number of particles didn't change when no particle IDs are defined.
		if(savedProperty->property()->size() != outputParticleCount())
			throwException(tr("Number of input particles has changed. Cannot restore saved property values. There were %1 particles when the snapshot was created. Now there are %2.").arg(savedProperty->property()->size()).arg(outputParticleCount()));

		if(outputProperty->type() == savedProperty->property()->type()
				&& outputProperty->name() == savedProperty->property()->name()
				&& outputProperty->dataType() == savedProperty->property()->dataType()) {
			// Make shallow data copy if input and output property are the same.
			outputProperty->setStorage(savedProperty->property()->storage());
		}
		else {
			// Make a full data copy otherwise.
			OVITO_ASSERT(outputProperty->dataType() == savedProperty->property()->dataType());
			OVITO_ASSERT(outputProperty->stride() == savedProperty->property()->stride());
			OVITO_ASSERT(outputProperty->size() == savedProperty->property()->size());
			memcpy(outputProperty->data(), savedProperty->property()->constData(), outputProperty->stride() * outputProperty->size());
			outputProperty->changed();
		}
	}

	// Replace display objects of output property with cached ones and cache any new display objects.
	// This is required to avoid losing the output property's display settings
	// each time the modifier is re-evaluated or when serializing the modifier.
	QVector<DisplayObject*> currentDisplayObjs = outputProperty->displayObjects();
	// Replace with cached display objects if they are of the same class type.
	for(int i = 0; i < currentDisplayObjs.size() && i < _cachedDisplayObjects.size(); i++) {
		if(currentDisplayObjs[i]->getOOType() == _cachedDisplayObjects[i]->getOOType()) {
			currentDisplayObjs[i] = _cachedDisplayObjects[i];
		}
	}
	outputProperty->setDisplayObjects(currentDisplayObjs);
	_cachedDisplayObjects = currentDisplayObjs;

	return PipelineStatus::Success;
}

/******************************************************************************
* This method is called by the system when the modifier is being inserted
* into a pipeline.
******************************************************************************/
void FreezePropertyModifier::initializeModifier(PipelineObject* pipeline, ModifierApplication* modApp)
{
	ParticleModifier::initializeModifier(pipeline, modApp);

	PipelineFlowState input;

	// Use the first available particle property from the input state as data source when the modifier is newly created.
	if(sourceProperty().isNull()) {
		input = getModifierInput(modApp);
		for(DataObject* o : input.objects()) {
			if(ParticlePropertyObject* property = dynamic_object_cast<ParticlePropertyObject>(o)) {
				setSourceProperty(ParticlePropertyReference(property));
				setDestinationProperty(sourceProperty());
				break;
			}
		}
	}

	// Take a snapshot of the property values at the time the modifier is created.
	if(!sourceProperty().isNull() && dynamic_object_cast<SavedParticleProperty>(modApp->modifierData()) == nullptr) {
		if(input.isEmpty())
			input = getModifierInput(modApp);
		takePropertySnapshot(modApp, input);
	}
}

/******************************************************************************
* Takes a snapshot of the source property for a specific ModifierApplication
* of this modifier.
******************************************************************************/
void FreezePropertyModifier::takePropertySnapshot(ModifierApplication* modApp, const PipelineFlowState& state)
{
	// Retrieve the source property.
	if(!sourceProperty().isNull()) {
		ParticlePropertyObject* property = sourceProperty().findInState(state);
		if(property) {
			// Take a snapshot of the property values.
			OORef<SavedParticleProperty> savedProperty = new SavedParticleProperty(dataset());
			savedProperty->reset(property, ParticlePropertyObject::findInState(state, ParticleProperty::IdentifierProperty));
			modApp->setModifierData(savedProperty);
			return;
		}
	}
	modApp->setModifierData(nullptr);
}

/******************************************************************************
* Takes a snapshot of the source property for every ModifierApplication of 
* this modifier.
******************************************************************************/
bool FreezePropertyModifier::takePropertySnapshot(TimePoint time, TaskManager& taskManager, bool waitUntilReady)
{
	for(ModifierApplication* modApp : modifierApplications()) {
		if(PipelineObject* pipelineObj = modApp->pipelineObject()) {
			PipelineFlowState state;
			if(waitUntilReady) {
				Future<PipelineFlowState> stateFuture = pipelineObj->evaluateAsync(PipelineEvalRequest(time, false, modApp, false));
				if(!taskManager.waitForTask(stateFuture))
					return false;
				state = stateFuture.result();
			}
			else {
				state = pipelineObj->evaluateImmediately(PipelineEvalRequest(time, false, modApp, false));
			}
			takePropertySnapshot(modApp, state);
		}
	}
	return true;
}

OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/******************************************************************************
* Makes a copy of the given source property and, optionally, of the provided
* particle identifier list, which will allow to restore the saved property
* values even if the order of particles changes.
******************************************************************************/
void SavedParticleProperty::reset(ParticlePropertyObject* property, ParticlePropertyObject* identifiers)
{
	CloneHelper cloneHelper;
	_property = cloneHelper.cloneObject(property, false);
	_identifiers = cloneHelper.cloneObject(identifiers, false);
	if(_property) _property->setSaveWithScene(true);
	if(_identifiers) _identifiers->setSaveWithScene(true);
}

OVITO_END_INLINE_NAMESPACE

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
