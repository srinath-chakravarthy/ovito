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
#include "PropertyBase.h"

namespace Ovito { namespace Particles {

/**
 * \brief Memory storage for a per-bond property.
 */
class OVITO_PARTICLES_EXPORT BondProperty : public PropertyBase
{
public:

	/// \brief The standard types of properties.
	enum Type {
		UserProperty = 0,	//< This is reserved for user-defined properties.
		BondTypeProperty,
		SelectionProperty,
		ColorProperty,
		LengthProperty
	};
	Q_ENUMS(Type);

public:

	/// \brief Default constructor that creates an empty, uninitialized storage.
	BondProperty() : PropertyBase(), _type(UserProperty) {}

	/// \brief Constructor that creates a standard property storage.
	/// \param bondsCount The number of bonds.
	/// \param type Specifies which standard property should be created.
	///             This must not be BondProperty::Type::UserProperty.
	/// \param componentCount The component count if this type of property
	///                       has a variable component count; otherwise 0 to use the
	///                       default number of components.
	/// \param initializeMemory Controls whether the newly allocated memory is initialized with zeros.
	///
	/// Data type, component count and property name are automatically set by this
	/// constructor.
	BondProperty(size_t bondsCount, Type type, size_t componentCount, bool initializeMemory);

	/// \brief Constructor that creates a user-defined property storage.
	/// \param bondsCount The number of bonds.
	/// \param dataType Specifies the data type (integer, floating-point, ...) of the per-bonds elements.
	///                 The data type is specified as identifier according to the Qt metatype system.
	/// \param componentCount The number of components per bond of type \a dataType.
	/// \param stride The number of bytes per bond (pass 0 to use the smallest possible stride).
	/// \param name The name assigned to the property.
	/// \param initializeMemory Controls whether the newly allocated memory is initialized with zeros.
	BondProperty(size_t bondsCount, int dataType, size_t componentCount, size_t stride, const QString& name, bool initializeMemory);

	/// \brief Copy constructor.
	BondProperty(const BondProperty& other);

	/// \brief Returns the type of this property.
	Type type() const { return _type; }

	/// \brief Changes the type of this property.
	/// \note The type may only be changed if the new property has the same
	///       data type and component count as the old one.
	void setType(Type newType) {
		if(newType != UserProperty) {
			OVITO_ASSERT(dataType() == standardPropertyDataType(newType));
			OVITO_ASSERT(componentCount() == standardPropertyComponentCount(newType));
			setName(standardPropertyName(newType));
			_componentNames = standardPropertyComponentNames(newType);
		}
		_type = newType;
	}

	/// Writes the object to an output stream.
	void saveToStream(SaveStream& stream, bool onlyMetadata = false) const;

	/// Reads the object from an input stream.
	void loadFromStream(LoadStream& stream);

public:

	/// \brief Returns the name of a standard property.
	/// \param which Any of the standard property types except Type::UserProperty.
	/// \return The name string used for the given standard property by default.
	static QString standardPropertyName(Type which);

	/// \brief Returns the display title used for a standard property object.
	/// \param which Any of the standard property types except Type::UserProperty.
	/// \return The title string used for a property object.
	static QString standardPropertyTitle(Type which);

	/// Returns the data type used by the given standard property type.
	static int standardPropertyDataType(Type which);

	/// \brief Returns the number of vector components per bonds used by the given standard property.
	/// \param which The standard property type for which the number of components should be returned.
	/// \return The number of fixed components or 0 if this kind of property has a variable number of components.
	static size_t standardPropertyComponentCount(Type which);

	/// \brief Returns the list of component names for the given standard property type.
	/// \param which The standard property type for which the component names should be returned.
	/// \param componentCount Optional number of actual components if the standard property has a variable number of components.
	static QStringList standardPropertyComponentNames(Type which, size_t componentCount = 0);

	/// \brief Returns a list with the names and types of all defined standard property types.
	static QMap<QString, Type> standardPropertyList();

protected:

	/// The type of this property.
	Type _type;
};

}	// End of namespace
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::Particles::BondProperty::Type);
Q_DECLARE_TYPEINFO(Ovito::Particles::BondProperty::Type, Q_PRIMITIVE_TYPE);


