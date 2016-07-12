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

#include <plugins/particles/Particles.h>
#include "PropertyBase.h"

namespace Ovito { namespace Particles {

/******************************************************************************
* Default constructor.
******************************************************************************/
PropertyBase::PropertyBase()
	: _stride(0), _dataTypeSize(0),
	_numElements(0), _dataType(QMetaType::Void), _componentCount(0)
{
}

/******************************************************************************
* Constructor.
******************************************************************************/
PropertyBase::PropertyBase(size_t count, int dataType, size_t componentCount, size_t stride, const QString& name, bool initializeMemory)
	: _numElements(0), _dataType(dataType), _dataTypeSize(QMetaType::sizeOf(dataType)),
	  _stride(stride), _componentCount(componentCount), _name(name)
{
	OVITO_ASSERT(_dataTypeSize > 0);
	OVITO_ASSERT(_componentCount > 0);
	if(_stride == 0) _stride = _dataTypeSize * _componentCount;
	OVITO_ASSERT(_stride >= _dataTypeSize * _componentCount);
	OVITO_ASSERT((_stride % _dataTypeSize) == 0);
	if(componentCount > 1) {
		for(size_t i = 1; i <= componentCount; i++)
			_componentNames << QString::number(i);
	}
	resize(count, initializeMemory);
}

/******************************************************************************
* Copy constructor.
******************************************************************************/
PropertyBase::PropertyBase(const PropertyBase& other)
	: _name(other._name), _dataType(other._dataType),
	  _dataTypeSize(other._dataTypeSize), _numElements(other._numElements),
	  _stride(other._stride), _componentCount(other._componentCount),
	  _componentNames(other._componentNames),
	  _data(new uint8_t[_numElements * _stride])
{
	memcpy(_data.get(), other._data.get(), _numElements * _stride);
}

/******************************************************************************
* Denstructor.
******************************************************************************/
PropertyBase::~PropertyBase()
{
}

/******************************************************************************
* Saves the class' contents to the given stream.
******************************************************************************/
void PropertyBase::saveToStream(SaveStream& stream, bool onlyMetadata, int type) const
{
	stream.beginChunk(0x01);
	stream << _name;
	stream << type;
	stream << QByteArray(QMetaType::typeName(_dataType));
	stream.writeSizeT(_dataTypeSize);
	stream.writeSizeT(_stride);
	stream.writeSizeT(_componentCount);
	stream << _componentNames;
	if(onlyMetadata) {
		stream.writeSizeT(0);
	}
	else {
		stream.writeSizeT(_numElements);
		stream.write(_data.get(), _stride * _numElements);
	}
	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
int PropertyBase::loadFromStream(LoadStream& stream)
{
	stream.expectChunk(0x01);
	stream >> _name;
	int type;
	stream >> type;
	QByteArray dataTypeName;
	stream >> dataTypeName;
	_dataType = QMetaType::type(dataTypeName.constData());
	OVITO_ASSERT_MSG(_dataType != 0, "PropertyBase::loadFromStream()", QString("The metadata type '%1' seems to be no longer defined.").arg(QString(dataTypeName)).toLocal8Bit().constData());
	OVITO_ASSERT(dataTypeName == QMetaType::typeName(_dataType));
	stream.readSizeT(_dataTypeSize);
	stream.readSizeT(_stride);
	stream.readSizeT(_componentCount);
	stream >> _componentNames;
	stream.readSizeT(_numElements);
	_data.reset(new uint8_t[_numElements * _stride]);
	stream.read(_data.get(), _stride * _numElements);
	stream.closeChunk();

	// Do floating-point precision conversion from single to double precision.
	if(_dataType == qMetaTypeId<float>() && qMetaTypeId<FloatType>() == qMetaTypeId<double>()) {
		OVITO_ASSERT(sizeof(FloatType) == sizeof(double));
		OVITO_ASSERT(_dataTypeSize == sizeof(float));
		_stride *= sizeof(double) / sizeof(float);
		_dataTypeSize = sizeof(double);
		_dataType = qMetaTypeId<FloatType>();
		std::unique_ptr<uint8_t[]> newBuffer(new uint8_t[_stride * _numElements]);
		double* dst = reinterpret_cast<double*>(newBuffer.get());
		const float* src = reinterpret_cast<const float*>(_data.get());
		for(size_t c = _numElements * _componentCount; c--; )
			*dst++ = (double)*src++;
		_data.swap(newBuffer);
	}

	// Do floating-point precision conversion from double to single precision.
	if(_dataType == qMetaTypeId<double>() && qMetaTypeId<FloatType>() == qMetaTypeId<float>()) {
		OVITO_ASSERT(sizeof(FloatType) == sizeof(float));
		OVITO_ASSERT(_dataTypeSize == sizeof(double));
		_stride /= sizeof(double) / sizeof(float);
		_dataTypeSize = sizeof(float);
		_dataType = qMetaTypeId<FloatType>();
		std::unique_ptr<uint8_t[]> newBuffer(new uint8_t[_stride * _numElements]);
		float* dst = reinterpret_cast<float*>(newBuffer.get());
		const double* src = reinterpret_cast<const double*>(_data.get());
		for(size_t c = _numElements * _componentCount; c--; )
			*dst++ = (float)*src++;
		_data.swap(newBuffer);
	}

	return type;
}

/******************************************************************************
* Resizes the array to the given size.
******************************************************************************/
void PropertyBase::resize(size_t newSize, bool preserveData)
{
	OVITO_ASSERT(newSize >= 0 && newSize < 0xFFFFFFFF);
	std::unique_ptr<uint8_t[]> newBuffer(new uint8_t[newSize * _stride]);
	if(preserveData)
		memcpy(newBuffer.get(), _data.get(), _stride * std::min(_numElements, newSize));
	_data.swap(newBuffer);

	// Initialize new elements to zero.
	if(newSize > _numElements && preserveData) {
		memset(_data.get() + _numElements * _stride, 0, (newSize - _numElements) * _stride);
	}
	_numElements = newSize;
}

/******************************************************************************
* Copies the contents from the given source into this property storage.
* Elements for which the bit in the given mask is set are skipped.
******************************************************************************/
void PropertyBase::filterCopy(const PropertyBase& source, const boost::dynamic_bitset<>& mask)
{
	OVITO_ASSERT(source.size() == mask.size());
	OVITO_ASSERT(stride() == source.stride());
	OVITO_ASSERT(source.size() == mask.count() + this->size());
	size_t oldElementCount = source.size();

	// Optimize filter operation for the most common property types.
	if(stride() == sizeof(FloatType)) {
		// Single float
		const FloatType* src = reinterpret_cast<const FloatType*>(source.constData());
		FloatType* dst = reinterpret_cast<FloatType*>(data());
		for(size_t i = 0; i < oldElementCount; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
	}
	else if(stride() == sizeof(int)) {
		// Single integer
		const int* src = reinterpret_cast<const int*>(source.constData());
		int* dst = reinterpret_cast<int*>(data());
		for(size_t i = 0; i < oldElementCount; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
	}
	else if(stride() == sizeof(Point3)) {
		// Triple float (may actually be four floats when SSE instructions are enabled).
		const Point3* src = reinterpret_cast<const Point3*>(source.constData());
		Point3* dst = reinterpret_cast<Point3*>(data());
		for(size_t i = 0; i < oldElementCount; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
	}
	else if(stride() == sizeof(Color)) {
		// Triple float
		const Color* src = reinterpret_cast<const Color*>(source.constData());
		Color* dst = reinterpret_cast<Color*>(data());
		for(size_t i = 0; i < oldElementCount; ++i, ++src) {
			if(!mask.test(i)) *dst++ = *src;
		}
	}
	else {
		// General case:
		const uint8_t* src = source._data.get();
		uint8_t* dst = _data.get();
		for(size_t i = 0; i < oldElementCount; i++, src += stride()) {
			if(!mask.test(i)) {
				memcpy(dst, src, stride());
				dst += stride();
			}
		}
	}
}

/******************************************************************************
* Copies the contents from the given source into this property storage usnig
* a mapping of indices.
******************************************************************************/
void PropertyBase::mappedCopy(const PropertyBase& source, const std::vector<int>& mapping)
{
	OVITO_ASSERT(source.size() == mapping.size());
	OVITO_ASSERT(stride() == source.stride());

	// Optimize copying operation for the most common property types.
	if(stride() == sizeof(FloatType)) {
		// Single float
		const FloatType* src = reinterpret_cast<const FloatType*>(source.constData());
		FloatType* dst = reinterpret_cast<FloatType*>(data());
		for(size_t i = 0; i < source.size(); ++i, ++src) {
			OVITO_ASSERT(mapping[i] >= 0);
			OVITO_ASSERT(mapping[i] < this->size());
			dst[mapping[i]] = *src;
		}
	}
	else if(stride() == sizeof(int)) {
		// Single integer
		const int* src = reinterpret_cast<const int*>(source.constData());
		int* dst = reinterpret_cast<int*>(data());
		for(size_t i = 0; i < source.size(); ++i, ++src) {
			OVITO_ASSERT(mapping[i] >= 0);
			OVITO_ASSERT(mapping[i] < this->size());
			dst[mapping[i]] = *src;
		}
	}
	else if(stride() == sizeof(Point3)) {
		// Triple float (may actually be four floats when SSE instructions are enabled).
		const Point3* src = reinterpret_cast<const Point3*>(source.constData());
		Point3* dst = reinterpret_cast<Point3*>(data());
		for(size_t i = 0; i < source.size(); ++i, ++src) {
			OVITO_ASSERT(mapping[i] >= 0);
			OVITO_ASSERT(mapping[i] < this->size());
			dst[mapping[i]] = *src;
		}
	}
	else if(stride() == sizeof(Color)) {
		// Triple float
		const Color* src = reinterpret_cast<const Color*>(source.constData());
		Color* dst = reinterpret_cast<Color*>(data());
		for(size_t i = 0; i < source.size(); ++i, ++src) {
			OVITO_ASSERT(mapping[i] >= 0);
			OVITO_ASSERT(mapping[i] < this->size());
			dst[mapping[i]] = *src;
		}
	}
	else {
		// General case:
		const uint8_t* src = source._data.get();
		uint8_t* dst = _data.get();
		for(size_t i = 0; i < source.size(); i++, src += stride()) {
			OVITO_ASSERT(mapping[i] >= 0);
			OVITO_ASSERT(mapping[i] < this->size());
			memcpy(dst + stride() * mapping[i], src, stride());
		}
	}
}

}	// End of namespace
}	// End of namespace
