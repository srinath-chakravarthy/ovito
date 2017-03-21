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


#include <core/Core.h>
#include "PropertyFieldDescriptor.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/******************************************************************************
* This structure describes one member field of a RefMaker object that stores
* a property of that object.
******************************************************************************/
class OVITO_CORE_EXPORT NativePropertyFieldDescriptor : public PropertyFieldDescriptor
{
public:

	/// Constructor	for a property field that stores a non-animatable parameter.
	NativePropertyFieldDescriptor(const NativeOvitoObjectType* definingClass, const char* identifier, PropertyFieldFlags flags,
			QVariant (*_propertyStorageReadFunc)(RefMaker*), void (*_propertyStorageWriteFunc)(RefMaker*, const QVariant&),
			void (*_propertyStorageSaveFunc)(RefMaker*, SaveStream&), void (*_propertyStorageLoadFunc)(RefMaker*, LoadStream&))
		: PropertyFieldDescriptor(definingClass, identifier, flags, _propertyStorageReadFunc, _propertyStorageWriteFunc,
				_propertyStorageSaveFunc, _propertyStorageLoadFunc) {}

	/// Constructor	for a property field that stores a single reference to a RefTarget.
	NativePropertyFieldDescriptor(const NativeOvitoObjectType* definingClass, const OvitoObjectType* targetClass, const char* identifier, PropertyFieldFlags flags, SingleReferenceFieldBase& (*_storageAccessFunc)(RefMaker*))
		: PropertyFieldDescriptor(definingClass, targetClass, identifier, flags, _storageAccessFunc) {}

	/// Constructor	for a property field that stores a vector of references to RefTarget objects.
	NativePropertyFieldDescriptor(const NativeOvitoObjectType* definingClass, const OvitoObjectType* targetClass, const char* identifier, PropertyFieldFlags flags, VectorReferenceFieldBase& (*_storageAccessFunc)(RefMaker*))
		: PropertyFieldDescriptor(definingClass, targetClass, identifier, flags, _storageAccessFunc) {}

public:

	// Internal helper class that is used to specify the units for a controller
	// property field. Do not use this class directly but use the
	// SET_PROPERTY_FIELD_UNITS macro instead.
	struct PropertyFieldUnitsSetter : public NumericalParameterDescriptor {
		PropertyFieldUnitsSetter(NativePropertyFieldDescriptor& propfield, const QMetaObject* parameterUnitType, FloatType minValue = FLOATTYPE_MIN, FloatType maxValue = FLOATTYPE_MAX) {
			OVITO_ASSERT(propfield._parameterInfo == nullptr);
			propfield._parameterInfo = this;
			this->unitType = parameterUnitType;
			this->minValue = minValue;
			this->maxValue = maxValue;
		}
	};

	// Internal helper class that is used to specify the label text for a
	// property field. Do not use this class directly but use the
	// SET_PROPERTY_FIELD_LABEL macro instead.
	struct PropertyFieldDisplayNameSetter {
		PropertyFieldDisplayNameSetter(NativePropertyFieldDescriptor& propfield, const QString& label) {
			OVITO_ASSERT(propfield._displayName.isEmpty());
			propfield._displayName = label;
		}
	};

	// Internal helper class that is used to set the reference event type to generate
	// for a property field every time its value changes. Do not use this class directly but use the
	// SET_PROPERTY_FIELD_CHANGE_EVENT macro instead.
	struct PropertyFieldChangeEventSetter {
		PropertyFieldChangeEventSetter(NativePropertyFieldDescriptor& propfield, ReferenceEvent::Type eventType) {
			OVITO_ASSERT(propfield._extraChangeEventType == 0);
			propfield._extraChangeEventType = eventType;
		}
	};
};

/*** Macros to define reference and property fields in RefMaker-derived classes: ***/

/// This macros returns a reference to the PropertyFieldDescriptor of a 
/// named reference or property field.
#define PROPERTY_FIELD(RefMakerClassPlusStorageFieldName) \
		RefMakerClassPlusStorageFieldName##__propdescr

#define DECLARE_PROPERTY_FIELD_DESCRIPTOR(name) \
	public: \
		static Ovito::NativePropertyFieldDescriptor name##__propdescr; \
	private:

