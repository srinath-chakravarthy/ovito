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


#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/particles/objects/ParticleType.h>
#include "BurgersVectorFamily.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/**
 * \brief A structural pattern (e.g. a lattice type, planar defect type, etc.)
 */
class OVITO_CRYSTALANALYSIS_EXPORT StructurePattern : public ParticleType
{
public:

	/// The types of structures described by a pattern:
	enum StructureType {
		OtherStructure,	// Non of the type below.
		Lattice,		// Three-dimensional crystal lattice.
		Interface,		// Two-dimensional coherent crystal interface, grain boundary, or stacking fault.
		PointDefect		// Zero-dimensional crystal defect.
	};
	Q_ENUMS(StructureType);

	/// The symmetry of the lattice described by the pattern:
	enum SymmetryType {
		OtherSymmetry,		// Unknown symmetry type.
		CubicSymmetry,		// Used for cubic crystals like FCC, BCC, diamond.
		HexagonalSymmetry,	// Used for hexagonal crystals like HCP, hexagonal diamond.
	};
	Q_ENUMS(SymmetryType);

public:

	/// \brief Constructor.
	Q_INVOKABLE StructurePattern(DataSet* dataset);

	/// Returns the long name of this pattern.
	const QString& longName() const { return name(); }

	/// Assigns a long name to this pattern.
	void setLongName(const QString& name) { setName(name); }

	/// Adds a new family to this lattice pattern's list of Burgers vector families.
	void addBurgersVectorFamily(BurgersVectorFamily* family) { _burgersVectorFamilies.push_back(family); }

	/// Removes a family from this lattice pattern's list of Burgers vector families.
	void removeBurgersVectorFamily(int index) { _burgersVectorFamilies.remove(index); }

	/// Returns the default Burgers vector family, which is assigned to dislocation segments that
	/// don't belong to any family.
	BurgersVectorFamily* defaultBurgersVectorFamily() const { return _burgersVectorFamilies.front(); }

	/// Returns the display color to be used for a given Burgers vector.
	static Color getBurgersVectorColor(const QString& latticeName, const Vector3& b);

private:

	/// The short name of this pattern.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, shortName, setShortName);

	/// The type of structure described by this pattern.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(StructureType, structureType, setStructureType);

	/// The type of crystal symmetry of the lattice.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(SymmetryType, symmetryType, setSymmetryType);

	/// List of Burgers vector families.
	DECLARE_MODIFIABLE_VECTOR_REFERENCE_FIELD(BurgersVectorFamily, burgersVectorFamilies, setBurgersVectorFamilies);

	Q_OBJECT
	OVITO_OBJECT
};

}	// End of namespace
}	// End of namespace
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::Plugins::CrystalAnalysis::StructurePattern::StructureType);
Q_DECLARE_TYPEINFO(Ovito::Plugins::CrystalAnalysis::StructurePattern::StructureType, Q_PRIMITIVE_TYPE);
Q_DECLARE_METATYPE(Ovito::Plugins::CrystalAnalysis::StructurePattern::SymmetryType);
Q_DECLARE_TYPEINFO(Ovito::Plugins::CrystalAnalysis::StructurePattern::SymmetryType, Q_PRIMITIVE_TYPE);


