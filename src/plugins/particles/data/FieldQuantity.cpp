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

#include <plugins/particles/Particles.h>
#include "FieldQuantity.h"

namespace Ovito { namespace Particles {

/******************************************************************************
* Constructor.
******************************************************************************/
FieldQuantity::FieldQuantity(std::vector<size_t> shape, int dataType, size_t componentCount, size_t stride, const QString& name, bool initializeMemory)
	: PropertyBase(std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<size_t>()), dataType, componentCount, stride, name, initializeMemory), _shape(std::move(shape))
{
}

/******************************************************************************
* Copy constructor.
******************************************************************************/
FieldQuantity::FieldQuantity(const FieldQuantity& other)
	: PropertyBase(other), _shape(other._shape)
{
}

/******************************************************************************
* Saves the class' contents to the given stream.
******************************************************************************/
void FieldQuantity::saveToStream(SaveStream& stream, bool onlyMetadata) const
{
	PropertyBase::saveToStream(stream, onlyMetadata, 0);
	stream.beginChunk(0x01);
	stream.writeSizeT(shape().size());
	for(size_t d : shape())
		stream.writeSizeT(d);
	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void FieldQuantity::loadFromStream(LoadStream& stream)
{
	PropertyBase::loadFromStream(stream);
	stream.expectChunk(0x01);
	size_t ndim;
	stream.readSizeT(ndim);
	_shape.resize(ndim);
	for(size_t d = 0; d < ndim; d++) 
		stream.readSizeT(_shape[d]);
	stream.closeChunk();
}

}	// End of namespace
}	// End of namespace
