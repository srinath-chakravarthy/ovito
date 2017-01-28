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
#include <plugins/particles/data/ParticleProperty.h>
#include <plugins/particles/objects/ParticleTypeProperty.h>
#include "ParticleType.h"

namespace Ovito { namespace Particles {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(ParticleType, RefTarget);
DEFINE_PROPERTY_FIELD(ParticleType, id, "Identifier");
DEFINE_PROPERTY_FIELD(ParticleType, color, "Color");
DEFINE_PROPERTY_FIELD(ParticleType, radius, "Radius");
DEFINE_PROPERTY_FIELD(ParticleType, name, "Name");
DEFINE_PROPERTY_FIELD(ParticleType, enabled, "Enabled");
SET_PROPERTY_FIELD_LABEL(ParticleType, id, "Id");
SET_PROPERTY_FIELD_LABEL(ParticleType, color, "Color");
SET_PROPERTY_FIELD_LABEL(ParticleType, radius, "Radius");
SET_PROPERTY_FIELD_LABEL(ParticleType, name, "Name");
SET_PROPERTY_FIELD_LABEL(ParticleType, enabled, "Enabled");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ParticleType, radius, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_CHANGE_EVENT(ParticleType, name, ReferenceEvent::TitleChanged);
SET_PROPERTY_FIELD_CHANGE_EVENT(ParticleType, enabled, ReferenceEvent::TargetEnabledOrDisabled);

/******************************************************************************
* Constructs a new ParticleType.
******************************************************************************/
ParticleType::ParticleType(DataSet* dataset) : RefTarget(dataset), _color(1,1,1), _radius(0), _id(0), _enabled(true)
{
	INIT_PROPERTY_FIELD(id);
	INIT_PROPERTY_FIELD(color);
	INIT_PROPERTY_FIELD(radius);
	INIT_PROPERTY_FIELD(name);
	INIT_PROPERTY_FIELD(enabled);
}

}	// End of namespace
}	// End of namespace
