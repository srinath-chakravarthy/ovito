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
#include <core/viewport/Viewport.h>
#include "StructureIdentificationModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(StructureIdentificationModifier, AsynchronousParticleModifier);
DEFINE_VECTOR_REFERENCE_FIELD(StructureIdentificationModifier, structureTypes, "StructureTypes", ParticleType);
DEFINE_PROPERTY_FIELD(StructureIdentificationModifier, onlySelectedParticles, "OnlySelectedParticles");
SET_PROPERTY_FIELD_LABEL(StructureIdentificationModifier, structureTypes, "Structure types");
SET_PROPERTY_FIELD_LABEL(StructureIdentificationModifier, onlySelectedParticles, "Use only selected particles");

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
StructureIdentificationModifier::StructureIdentificationModifier(DataSet* dataset) : AsynchronousParticleModifier(dataset),
		_onlySelectedParticles(false)
{
	INIT_PROPERTY_FIELD(structureTypes);
	INIT_PROPERTY_FIELD(onlySelectedParticles);
}

/******************************************************************************
* Create an instance of the ParticleType class to represent a structure type.
******************************************************************************/
void StructureIdentificationModifier::createStructureType(int id, ParticleTypeProperty::PredefinedStructureType predefType)
{
	OORef<ParticleType> stype(new ParticleType(dataset()));
	stype->setId(id);
	stype->setName(ParticleTypeProperty::getPredefinedStructureTypeName(predefType));
	stype->setColor(ParticleTypeProperty::getDefaultParticleColor(ParticleProperty::StructureTypeProperty, stype->name(), id));
	addStructureType(stype);
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void StructureIdentificationModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	AsynchronousParticleModifier::propertyChanged(field);

	// Recompute results when the parameters have changed.
	if(field == PROPERTY_FIELD(onlySelectedParticles))
		invalidateCachedResults();
}

/******************************************************************************
* Is called when a RefTarget referenced by this object has generated an event.
******************************************************************************/
bool StructureIdentificationModifier::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	if(event->type() == ReferenceEvent::TargetEnabledOrDisabled) {
		// Recompute results if a structure type has been enabled or disabled.
		invalidateCachedResults();
	}
	return AsynchronousParticleModifier::referenceEvent(source, event);
}

/******************************************************************************
* Saves the class' contents to the given stream.
******************************************************************************/
void StructureIdentificationModifier::saveToStream(ObjectSaveStream& stream)
{
	AsynchronousParticleModifier::saveToStream(stream);
	stream.beginChunk(0x02);
	// For future use.
	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void StructureIdentificationModifier::loadFromStream(ObjectLoadStream& stream)
{
	AsynchronousParticleModifier::loadFromStream(stream);
	stream.expectChunkRange(0, 2);
	// For future use.
	stream.closeChunk();
}

/******************************************************************************
* Returns a bit flag array which indicates what structure types to search for.
******************************************************************************/
QVector<bool> StructureIdentificationModifier::getTypesToIdentify(int numTypes) const
{
	QVector<bool> typesToIdentify(numTypes, true);
	for(ParticleType* type : structureTypes()) {
		if(type->id() >= 0 && type->id() < numTypes)
			typesToIdentify[type->id()] = type->enabled();
	}
	return typesToIdentify;
}

/******************************************************************************
* Unpacks the results of the computation engine and stores them in the modifier.
******************************************************************************/
void StructureIdentificationModifier::transferComputationResults(ComputeEngine* engine)
{
	setStructureData(static_cast<StructureIdentificationEngine*>(engine)->structures());
}

/******************************************************************************
* Lets the modifier insert the cached computation results into the modification pipeline.
******************************************************************************/
PipelineStatus StructureIdentificationModifier::applyComputationResults(TimePoint time, TimeInterval& validityInterval)
{
	if(!structureData())
		throwException(tr("No computation results available."));

	if(inputParticleCount() != structureData()->size())
		throwException(tr("The number of input particles has changed. The stored analysis results have become invalid."));

	// Create output property object.
	ParticleTypeProperty* structureProperty = static_object_cast<ParticleTypeProperty>(outputStandardProperty(structureData()));

	// Insert structure types into output property.
	structureProperty->setParticleTypes(structureTypes());

	// Build structure type to color map.
	std::vector<Color> structureTypeColors(structureTypes().size());
	std::vector<size_t> typeCounters(structureTypes().size());
	for(ParticleType* stype : structureTypes()) {
		OVITO_ASSERT(stype->id() >= 0);
		if(stype->id() >= (int)structureTypeColors.size()) {
			structureTypeColors.resize(stype->id() + 1);
			typeCounters.resize(stype->id() + 1);
		}
		structureTypeColors[stype->id()] = stype->color();
		typeCounters[stype->id()] = 0;
	}

	// Assign colors to particles based on their structure type.
	ParticlePropertyObject* colorProperty = outputStandardProperty(ParticleProperty::ColorProperty);
	const int* s = structureProperty->constDataInt();
	for(Color& c : colorProperty->colorRange()) {
		if(*s >= 0 && *s < structureTypeColors.size()) {
			c = structureTypeColors[*s];
			typeCounters[*s]++;
		}
		else c.setWhite();
		++s;
	}
	colorProperty->changed();

	QList<int> structureCounts;
	for(ParticleType* stype : structureTypes()) {
		OVITO_ASSERT(stype->id() >= 0);
		while(structureCounts.size() <= stype->id())
			structureCounts.push_back(0);
		structureCounts[stype->id()] = typeCounters[stype->id()];
	}
	if(_structureCounts != structureCounts) {
		_structureCounts.swap(structureCounts);
		notifyDependents(ReferenceEvent::ObjectStatusChanged);
	}

	return PipelineStatus::Success;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
