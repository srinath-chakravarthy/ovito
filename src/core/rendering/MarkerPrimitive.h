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

#pragma once


#include <core/Core.h>
#include "PrimitiveBase.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Rendering)

/**
 * \brief Abstract base class for particle drawing primitives.
 */
class OVITO_CORE_EXPORT MarkerPrimitive : public PrimitiveBase
{
public:

	enum MarkerShape {
		DotShape
	};
	Q_ENUMS(MarkerShape);

public:

	/// Constructor.
	MarkerPrimitive(MarkerShape shape) : _markerShape(shape) {}

	/// \brief Allocates a geometry buffer with the given number of markers.
	virtual void setCount(int markerCount) = 0;

	/// \brief Returns the number of markers stored in the buffer.
	virtual int markerCount() const = 0;

	/// \brief Sets the coordinates of the markers.
	virtual void setMarkerPositions(const Point3* coordinates) = 0;

	/// \brief Sets the color of all markers to the given value.
	virtual void setMarkerColor(const ColorA color) = 0;

	/// \brief Returns the display shape of markers.
	MarkerShape markerShape() const { return _markerShape; }

private:

	/// Controls the shape of markers.
	MarkerShape _markerShape;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::MarkerPrimitive::MarkerShape);
Q_DECLARE_TYPEINFO(Ovito::MarkerPrimitive::MarkerShape, Q_PRIMITIVE_TYPE);


