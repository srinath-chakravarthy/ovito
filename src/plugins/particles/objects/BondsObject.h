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
#include <core/scene/objects/DataObjectWithSharedStorage.h>
#include <plugins/particles/data/BondsStorage.h>

namespace Ovito { namespace Particles {

/**
 * \brief Stores the bonds between particles.
 */
class OVITO_PARTICLES_EXPORT BondsObject : public DataObjectWithSharedStorage<BondsStorage>
{
public:

	/// \brief Constructor.
	Q_INVOKABLE BondsObject(DataSet* dataset, BondsStorage* storage = nullptr);

	/// \brief Returns the title of this object.
	virtual QString objectTitle() override { return tr("Bonds"); }

	/// Deletes all bonds.
	void clear() {
		modifiableStorage()->clear();
		changed();
	}

	/// Returns the number of half-bonds.
	size_t size() const {
		return storage()->size();
	}

	/// Inserts a new half-bond into the list.
	void addBond(unsigned int index1, unsigned int index2, Vector_3<int8_t> pbcShift = Vector_3<int8_t>::Zero()) {
		modifiableStorage()->push_back(Bond{ pbcShift, index1, index2 });
		changed();
	}

	/// Remaps the bonds after some of the particles have been deleted.
	/// Dangling bonds are removed too and the list of deleted bonds is returned in the bitset object.
	size_t particlesDeleted(const boost::dynamic_bitset<>& deletedParticlesMask, boost::dynamic_bitset<>& deletedBondsMask);

	/// \brief Returns whether this object, when returned as an editable sub-object by another object,
	///        should be displayed in the modification stack.
	virtual bool isSubObjectEditable() const override { return false; }

protected:

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream) override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

private:

	Q_OBJECT
	OVITO_OBJECT
};

}	// End of namespace
}	// End of namespace