/// Adds a reference field to a class definition.
/// The first parameter specifies the RefTarget derived class of the referenced object.
/// The second parameter determines the name of the reference field. It must be unique within the current class.
#define DECLARE_REFERENCE_FIELD(type, name) \
	public: \
		Ovito::ReferenceField<type> _##name; \
		type* name() const { return _##name; } \
		DECLARE_PROPERTY_FIELD_DESCRIPTOR(name) \
	private:

/// Adds a settable reference field to a class definition.
/// The first parameter specifies the RefTarget derived class of the referenced object.
/// The second parameter determines the name of the reference field. It must be unique within the current class.
/// The third parameter is the name of the setter method to be created for this reference field.
#define DECLARE_MODIFIABLE_REFERENCE_FIELD(type, name, setterName) \
	DECLARE_REFERENCE_FIELD(type, name) \
	public: \
		void setterName(type* obj) { _##name = obj; } \
	private:

#define DEFINE_FLAGS_REFERENCE_FIELD(RefMakerClass, name, UniqueFieldIdentifier, TargetClass, Flags) \
	static Ovito::SingleReferenceFieldBase& RefMakerClass##name##__access_reffield(RefMaker* obj) { \
		return static_cast<RefMakerClass*>(obj)->_##name; \
	} \
	Ovito::NativePropertyFieldDescriptor RefMakerClass::name##__propdescr( \
		&RefMakerClass::OOType, &TargetClass::OOType, \
		UniqueFieldIdentifier, Flags, &RefMakerClass##name##__access_reffield \
	);

#define DEFINE_REFERENCE_FIELD(RefMakerClass, name, UniqueFieldIdentifier, TargetClass)	\
	DEFINE_FLAGS_REFERENCE_FIELD(RefMakerClass, name, UniqueFieldIdentifier, TargetClass, PROPERTY_FIELD_NO_FLAGS)

/// Adds a vector reference field to a class definition.
/// The first parameter specifies the RefTarget-derived class of the objects stored in the vector reference field.
/// The second parameter determines the name of the vector reference field. It must be unique within the current class.
#define DECLARE_VECTOR_REFERENCE_FIELD(type, name) \
	public: \
		Ovito::VectorReferenceField<type> _##name; \
		const QVector<type*>& name() const { return _##name.targets(); } \
		DECLARE_PROPERTY_FIELD_DESCRIPTOR(name) \
	private:

/// Adds a vector reference field to a class definition that is settable.
/// The first parameter specifies the RefTarget-derived class of the objects stored in the vector reference field.
/// The second parameter determines the name of the vector reference field. It must be unique within the current class.
/// The third parameter is the name of the setter method to be created for this reference field.
#define DECLARE_MODIFIABLE_VECTOR_REFERENCE_FIELD(type, name, setterName) \
	DECLARE_VECTOR_REFERENCE_FIELD(type, name) \
	public: \
		void setterName(const QVector<type*>& lst) { _##name = lst; } \
	private:

#define DEFINE_FLAGS_VECTOR_REFERENCE_FIELD(RefMakerClass, name, UniqueFieldIdentifier, TargetClass, Flags)			\
	static Ovito::VectorReferenceFieldBase& RefMakerClass##name##__access_reffield(RefMaker* obj) {			\
		return static_cast<RefMakerClass*>(obj)->_##name;													\
	}																												\
	Ovito::NativePropertyFieldDescriptor RefMakerClass::name##__propdescr(			\
		&RefMakerClass::OOType, &TargetClass::OOType, 																\
		UniqueFieldIdentifier, Flags | PROPERTY_FIELD_VECTOR, &RefMakerClass##name##__access_reffield 	\
	);

#define DEFINE_VECTOR_REFERENCE_FIELD(RefMakerClass, name, UniqueFieldIdentifier, TargetClass)	\
	DEFINE_FLAGS_VECTOR_REFERENCE_FIELD(RefMakerClass, name, UniqueFieldIdentifier, TargetClass, PROPERTY_FIELD_VECTOR)

/// This macro must be called for every reference or property field from the constructor
/// of the RefMaker derived class that owns the field. 
#define INIT_PROPERTY_FIELD(name) \
	_##name.init(this, &PROPERTY_FIELD(name));

/// Assigns a unit class to an animation controller reference or numeric property field.
/// The unit class will automatically be assigned to the numeric input field for this parameter in the user interface. 
#define SET_PROPERTY_FIELD_UNITS(RefMakerClass, name, ParameterUnitClass)								\
	static Ovito::NativePropertyFieldDescriptor::PropertyFieldUnitsSetter __unitsSetter##RefMakerClass##name(PROPERTY_FIELD(RefMakerClass::name), &ParameterUnitClass::staticMetaObject);

