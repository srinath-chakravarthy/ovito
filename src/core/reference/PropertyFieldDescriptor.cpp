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

#include <core/Core.h>
#include <core/utilities/units/UnitsManager.h>
#include <core/reference/RefMaker.h>
#include "PropertyFieldDescriptor.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem)

/******************************************************************************
* Return the human readable and localized name of the parameter field.
* This information is parsed from the plugin manifest file.
******************************************************************************/
QString PropertyFieldDescriptor::displayName() const
{
	if(_displayName.isEmpty())
		return identifier();
	else
		return _displayName;
}

/******************************************************************************
* Saves the current value of a property field in the application's settings store.
******************************************************************************/
void PropertyFieldDescriptor::memorizeDefaultValue(RefMaker* object) const
{
	OVITO_CHECK_OBJECT_POINTER(object);
	QSettings settings;
	settings.beginGroup(definingClass()->plugin()->pluginId());
	settings.beginGroup(definingClass()->name());
	QVariant v = object->getPropertyFieldValue(*this);
	// Workaround for bug in Qt 5.7.0: QVariants of type float for not get correctly stored
	// by QSettings (at least on macOS), because QVariant::Float is not an official type.
	if((QMetaType::Type)v.type() == QMetaType::Float)
		v = QVariant::fromValue((double)v.toFloat());
	settings.setValue(identifier(), v);

	//qDebug() << "Saving default value for parameter" << identifier() << "of class" << definingClass()->name() << 
	//	"to settings store" << settings.fileName() << ":" << object->getPropertyFieldValue(*this) <<
	//	"(type=" << object->getPropertyFieldValue(*this).type() << ")";
}

/******************************************************************************
* Loads the default value of a property field from the application's settings store.
******************************************************************************/
bool PropertyFieldDescriptor::loadDefaultValue(RefMaker* object) const
{
	OVITO_CHECK_OBJECT_POINTER(object);
	QSettings settings;
	settings.beginGroup(definingClass()->plugin()->pluginId());
	settings.beginGroup(definingClass()->name());
	QVariant v = settings.value(identifier());
	if(!v.isNull()) {
		//qDebug() << "Loading default value for parameter" << identifier() << "of class" << definingClass()->name() << ":" << v;
		object->setPropertyFieldValue(*this, v);
		return true;
	}
	return false;
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
