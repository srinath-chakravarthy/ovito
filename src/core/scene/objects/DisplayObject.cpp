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
#include "DisplayObject.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(DisplayObject, RefTarget);
DEFINE_PROPERTY_FIELD(DisplayObject, isEnabled, "IsEnabled");
SET_PROPERTY_FIELD_LABEL(DisplayObject, isEnabled, "Enabled");
SET_PROPERTY_FIELD_CHANGE_EVENT(DisplayObject, isEnabled, ReferenceEvent::TargetEnabledOrDisabled);
DEFINE_PROPERTY_FIELD(DisplayObject, title, "Name");
SET_PROPERTY_FIELD_LABEL(DisplayObject, title, "Name");
SET_PROPERTY_FIELD_CHANGE_EVENT(DisplayObject, title, ReferenceEvent::TitleChanged);

/******************************************************************************
* Constructor.
******************************************************************************/
DisplayObject::DisplayObject(DataSet* dataset) : RefTarget(dataset), _isEnabled(true)
{
	INIT_PROPERTY_FIELD(isEnabled);
	INIT_PROPERTY_FIELD(title);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
