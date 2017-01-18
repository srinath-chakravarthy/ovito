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
#include "BondTypeProperty.h"

namespace Ovito { namespace Particles {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(BondTypeProperty, BondPropertyObject);
DEFINE_VECTOR_REFERENCE_FIELD(BondTypeProperty, _bondTypes, "BondTypes", BondType);
SET_PROPERTY_FIELD_LABEL(BondTypeProperty, _bondTypes, "Bond Types");

/******************************************************************************
* Constructor.
******************************************************************************/
BondTypeProperty::BondTypeProperty(DataSet* dataset, BondProperty* storage)
	: BondPropertyObject(dataset, storage)
{
	INIT_PROPERTY_FIELD(BondTypeProperty::_bondTypes);
}

/******************************************************************************
* Returns the default color for a bond type ID.
******************************************************************************/
Color BondTypeProperty::getDefaultBondColorFromId(BondProperty::Type typeClass, int bondTypeId)
{
	// Assign initial standard color to new bond types.
	static const Color defaultTypeColors[] = {
		Color(1.0f,1.0f,0.0f),
		Color(0.7f,0.0f,1.0f),
		Color(0.2f,1.0f,1.0f),
		Color(1.0f,0.4f,1.0f),
		Color(0.4f,1.0f,0.4f),
		Color(1.0f,0.4f,0.4f),
		Color(0.4f,0.4f,1.0f),
		Color(1.0f,1.0f,0.7f),
		Color(0.97f,0.97f,0.97f)
	};
	return defaultTypeColors[std::abs(bondTypeId) % (sizeof(defaultTypeColors) / sizeof(defaultTypeColors[0]))];
}

/******************************************************************************
* Returns the default color for a bond type name.
******************************************************************************/
Color BondTypeProperty::getDefaultBondColor(BondProperty::Type typeClass, const QString& bondTypeName, int bondTypeId, bool userDefaults)
{
	if(userDefaults) {
		QSettings settings;
		settings.beginGroup("bonds/defaults/color");
		settings.beginGroup(QString::number((int)typeClass));
		QVariant v = settings.value(bondTypeName);
		if(v.isValid() && v.canConvert<Color>())
			return v.value<Color>();
	}

	return getDefaultBondColorFromId(typeClass, bondTypeId);
}

/******************************************************************************
* Returns the default radius for a bond type name.
******************************************************************************/
FloatType BondTypeProperty::getDefaultBondRadius(BondProperty::Type typeClass, const QString& bondTypeName, int bondTypeId, bool userDefaults)
{
	if(userDefaults) {
		QSettings settings;
		settings.beginGroup("bonds/defaults/radius");
		settings.beginGroup(QString::number((int)typeClass));
		QVariant v = settings.value(bondTypeName);
		if(v.isValid() && v.canConvert<FloatType>())
			return v.value<FloatType>();
	}

	return 0;
}

}	// End of namespace
}	// End of namespace
