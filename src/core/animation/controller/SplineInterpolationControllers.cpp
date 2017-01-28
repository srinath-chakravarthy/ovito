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
#include "SplineInterpolationControllers.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Anim)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(FloatSplineAnimationKey, FloatAnimationKey);
DEFINE_PROPERTY_FIELD(FloatSplineAnimationKey, inTangent, "InTangent");
DEFINE_PROPERTY_FIELD(FloatSplineAnimationKey, outTangent, "OutTangent");
SET_PROPERTY_FIELD_LABEL(FloatSplineAnimationKey, inTangent, "In Tangent");
SET_PROPERTY_FIELD_LABEL(FloatSplineAnimationKey, outTangent, "Out Tangent");

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(PositionSplineAnimationKey, PositionAnimationKey);
DEFINE_PROPERTY_FIELD(PositionSplineAnimationKey, inTangent, "InTangent");
DEFINE_PROPERTY_FIELD(PositionSplineAnimationKey, outTangent, "OutTangent");
SET_PROPERTY_FIELD_LABEL(PositionSplineAnimationKey, inTangent, "In Tangent");
SET_PROPERTY_FIELD_LABEL(PositionSplineAnimationKey, outTangent, "Out Tangent");

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(SplinePositionController, KeyframeController);

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
