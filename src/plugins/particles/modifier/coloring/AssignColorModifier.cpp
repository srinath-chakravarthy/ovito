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
#include <core/animation/controller/Controller.h>
#include "AssignColorModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Coloring)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(AssignColorModifier, ParticleModifier);
DEFINE_FLAGS_REFERENCE_FIELD(AssignColorModifier, colorController, "Color", Controller, PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(AssignColorModifier, keepSelection, "KeepSelection");
SET_PROPERTY_FIELD_LABEL(AssignColorModifier, colorController, "Color");
SET_PROPERTY_FIELD_LABEL(AssignColorModifier, keepSelection, "Keep selection");

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
AssignColorModifier::AssignColorModifier(DataSet* dataset) : ParticleModifier(dataset), _keepSelection(false)
{
	INIT_PROPERTY_FIELD(colorController);
	INIT_PROPERTY_FIELD(keepSelection);

	setColorController(ControllerManager::createColorController(dataset));
	colorController()->setColorValue(0, Color(0.3f, 0.3f, 1.0f));
}

/******************************************************************************
* Asks the modifier for its validity interval at the given time.
******************************************************************************/
TimeInterval AssignColorModifier::modifierValidity(TimePoint time)
{
	TimeInterval interval = Modifier::modifierValidity(time);
	if(colorController()) interval.intersect(colorController()->validityInterval(time));
	return interval;
}

/******************************************************************************
* This modifies the input object.
******************************************************************************/
PipelineStatus AssignColorModifier::modifyParticles(TimePoint time, TimeInterval& validityInterval)
{
	// Get the selection property.
	ParticlePropertyObject* selProperty = inputStandardProperty(ParticleProperty::SelectionProperty);

	// Get the output color property.
	ParticlePropertyObject* colorProperty = outputStandardProperty(ParticleProperty::ColorProperty, selProperty != nullptr);

	// Get the color to be assigned.
	Color color(1,1,1);
	if(colorController())
		colorController()->getColorValue(time, color, validityInterval);

	if(selProperty) {
		OVITO_ASSERT(colorProperty->size() == selProperty->size());
		const int* s = selProperty->constDataInt();
		Color* c = colorProperty->dataColor();
		Color* c_end = c + colorProperty->size();
		if(inputStandardProperty(ParticleProperty::ColorProperty) == nullptr) {
			std::vector<Color> existingColors = inputParticleColors(time, validityInterval);
			OVITO_ASSERT(existingColors.size() == colorProperty->size());
			auto ec = existingColors.cbegin();
			for(; c != c_end; ++c, ++s, ++ec) {
				if(*s)
					*c = color;
				else
					*c = *ec;
			}
		}
		else {
			for(; c != c_end; ++c, ++s)
				if(*s) *c = color;
		}

		// Clear particle selection if requested.
		if(!keepSelection())
			output().removeObject(selProperty);
	}
	else {
		// Assign color to all particles.
		std::fill(colorProperty->dataColor(), colorProperty->dataColor() + colorProperty->size(), color);
	}
	colorProperty->changed();

	return PipelineStatus();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
