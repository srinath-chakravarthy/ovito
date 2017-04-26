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


#include <plugins/particles/Particles.h>
#include <core/reference/RefTarget.h>

namespace Ovito { namespace Particles {

/**
 * \brief Stores the properties of a particle type, e.g. name, color, and radius.
 *
 * \ingroup particles_objects
 */
class OVITO_PARTICLES_EXPORT ParticleType : public RefTarget
{
public:

	/// \brief Constructs a new particle type.
	Q_INVOKABLE ParticleType(DataSet* dataset);

	/// Returns the title of this object.
	virtual QString objectTitle() override { return name(); }

protected:

	/// Stores the identifier of the particle type.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, id, setId);

	/// The name of this particle type.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, name, setName);

	/// Stores the color of the particle type.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(Color, color, setColor);

	/// Stores the radius of the particle type.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, radius, setRadius);

	/// Stores whether this type is enabled or disabled.
	/// This controls, e.g., the search for this structure type by structure identification modifiers.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, enabled, setEnabled);

	Q_OBJECT
	OVITO_OBJECT
};

}	// End of namespace
}	// End of namespace


