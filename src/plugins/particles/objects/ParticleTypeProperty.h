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
#include "ParticlePropertyObject.h"
#include "ParticleType.h"

namespace Ovito { namespace Particles {

/**
 * \brief This particle property stores the particle types.
 */
class OVITO_PARTICLES_EXPORT ParticleTypeProperty : public ParticlePropertyObject
{
public:

	enum PredefinedParticleType {
		H,He,Li,C,N,O,Na,Mg,Al,Si,K,Ca,Ti,Cr,Fe,Co,Ni,Cu,Zn,Ga,Ge,Kr,Sr,Y,Zr,Nb,Pd,Pt,W,Au,

		NUMBER_OF_PREDEFINED_PARTICLE_TYPES
	};

	enum PredefinedStructureType {
		OTHER = 0,					//< Unidentified structure
		FCC,						//< Face-centered cubic
		HCP,						//< Hexagonal close-packed
		BCC,						//< Body-centered cubic
		ICO,						//< Icosahedral structure
		CUBIC_DIAMOND,				//< Cubic diamond structure
		CUBIC_DIAMOND_FIRST_NEIGH,	//< First neighbor of a cubic diamond atom
		CUBIC_DIAMOND_SECOND_NEIGH,	//< Second neighbor of a cubic diamond atom
		HEX_DIAMOND,				//< Hexagonal diamond structure
		HEX_DIAMOND_FIRST_NEIGH,	//< First neighbor of a hexagonal diamond atom
		HEX_DIAMOND_SECOND_NEIGH,	//< Second neighbor of a hexagonal diamond atom
		SC,							//< Simple cubic structure

		NUMBER_OF_PREDEFINED_STRUCTURE_TYPES
	};

public:

	/// \brief Constructor.
	Q_INVOKABLE ParticleTypeProperty(DataSet* dataset, ParticleProperty* storage = nullptr);

	//////////////////////////////////// Specific methods //////////////////////////////////

	/// Appends a particle type to the list of types.
	void addParticleType(ParticleType* ptype) {
		OVITO_ASSERT(particleTypes().contains(ptype) == false);
		_particleTypes.push_back(ptype);
	}

	/// Inserts a particle type into the list of types.
	void insertParticleType(int index, ParticleType* ptype) {
		OVITO_ASSERT(particleTypes().contains(ptype) == false);
		_particleTypes.insert(index, ptype);
	}

	/// Returns the particle type with the given ID, or NULL if no such type exists.
	ParticleType* particleType(int id) const {
		for(ParticleType* ptype : particleTypes())
			if(ptype->id() == id)
				return ptype;
		return nullptr;
	}

	/// Returns the particle type with the given name, or NULL if no such type exists.
	ParticleType* particleType(const QString& name) const {
		for(ParticleType* ptype : particleTypes())
			if(ptype->name() == name)
				return ptype;
		return nullptr;
	}

	/// Removes a single particle type from this object.
	void removeParticleType(int index) {
		_particleTypes.remove(index);
	}

	/// Removes all particle types from this object.
	void clearParticleTypes() {
		_particleTypes.clear();
	}

	/// Returns a map from type identifier to color.
	std::map<int,Color> colorMap() const {
		std::map<int,Color> m;
		for(ParticleType* ptype : particleTypes())
			m.insert({ptype->id(), ptype->color()});
		return m;
	}

	/// Returns a map from type identifier to particle radius.
	std::map<int,FloatType> radiusMap() const {
		std::map<int,FloatType> m;
		for(ParticleType* ptype : particleTypes())
			m.insert({ptype->id(), ptype->radius()});
		return m;
	}

	//////////////////////////////////// from RefTarget //////////////////////////////////

	/// \brief Returns whether this object, when returned as an editable sub-object by another object,
	///        should be displayed in the modification stack.
	virtual bool isSubObjectEditable() const override { return true; }

	//////////////////////////////////// Default settings ////////////////////////////////

	/// Returns the name string of a predefined particle type.
	static const QString& getPredefinedParticleTypeName(PredefinedParticleType predefType) {
		OVITO_ASSERT(predefType < NUMBER_OF_PREDEFINED_PARTICLE_TYPES);
		return std::get<0>(_predefinedParticleTypes[predefType]);
	}

	/// Returns the name string of a predefined structure type.
	static const QString& getPredefinedStructureTypeName(PredefinedStructureType predefType) {
		OVITO_ASSERT(predefType < NUMBER_OF_PREDEFINED_STRUCTURE_TYPES);
		return std::get<0>(_predefinedStructureTypes[predefType]);
	}

	/// Returns the default color for the particle type with the given ID.
	static Color getDefaultParticleColorFromId(ParticleProperty::Type typeClass, int particleTypeId);

	/// Returns the default color for a named particle type.
	static Color getDefaultParticleColor(ParticleProperty::Type typeClass, const QString& particleTypeName, int particleTypeId, bool userDefaults = true);

	/// Changes the default color for a named particle type.
	static void setDefaultParticleColor(ParticleProperty::Type typeClass, const QString& particleTypeName, const Color& color);

	/// Returns the default radius for a named particle type.
	static FloatType getDefaultParticleRadius(ParticleProperty::Type typeClass, const QString& particleTypeName, int particleTypeId, bool userDefaults = true);

	/// Changes the default radius for a named particle type.
	static void setDefaultParticleRadius(ParticleProperty::Type typeClass, const QString& particleTypeName, FloatType radius);

private:

	/// Data structure that holds the name, color, and radius of a particle type.
	typedef std::tuple<QString,Color,FloatType> PredefinedTypeInfo;

	/// Contains default names, colors, and radii for some predefined particle types.
	static std::array<PredefinedTypeInfo, NUMBER_OF_PREDEFINED_PARTICLE_TYPES> _predefinedParticleTypes;

	/// Contains default names, colors, and radii for the predefined structure types.
	static std::array<PredefinedTypeInfo, NUMBER_OF_PREDEFINED_STRUCTURE_TYPES> _predefinedStructureTypes;

	/// Contains the particle types.
	DECLARE_MODIFIABLE_VECTOR_REFERENCE_FIELD(ParticleType, particleTypes, setParticleTypes);

	Q_OBJECT
	OVITO_OBJECT
};

}	// End of namespace
}	// End of namespace


