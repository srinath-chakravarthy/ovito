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
#include <boost/iterator/iterator_facade.hpp>
#include <boost/range/iterator_range.hpp>

namespace Ovito { namespace Particles {

/**
 * A single bond between two particles.
 */
struct Bond {

	/// If the bond crosses a periodic boundary, this tells us
	/// in which direction.
	Vector_3<int8_t> pbcShift;

	/// The index of the first particle.
	/// Note that we are not using size_t here to save memory.
	unsigned int index1;

	/// The index of the second particle.
	/// Note that we are not using size_t here to save memory.
	unsigned int index2;
};

/**
 * \brief List of bonds, which connect pairs of particles.
 */
class OVITO_PARTICLES_EXPORT BondsStorage : public std::vector<Bond>, public QSharedData
{
public:

	/// Writes the stored data to an output stream.
	void saveToStream(SaveStream& stream, bool onlyMetadata = false) const;

	/// Reads the stored data from an input stream.
	void loadFromStream(LoadStream& stream);
};

/**
 * \brief Helper class that allows to efficiently iterate over the half-bonds that are adjacent to a particle.
 */
class OVITO_PARTICLES_EXPORT ParticleBondMap
{
public:

	class bond_index_iterator : public boost::iterator_facade<bond_index_iterator, size_t const, boost::forward_traversal_tag, size_t> {
	public:
		bond_index_iterator() : _bondMap(nullptr), _currentIndex(0) {}
		bond_index_iterator(const ParticleBondMap* map, size_t startIndex) :
			_bondMap(map), _currentIndex(startIndex) {}
	private:
		size_t _currentIndex;
		const ParticleBondMap* _bondMap;

		friend class boost::iterator_core_access;

		void increment() {
			_currentIndex = _bondMap->nextBondOfParticle(_currentIndex);
		}

		bool equal(const bond_index_iterator& other) const {
			OVITO_ASSERT(_bondMap == other._bondMap);
			return this->_currentIndex == other._currentIndex;
		}

		size_t dereference() const {
			OVITO_ASSERT(_currentIndex < _bondMap->_nextBond.size());
			return _currentIndex;
		}
	};

public:

	/// Initializes the helper class.
	ParticleBondMap(const BondsStorage& bonds);

	/// Returns an iterator range over the indices of the half-bonds adjacent to the given particle.
	boost::iterator_range<bond_index_iterator> bondsOfParticle(size_t particleIndex) const {
		return boost::iterator_range<bond_index_iterator>(
				bond_index_iterator(this, firstBondOfParticle(particleIndex)),
				bond_index_iterator(this, endOfListValue()));
	}

	/// Returns the index of the first half-bond adjacent to the given particle.
	/// Returns the half-bond count to indicate that the particle has no bonds at all.
	size_t firstBondOfParticle(size_t particleIndex) const {
		return particleIndex < _startIndices.size() ? _startIndices[particleIndex] : endOfListValue();
	}

	/// Returns the index of the next half-bond in the linked list of half-bonds of a particle.
	/// Returns the half-bond count to indicate that the end of the particle's bond list has been reached.
	size_t nextBondOfParticle(size_t bondIndex) const {
		OVITO_ASSERT(bondIndex < _nextBond.size());
		return _nextBond[bondIndex];
	}

	/// Returns the number of half bonds, which is used to indicate the end of the per-particle bond list.
	size_t endOfListValue() const { return _nextBond.size(); }

	/// Returns the index of a bond in the bonds list.
	/// Returns the total half-bond count in case such a bond does not exist.
	size_t findBond(const Bond& bond) const {
		size_t bindex;
		for(bindex = firstBondOfParticle(bond.index1); bindex != endOfListValue(); bindex = nextBondOfParticle(bindex)) {
			OVITO_ASSERT(_bonds[bindex].index1 == bond.index1);
			if(_bonds[bindex].index2 == bond.index2 && _bonds[bindex].pbcShift == bond.pbcShift)
				break;
		}
		return bindex;
	}

private:

	/// Contains the first half-bond index for each particle.
	std::vector<size_t> _startIndices;

	/// Stores the index of the next half-bond of particle in the linked list.
	std::vector<size_t> _nextBond;

	/// The bonds storage this map has been created for.
	const BondsStorage& _bonds;
};

}	// End of namespace
}	// End of namespace


