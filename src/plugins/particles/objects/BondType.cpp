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

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(Particles, BondType, RefTarget);
DEFINE_PROPERTY_FIELD(BondType, _id, "Identifier");
DEFINE_PROPERTY_FIELD(BondType, _color, "Color");
DEFINE_PROPERTY_FIELD(BondType, _radius, "Radius");
DEFINE_PROPERTY_FIELD(BondType, _name, "Name");
SET_PROPERTY_FIELD_LABEL(BondType, _id, "Id");
SET_PROPERTY_FIELD_LABEL(BondType, _color, "Color");
SET_PROPERTY_FIELD_LABEL(BondType, _radius, "Radius");
SET_PROPERTY_FIELD_LABEL(BondType, _name, "Name");
SET_PROPERTY_FIELD_UNITS(BondType, _radius, WorldParameterUnit);

/******************************************************************************
* Constructs a new BondType.
******************************************************************************/
BondType::BondType(DataSet* dataset) : RefTarget(dataset), _color(1,1,1), _radius(0), _id(0)
{
	INIT_PROPERTY_FIELD(BondType::_id);
	INIT_PROPERTY_FIELD(BondType::_color);
	INIT_PROPERTY_FIELD(BondType::_radius);
	INIT_PROPERTY_FIELD(BondType::_name);
}

}	// End of namespace
}	// End of namespace
