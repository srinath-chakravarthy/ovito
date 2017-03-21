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

#pragma once


#include <core/Core.h>
#include "PrimitiveBase.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Rendering)

/**
 * \brief Abstract base class for rendering triangle meshes.
 */
class OVITO_CORE_EXPORT MeshPrimitive : public PrimitiveBase
{
public:

	/// Sets the mesh to be stored in this buffer object.
	virtual void setMesh(const TriMesh& mesh, const ColorA& meshColor) = 0;

	/// \brief Returns the number of triangle faces stored in the buffer.
	virtual int faceCount() = 0;

	/// \brief Enables or disables the culling of triangles not facing the viewer.
	void setCullFaces(bool enable) { _cullFaces = enable; }

	/// \brief Returns whether the culling of triangles not facing the viewer is enabled.
	bool cullFaces() const { return _cullFaces; }

	/// Returns the array of materials referenced by the materialIndex() field of the mesh faces.
	const std::vector<ColorA>& materialColors() const { return _materialColors; }

	/// Sets array of materials referenced by the materialIndex() field of the mesh faces.
	void setMaterialColors(std::vector<ColorA> colors) {
		_materialColors = std::move(colors);
	}

private:

	/// Controls the culling of triangles not facing the viewer.
	bool _cullFaces = false;

	/// The array of materials referenced by the materialIndex() field of the mesh faces.
	std::vector<ColorA> _materialColors;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace


