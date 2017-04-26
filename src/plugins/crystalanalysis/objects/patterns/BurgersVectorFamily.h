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


#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <core/reference/RefTarget.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/**
 * \brief Stores properties of an atom type.
 */
class OVITO_CRYSTALANALYSIS_EXPORT BurgersVectorFamily : public RefTarget
{
public:

	/// \brief Constructs a new BurgersVectorFamily.
	Q_INVOKABLE BurgersVectorFamily(DataSet* dataset, const QString& name = QString(), const Vector3& burgersVector = Vector3::Zero(), const Color& color = Color(0,0,0));

	/// Checks if the given Burgers vector is a member of this family.
	bool isMember(const Vector3& v, StructurePattern* latticeStructure) const;

	/// Returns the title of this object.
	virtual QString objectTitle() override { return name(); }

public:

	Q_PROPERTY(QString name READ name WRITE setName);
	Q_PROPERTY(Color color READ color WRITE setColor);

private:

	/// The name of this atom type.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, name, setName);

	/// This visualization color of this family.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(Color, color, setColor);

	/// This prototype Burgers vector of this family.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(Vector3, burgersVector, setBurgersVector);

	Q_OBJECT
	OVITO_OBJECT
};

}	// End of namespace
}	// End of namespace
}	// End of namespace


