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
#include "BondPropertyObject.h"
#include "BondType.h"

namespace Ovito { namespace Particles {

/**
 * \brief This bond property stores the bond types.
 */
class OVITO_PARTICLES_EXPORT BondTypeProperty : public BondPropertyObject
{
public:

	/// \brief Constructor.
	Q_INVOKABLE BondTypeProperty(DataSet* dataset, BondProperty* storage = nullptr);

	//////////////////////////////////// Specific methods //////////////////////////////////

	/// Appends a bond type to the list of types.
	void addBondType(BondType* type) {
		OVITO_ASSERT(bondTypes().contains(type) == false);
		_bondTypes.push_back(type);
	}

	/// Inserts a bond type into the list of types.
	void insertBondType(int index, BondType* type) {
		OVITO_ASSERT(bondTypes().contains(type) == false);
		_bondTypes.insert(index, type);
	}

	/// Returns the bond type with the given ID, or NULL if no such type exists.
	BondType* bondType(int id) const {
		for(BondType* type : bondTypes())
			if(type->id() == id)
				return type;
		return nullptr;
	}

	/// Returns the bond type with the given name, or NULL if no such type exists.
	BondType* bondType(const QString& name) const {
		for(BondType* type : bondTypes())
			if(type->name() == name)
				return type;
		return nullptr;
	}

	/// Removes a single bond type from this object.
	void removeBondType(int index) {
		_bondTypes.remove(index);
	}

	/// Removes all bond types from this object.
	void clearBondTypes() {
		_bondTypes.clear();
	}

	/// Returns a map from type identifier to color.
	std::map<int, Color> colorMap() const {
		std::map<int, Color> m;
		for(BondType* type : bondTypes())
			m.insert({type->id(), type->color()});
		return m;
	}

	/// Returns a map from type identifier to bond radius.
	std::map<int, FloatType> radiusMap() const {
		std::map<int, FloatType> m;
		for(BondType* type : bondTypes())
			m.insert({type->id(), type->radius()});
		return m;
	}

	//////////////////////////////////// from RefTarget //////////////////////////////////

	/// \brief Returns whether this object, when returned as an editable sub-object by another object,
	///        should be displayed in the modification stack.
	virtual bool isSubObjectEditable() const override { return true; }

	//////////////////////////////////// Default settings ////////////////////////////////

	/// Returns the default color for the bond type with the given ID.
	static Color getDefaultBondColorFromId(BondProperty::Type typeClass, int bondTypeId);

	/// Returns the default color for a named bond type.
	static Color getDefaultBondColor(BondProperty::Type typeClass, const QString& bondTypeName, int bondTypeId, bool userDefaults = true);

	/// Returns the default radius for a named bond type.
	static FloatType getDefaultBondRadius(BondProperty::Type typeClass, const QString& bondTypeName, int bondTypeId, bool userDefaults = true);

private:

	/// Contains the bond types.
	DECLARE_MODIFIABLE_VECTOR_REFERENCE_FIELD(BondType, bondTypes, setBondTypes);

	Q_OBJECT
	OVITO_OBJECT
};

}	// End of namespace
}	// End of namespace


