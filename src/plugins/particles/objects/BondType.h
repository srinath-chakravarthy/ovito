///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2015) Alexander Stukowski
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
 * \brief Stores the properties of a bond type, e.g. name, color, and radius.
 *
 * \ingroup particles_objects
 */
class OVITO_PARTICLES_EXPORT BondType : public RefTarget
{
public:

	/// \brief Constructs a new bond type.
	Q_INVOKABLE BondType(DataSet* dataset);

	/// Returns the title of this object.
	virtual QString objectTitle() override { return name(); }

protected:

	/// Stores the identifier of the bond type.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, id, setId);

	/// The name of this bond type.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, name, setName);

	/// Stores the color of the bond type.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(Color, color, setColor);

	/// Stores the radius of the bond type.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, radius, setRadius);

private:

	Q_OBJECT
	OVITO_OBJECT
};

}	// End of namespace
}	// End of namespace


