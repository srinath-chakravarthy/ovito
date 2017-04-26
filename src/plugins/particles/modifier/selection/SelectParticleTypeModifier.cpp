///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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
#include <plugins/particles/objects/ParticleTypeProperty.h>
#include <core/dataset/UndoStack.h>
#include <core/animation/AnimationSettings.h>
#include <core/scene/pipeline/PipelineObject.h>
#include "SelectParticleTypeModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Selection)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(SelectParticleTypeModifier, ParticleModifier);
DEFINE_PROPERTY_FIELD(SelectParticleTypeModifier, sourceProperty, "SourceProperty");
DEFINE_PROPERTY_FIELD(SelectParticleTypeModifier, selectedParticleTypes, "SelectedParticleTypes");
SET_PROPERTY_FIELD_LABEL(SelectParticleTypeModifier, sourceProperty, "Property");
SET_PROPERTY_FIELD_LABEL(SelectParticleTypeModifier, selectedParticleTypes, "Selected types");

/******************************************************************************
* This modifies the input object.
******************************************************************************/
PipelineStatus SelectParticleTypeModifier::modifyParticles(TimePoint time, TimeInterval& validityInterval)
{
	// Get the input type property.
	ParticleTypeProperty* typeProperty = dynamic_object_cast<ParticleTypeProperty>(sourceProperty().findInState(input()));
	if(!typeProperty)
		throwException(tr("The source property for this modifier is not present in the input."));
	OVITO_ASSERT(typeProperty->componentCount() == 1);
	OVITO_ASSERT(typeProperty->dataType() == qMetaTypeId<int>());

	// Get the deep copy of the selection property.
	ParticlePropertyObject* selProperty = outputStandardProperty(ParticleProperty::SelectionProperty);

	// The number of selected particles.
	size_t nSelected = 0;

	OVITO_ASSERT(selProperty->size() == typeProperty->size());
	const int* t = typeProperty->constDataInt();
	for(int& s : selProperty->intRange()) {
		if(selectedParticleTypes().contains(*t++)) {
			s = 1;
			nSelected++;
		}
		else s = 0;
	}
	selProperty->changed();

	output().attributes().insert(QStringLiteral("SelectParticleType.num_selected"), QVariant::fromValue(nSelected));

	QString statusMessage = tr("%1 out of %2 particles selected (%3%)").arg(nSelected).arg(inputParticleCount()).arg((FloatType)nSelected * 100 / std::max(1,(int)inputParticleCount()), 0, 'f', 1);
	return PipelineStatus(PipelineStatus::Success, statusMessage);
}

/******************************************************************************
* Loads the user-defined default values of this object's parameter fields from the
* application's settings store.
******************************************************************************/
void SelectParticleTypeModifier::loadUserDefaults()
{
	ParticleModifier::loadUserDefaults();

	// Reset selected input particle property set by the constructor so that initializeModifier() will automatically select a good first choice.
	setSourceProperty(ParticlePropertyReference());
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a pipeline.
******************************************************************************/
void SelectParticleTypeModifier::initializeModifier(PipelineObject* pipeline, ModifierApplication* modApp)
{
	ParticleModifier::initializeModifier(pipeline, modApp);

	if(sourceProperty().isNull()) {
		// Select the first particle type property from the input with more than one particle type.
		PipelineFlowState input = getModifierInput(modApp);
		ParticleTypeProperty* bestProperty = nullptr;
		for(DataObject* o : input.objects()) {
			ParticleTypeProperty* ptypeProp = dynamic_object_cast<ParticleTypeProperty>(o);
			if(ptypeProp && ptypeProp->particleTypes().empty() == false && ptypeProp->componentCount() == 1) {
				bestProperty = ptypeProp;
			}
		}
		if(bestProperty)
			setSourceProperty(bestProperty);
	}
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void SelectParticleTypeModifier::loadFromStream(ObjectLoadStream& stream)
{
	ParticleModifier::loadFromStream(stream);

	// This is to maintain backward compatibility with old program versions.
	// Can be removed in the future.
	if(stream.applicationMajorVersion() == 2 && stream.applicationMinorVersion() <= 3) {
		stream.expectChunk(0x01);
		ParticlePropertyReference pref;
		stream >> pref;
		setSourceProperty(pref);
		QSet<int> types;
		stream >> types;
		setSelectedParticleTypes(types);
		stream.closeChunk();
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
