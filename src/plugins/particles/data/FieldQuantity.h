///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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
#include "PropertyBase.h"

namespace Ovito { namespace Particles {

/**
 * \brief Memory storage for a field quantity.
 */
class OVITO_PARTICLES_EXPORT FieldQuantity : public PropertyBase
{
public:

	/// \brief Default constructor that creates an empty, uninitialized storage.
	FieldQuantity() : PropertyBase() {}

	/// \brief Constructor that creates a field quantity storage.
	/// \param shape The number of grid samples along each dimension.
	/// \param dataType Specifies the data type (integer, floating-point, ...) of the elements.
	///                 The data type is specified as identifier according to the Qt metatype system.
	/// \param componentCount The number of components per field value (each of type \a dataType).
	/// \param stride The number of bytes per field value (pass 0 to use the smallest possible stride).
	/// \param name The name of the field quantity.
	/// \param initializeMemory Controls whether the newly allocated memory is initialized with zeros.
	FieldQuantity(std::vector<size_t> shape, int dataType, size_t componentCount, size_t stride, const QString& name, bool initializeMemory);

	/// \brief Copy constructor.
	FieldQuantity(const FieldQuantity& other);

	/// Writes the object to an output stream.
	void saveToStream(SaveStream& stream, bool onlyMetadata = false) const;

	/// Reads the object from an input stream.
	void loadFromStream(LoadStream& stream);

	/// Returns the number of grid points along each dimension. 
	const std::vector<size_t>& shape() const { return _shape; }

protected:

	/// The number of grid points along each dimension. 
	std::vector<size_t> _shape;
};

}	// End of namespace
}	// End of namespace
