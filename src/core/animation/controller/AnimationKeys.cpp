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
#include "AnimationKeys.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Anim)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(AnimationKey, RefTarget);
DEFINE_PROPERTY_FIELD(AnimationKey, time, "Time");
SET_PROPERTY_FIELD_LABEL(AnimationKey, time, "Time");
SET_PROPERTY_FIELD_UNITS(AnimationKey, time, TimeParameterUnit);

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(FloatAnimationKey, AnimationKey);
DEFINE_PROPERTY_FIELD(FloatAnimationKey, value, "Value");
SET_PROPERTY_FIELD_LABEL(FloatAnimationKey, value, "Value");

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(IntegerAnimationKey, AnimationKey);
DEFINE_PROPERTY_FIELD(IntegerAnimationKey, value, "Value");
SET_PROPERTY_FIELD_LABEL(IntegerAnimationKey, value, "Value");

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(Vector3AnimationKey, AnimationKey);
DEFINE_PROPERTY_FIELD(Vector3AnimationKey, value, "Value");
SET_PROPERTY_FIELD_LABEL(Vector3AnimationKey, value, "Value");

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(PositionAnimationKey, AnimationKey);
DEFINE_PROPERTY_FIELD(PositionAnimationKey, value, "Value");
SET_PROPERTY_FIELD_LABEL(PositionAnimationKey, value, "Value");

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(RotationAnimationKey, AnimationKey);
DEFINE_PROPERTY_FIELD(RotationAnimationKey, value, "Value");
SET_PROPERTY_FIELD_LABEL(RotationAnimationKey, value, "Value");

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(ScalingAnimationKey, AnimationKey);
DEFINE_PROPERTY_FIELD(ScalingAnimationKey, value, "Value");
SET_PROPERTY_FIELD_LABEL(ScalingAnimationKey, value, "Value");

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