/// Assigns a unit class and a minimum value limit to an animation controller reference or numeric property field.
/// The unit class and the value limit will automatically be assigned to the numeric input field for this parameter in the user interface. 
#define SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(RefMakerClass, name, ParameterUnitClass, minValue)	\
	static Ovito::NativePropertyFieldDescriptor::PropertyFieldUnitsSetter __unitsSetter##RefMakerClass##name(PROPERTY_FIELD(RefMakerClass::name), &ParameterUnitClass::staticMetaObject, minValue);

/// Assigns a unit class and a minimum and maximum value limit to an animation controller reference or numeric property field.
/// The unit class and the value limits will automatically be assigned to the numeric input field for this parameter in the user interface. 
#define SET_PROPERTY_FIELD_UNITS_AND_RANGE(RefMakerClass, name, ParameterUnitClass, minValue, maxValue)	\
	static Ovito::NativePropertyFieldDescriptor::PropertyFieldUnitsSetter __unitsSetter##RefMakerClass##name(PROPERTY_FIELD(RefMakerClass::name), &ParameterUnitClass::staticMetaObject, minValue, maxValue);

/// Assigns a label string to the given reference or property field.
/// This string will be used in the user interface.
#define SET_PROPERTY_FIELD_LABEL(RefMakerClass, name, labelText)										\
	static Ovito::NativePropertyFieldDescriptor::PropertyFieldDisplayNameSetter __displayNameSetter##RefMakerClass##name(PROPERTY_FIELD(RefMakerClass::name), labelText);

/// Use this macro to let the system automatically generate an event of the
/// given type every time the given property field changes its value. 
#define SET_PROPERTY_FIELD_CHANGE_EVENT(RefMakerClass, name, eventType)										\
	static Ovito::NativePropertyFieldDescriptor::PropertyFieldChangeEventSetter __changeEventSetter##RefMakerClass##name(PROPERTY_FIELD(RefMakerClass::name), eventType);

/// Adds a property field to a class definition.
/// The first parameter specifies the data type of the property field.
/// The second parameter determines the name of the property field. It must be unique within the current class.
#define DECLARE_PROPERTY_FIELD(type, name)																	\
	public: 																										\
		Ovito::PropertyField<type> _##name; \
		const type& name() const { return _##name; } \
		DECLARE_PROPERTY_FIELD_DESCRIPTOR(name) \
	private:

/// Adds a settable property field to a class definition.
/// The first parameter specifies the data type of the property field.
/// The second parameter determines the name of the property field. It must be unique within the current class.
/// The third parameter is the name of the setter method to be created for this property field.
#define DECLARE_MODIFIABLE_PROPERTY_FIELD(type, name, setterName) \
	DECLARE_PROPERTY_FIELD(type, name) \
	public: \
		void setterName(const type& value) { _##name = value; } \
	private:

#define DEFINE_FLAGS_PROPERTY_FIELD(RefMakerClass, name, UniqueFieldIdentifier, Flags)			\
	static QVariant RefMakerClass##__read_propfield_##name(RefMaker* obj) {							\
		return static_cast<const RefMakerClass*>(obj)->_##name.to_qvariant();						\
	}																										\
	static void RefMakerClass##__write_propfield_##name(RefMaker* obj, const QVariant& newValue) {		\
		static_cast<RefMakerClass*>(obj)->_##name = newValue;										\
	}																										\
	static void RefMakerClass##__save_propfield_##name(RefMaker* obj, SaveStream& stream) {			\
		static_cast<RefMakerClass*>(obj)->_##name.saveToStream(stream);							\
	}																										\
	static void RefMakerClass##__load_propfield_##name(RefMaker* obj, LoadStream& stream) {			\
		static_cast<RefMakerClass*>(obj)->_##name.loadFromStream(stream);							\
	}																										\
	Ovito::NativePropertyFieldDescriptor RefMakerClass::name##__propdescr(				\
		&RefMakerClass::OOType, \
		UniqueFieldIdentifier, Flags, \
		&RefMakerClass##__read_propfield_##name, \
		&RefMakerClass##__write_propfield_##name, \
		&RefMakerClass##__save_propfield_##name, \
		&RefMakerClass##__load_propfield_##name \
	);

#define DEFINE_PROPERTY_FIELD(RefMakerClass, name, UniqueFieldIdentifier)						\
	DEFINE_FLAGS_PROPERTY_FIELD(RefMakerClass, name, UniqueFieldIdentifier, PROPERTY_FIELD_NO_FLAGS)

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace

// The RefTarget header must be present because
// we are using OORef<RefTarget> here.
#include "RefMaker.h"
#include "RefTarget.h"


