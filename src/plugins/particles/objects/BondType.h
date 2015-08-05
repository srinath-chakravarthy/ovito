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

#ifndef __OVITO_BOND_TYPE_H
#define __OVITO_BOND_TYPE_H

#include <plugins/particles/Particles.h>
#include <core/reference/RefTarget.h>
#include <core/gui/properties/PropertiesEditor.h>

namespace Ovito { namespace Particles {

/**
 * \brief Stores the properties of a bond type, e.g. name, color, and radius.
 *
 * \ingroup particles_objects
 */
class OVITO_PARTICLES_EXPORT BondType : public RefTarget
{
public:

	/// \brief Constructs a new bond type.
	Q_INVOKABLE BondType(DataSet* dataset);

	/// \brief Returns the identifier of the bond type.
	/// \return The type identifier.
	int id() const { return _id; }

	/// \brief Sets the identifier of the bond type.
	/// \param identifier The new identifier.
	/// \undoable
	void setId(int identifier) { _id = identifier; }

	/// \brief Gets the types's display name.
	/// \return The human-readable name of the bond type.
	/// \sa setName()
	const QString& name() const { return _name; }

	/// \brief Sets the types's display name.
	/// \param name The new human-readable name for this bond type.
	/// \undoable
	/// \sa name()
	void setName(const QString& name) { _name = name; }

	/// \brief Returns the display color that is assigned to the bonds of this type.
	/// \return The color used for bonds of this type.
	/// \sa setColor()
	Color color() const { return _color; }

	/// \brief Sets the display color of this bond type.
	/// \param color The new color to be used to display bonds of this type.
	/// \undoable
	void setColor(const Color& color) { _color = color; }

	/// \brief Returns the radius of the bond type.
	/// \return The radius in world units.
	FloatType radius() const { return _radius; }

	/// \brief Sets the radius of the bond type.
	/// \param newRadius The radius in world units to be used to display this kind of bond.
	/// \undoable
	void setRadius(FloatType newRadius) { _radius = newRadius; }

	// From RefTarget class:

	/// Returns the title of this object.
	virtual QString objectTitle() override { return name(); }

protected:

	/// Stores the identifier of the bond type.
	PropertyField<int> _id;

	/// The name of this bond type.
	PropertyField<QString, QString, ReferenceEvent::TitleChanged> _name;

	/// Stores the color of the bond type.
	PropertyField<Color, QColor> _color;

	/// Stores the radius of the bond type.
	PropertyField<FloatType> _radius;

private:

	Q_OBJECT
	OVITO_OBJECT

	DECLARE_PROPERTY_FIELD(_id);
	DECLARE_PROPERTY_FIELD(_name);
	DECLARE_PROPERTY_FIELD(_color);
	DECLARE_PROPERTY_FIELD(_radius);
};

OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * \brief A properties editor for the BondType class.
 */
class BondTypeEditor : public PropertiesEditor
{
public:

	/// Default constructor.
	Q_INVOKABLE BondTypeEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

private:

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE

}	// End of namespace
}	// End of namespace

#endif // __OVITO_BOND_TYPE_H
