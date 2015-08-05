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
#include <plugins/particles/objects/BondPropertyObject.h>
#include "BondProperty.h"

namespace Ovito { namespace Particles {

/******************************************************************************
* Constructor for user-defined properties.
******************************************************************************/
BondProperty::BondProperty(size_t bondsCount, int dataType, size_t componentCount, size_t stride, const QString& name, bool initializeMemory)
	: PropertyBase(bondsCount, dataType, componentCount, stride, name, initializeMemory), _type(UserProperty)
{
}

/******************************************************************************
* Constructor for standard properties.
******************************************************************************/
BondProperty::BondProperty(size_t bondsCount, Type type, size_t componentCount, bool initializeMemory)
	: PropertyBase(), _type(type)
{
	switch(type) {
	case BondTypeProperty:
	case SelectionProperty:
		_dataType = qMetaTypeId<int>();
		_componentCount = 1;
		_stride = sizeof(int);
		break;
	case ColorProperty:
		_dataType = qMetaTypeId<FloatType>();
		_componentCount = 3;
		_stride = _componentCount * sizeof(FloatType);
		OVITO_ASSERT(_stride == sizeof(Color));
		break;
	default:
		OVITO_ASSERT_MSG(false, "BondProperty constructor", "Invalid standard property type");
		throw Exception(BondPropertyObject::tr("This is not a valid standard bond property type: %1").arg(type));
	}
	_dataTypeSize = QMetaType::sizeOf(_dataType);
	OVITO_ASSERT(_dataTypeSize > 0);
	OVITO_ASSERT_MSG(componentCount == 0 || componentCount == _componentCount, "BondProperty::BondProperty(type)", "Cannot specify component count for a standard property with a fixed component count.");
	OVITO_ASSERT(_stride >= _dataTypeSize * _componentCount);
	OVITO_ASSERT((_stride % _dataTypeSize) == 0);

	_componentNames = standardPropertyComponentNames(type, _componentCount);
	_name = standardPropertyName(type);
	resize(bondsCount, initializeMemory);
}

/******************************************************************************
* Copy constructor.
******************************************************************************/
BondProperty::BondProperty(const BondProperty& other)
	: PropertyBase(other), _type(other._type)
{
}

/******************************************************************************
* Saves the class' contents to the given stream.
******************************************************************************/
void BondProperty::saveToStream(SaveStream& stream, bool onlyMetadata) const
{
	PropertyBase::saveToStream(stream, onlyMetadata, type());
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void BondProperty::loadFromStream(LoadStream& stream)
{
	_type = (Type)PropertyBase::loadFromStream(stream);
}

/******************************************************************************
* Returns the name of a standard property.
******************************************************************************/
QString BondProperty::standardPropertyName(Type which)
{
	switch(which) {
	case BondTypeProperty: return BondPropertyObject::tr("Bond Type");
	case SelectionProperty: return BondPropertyObject::tr("Selection");
	case ColorProperty: return BondPropertyObject::tr("Color");
	default:
		OVITO_ASSERT_MSG(false, "BondProperty::standardPropertyName", "Invalid standard bond property type");
		throw Exception(BondPropertyObject::tr("This is not a valid standard bond property type: %1").arg(which));
	}
}

/******************************************************************************
* Returns the display title used for a standard property object.
******************************************************************************/
QString BondProperty::standardPropertyTitle(Type which)
{
	switch(which) {
	case BondTypeProperty: return BondPropertyObject::tr("Bond types");
	case ColorProperty: return BondPropertyObject::tr("Bond colors");
	default:
		return standardPropertyName(which);
	}
}

/******************************************************************************
* Returns the data type used by the given standard property.
******************************************************************************/
int BondProperty::standardPropertyDataType(Type which)
{
	switch(which) {
	case BondTypeProperty:
	case SelectionProperty:
		return qMetaTypeId<int>();
	case ColorProperty:
		return qMetaTypeId<FloatType>();
	default:
		OVITO_ASSERT_MSG(false, "BondProperty::standardPropertyDataType", "Invalid standard bond property type");
		throw Exception(BondPropertyObject::tr("This is not a valid standard bond property type: %1").arg(which));
	}
}

/******************************************************************************
* Returns a list with the names and identifiers of all defined standard properties.
******************************************************************************/
QMap<QString, BondProperty::Type> BondProperty::standardPropertyList()
{
	static QMap<QString, Type> table;
	if(table.empty()) {
		table.insert(standardPropertyName(BondTypeProperty), BondTypeProperty);
		table.insert(standardPropertyName(SelectionProperty), SelectionProperty);
		table.insert(standardPropertyName(ColorProperty), ColorProperty);
	}
	return table;
}

/******************************************************************************
* Returns the number of vector components per atom used by the given standard data channel.
******************************************************************************/
size_t BondProperty::standardPropertyComponentCount(Type which)
{
	switch(which) {
	case BondTypeProperty:
	case SelectionProperty:
		return 1;
	case ColorProperty:
		return 3;
	default:
		OVITO_ASSERT_MSG(false, "BondProperty::standardPropertyComponentCount", "Invalid standard bond property type");
		throw Exception(BondPropertyObject::tr("This is not a valid standard bond property type: %1").arg(which));
	}
}

/******************************************************************************
* Returns the list of component names for the given standard property.
******************************************************************************/
QStringList BondProperty::standardPropertyComponentNames(Type which, size_t componentCount)
{
	const static QStringList emptyList;
	const static QStringList rgbList = QStringList() << "R" << "G" << "B";

	switch(which) {
	case BondTypeProperty:
	case SelectionProperty:
		return emptyList;
	case ColorProperty:
		return rgbList;
	default:
		OVITO_ASSERT_MSG(false, "BondProperty::standardPropertyComponentNames", "Invalid standard bond property type");
		throw Exception(BondPropertyObject::tr("This is not a valid standard bond property type: %1").arg(which));
	}
}

}	// End of namespace
}	// End of namespace
