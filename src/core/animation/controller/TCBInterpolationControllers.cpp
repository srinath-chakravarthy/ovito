///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
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
#include "TCBInterpolationControllers.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Anim)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(Core, FloatTCBAnimationKey, FloatAnimationKey);
DEFINE_PROPERTY_FIELD(FloatTCBAnimationKey, _easeTo, "EaseTo");
DEFINE_PROPERTY_FIELD(FloatTCBAnimationKey, _easeFrom, "EaseFrom");
DEFINE_PROPERTY_FIELD(FloatTCBAnimationKey, _tension, "Tension");
DEFINE_PROPERTY_FIELD(FloatTCBAnimationKey, _continuity, "Continuity");
DEFINE_PROPERTY_FIELD(FloatTCBAnimationKey, _bias, "Bias");
SET_PROPERTY_FIELD_LABEL(FloatTCBAnimationKey, _easeTo, "Ease to");
SET_PROPERTY_FIELD_LABEL(FloatTCBAnimationKey, _easeFrom, "Ease from");
SET_PROPERTY_FIELD_LABEL(FloatTCBAnimationKey, _tension, "Tension");
SET_PROPERTY_FIELD_LABEL(FloatTCBAnimationKey, _continuity, "Continuity");
SET_PROPERTY_FIELD_LABEL(FloatTCBAnimationKey, _bias, "Bias");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(FloatTCBAnimationKey, _easeTo, FloatParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(FloatTCBAnimationKey, _easeFrom, FloatParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(FloatTCBAnimationKey, _tension, FloatParameterUnit, -1, 1);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(FloatTCBAnimationKey, _continuity, FloatParameterUnit, -1, 1);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(FloatTCBAnimationKey, _bias, FloatParameterUnit, -1, 1);

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(Core, PositionTCBAnimationKey, PositionAnimationKey);
DEFINE_PROPERTY_FIELD(PositionTCBAnimationKey, _easeTo, "EaseTo");
DEFINE_PROPERTY_FIELD(PositionTCBAnimationKey, _easeFrom, "EaseFrom");
DEFINE_PROPERTY_FIELD(PositionTCBAnimationKey, _tension, "Tension");
DEFINE_PROPERTY_FIELD(PositionTCBAnimationKey, _continuity, "Continuity");
DEFINE_PROPERTY_FIELD(PositionTCBAnimationKey, _bias, "Bias");
SET_PROPERTY_FIELD_LABEL(PositionTCBAnimationKey, _easeTo, "Ease to");
SET_PROPERTY_FIELD_LABEL(PositionTCBAnimationKey, _easeFrom, "Ease from");
SET_PROPERTY_FIELD_LABEL(PositionTCBAnimationKey, _tension, "Tension");
SET_PROPERTY_FIELD_LABEL(PositionTCBAnimationKey, _continuity, "Continuity");
SET_PROPERTY_FIELD_LABEL(PositionTCBAnimationKey, _bias, "Bias");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(PositionTCBAnimationKey, _easeTo, FloatParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(PositionTCBAnimationKey, _easeFrom, FloatParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(PositionTCBAnimationKey, _tension, FloatParameterUnit, -1, 1);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(PositionTCBAnimationKey, _continuity, FloatParameterUnit, -1, 1);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(PositionTCBAnimationKey, _bias, FloatParameterUnit, -1, 1);

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(Core, TCBPositionController, KeyframeController);

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
