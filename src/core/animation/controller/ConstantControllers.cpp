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
#include "ConstantControllers.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Anim)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(ConstFloatController, Controller);
IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(ConstIntegerController, Controller);
IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(ConstVectorController, Controller);
IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(ConstPositionController, Controller);
IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(ConstRotationController, Controller);
IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(ConstScalingController, Controller);
DEFINE_PROPERTY_FIELD(ConstFloatController, value, "Value");
DEFINE_PROPERTY_FIELD(ConstIntegerController, value, "Value");
DEFINE_PROPERTY_FIELD(ConstVectorController, value, "Value");
DEFINE_PROPERTY_FIELD(ConstPositionController, value, "Value");
DEFINE_PROPERTY_FIELD(ConstRotationController, value, "Value");
DEFINE_PROPERTY_FIELD(ConstScalingController, value, "Value");

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
