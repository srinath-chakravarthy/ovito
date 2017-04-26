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
#include <core/rendering/MarkerPrimitive.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Rendering)

/**
 * \brief Buffer object that stores a set of markers to be rendered by a non-interactive renderer.
 */
class OVITO_CORE_EXPORT DefaultMarkerPrimitive : public MarkerPrimitive
{
public:

	/// Constructor.
	DefaultMarkerPrimitive(MarkerShape shape) : MarkerPrimitive(shape) {}

	/// \brief Allocates a geometry buffer with the given number of markers.
	virtual void setCount(int markerCount) override {
		OVITO_ASSERT(markerCount >= 0);
		_positionsBuffer.resize(markerCount);
	}

	/// \brief Returns the number of markers stored in the buffer.
	virtual int markerCount() const override { return _positionsBuffer.size(); }

	/// \brief Sets the coordinates of the markers.
	virtual void setMarkerPositions(const Point3* coordinates) override {
		std::copy(coordinates, coordinates + _positionsBuffer.size(), _positionsBuffer.begin());
	}

	/// \brief Sets the color of all markers to the given value.
	virtual void setMarkerColor(const ColorA color) override {}

	/// \brief Returns true if the geometry buffer is filled and can be rendered with the given renderer.
	virtual bool isValid(SceneRenderer* renderer) override;

	/// \brief Renders the geometry.
	virtual void render(SceneRenderer* renderer) override;

	/// Returns a reference to the internal buffer that stores the marker positions.
	const std::vector<Point3>& positions() const { return _positionsBuffer; }

private:

	/// The internal buffer that stores the marker positions.
	std::vector<Point3> _positionsBuffer;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace


