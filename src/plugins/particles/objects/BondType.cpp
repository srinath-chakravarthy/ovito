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
#include "BondType.h"

namespace Ovito { namespace Particles {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(BondType, RefTarget);
DEFINE_PROPERTY_FIELD(BondType, id, "Identifier");
DEFINE_PROPERTY_FIELD(BondType, color, "Color");
DEFINE_PROPERTY_FIELD(BondType, radius, "Radius");
DEFINE_PROPERTY_FIELD(BondType, name, "Name");
SET_PROPERTY_FIELD_LABEL(BondType, id, "Id");
SET_PROPERTY_FIELD_LABEL(BondType, color, "Color");
SET_PROPERTY_FIELD_LABEL(BondType, radius, "Radius");
SET_PROPERTY_FIELD_LABEL(BondType, name, "Name");
SET_PROPERTY_FIELD_UNITS(BondType, radius, WorldParameterUnit);
SET_PROPERTY_FIELD_CHANGE_EVENT(BondType, name, ReferenceEvent::TitleChanged);

/******************************************************************************
* Constructs a new BondType.
******************************************************************************/
BondType::BondType(DataSet* dataset) : RefTarget(dataset), _color(1,1,1), _radius(0), _id(0)
{
	INIT_PROPERTY_FIELD(id);
	INIT_PROPERTY_FIELD(color);
	INIT_PROPERTY_FIELD(radius);
	INIT_PROPERTY_FIELD(name);
}

}	// End of namespace
}	// End of namespace
