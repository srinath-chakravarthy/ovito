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
#include <core/scene/objects/DataObjectWithSharedStorage.h>
#include <core/scene/pipeline/PipelineFlowState.h>
#include <plugins/particles/data/FieldQuantity.h>

namespace Ovito { namespace Particles {

/**
 * \brief Stores a field quantity.
 */
class OVITO_PARTICLES_EXPORT FieldQuantityObject : public DataObjectWithSharedStorage<FieldQuantity>
{
public:

	/// \brief Creates a field quantity object.
	Q_INVOKABLE FieldQuantityObject(DataSet* dataset, FieldQuantity* storage = nullptr);

	/// \brief Factory function that creates a field quantity object.
	/// \param shape The number of grid samples along each dimension.
	/// \param dataType Specifies the data type (integer, floating-point, ...) of the quantity elements.
	///                 The data type is specified as an identifier according to the Qt metatype system.
	/// \param componentCount The number of components per field value of type \a dataType.
	/// \param stride The number of bytes per field value (pass 0 to use the smallest possible stride).
	/// \param name The name assigned to the field quantity.
	/// \param initializeMemory Controls whether the newly allocated memory is initialized with zeros.
	static OORef<FieldQuantityObject> createFieldQuantity(DataSet* dataset, std::vector<size_t> shape, int dataType, size_t componentCount, size_t stride, const QString& name, bool initializeMemory);

	/// \brief Factory function that creates a property object based on an existing storage.
	static OORef<FieldQuantityObject> createFromStorage(DataSet* dataset, FieldQuantity* storage);

	/// \brief Gets the quantity's name.
	/// \return The name of quantity.
	const QString& name() const { return storage()->name(); }

	/// \brief Sets the quantity's name.
	/// \param name The new name string.
	/// \undoable
	void setName(const QString& name);

	/// \brief Returns the data type of the quantity.
	/// \return The identifier of the data type used for the elements stored in
	///         this property storage according to the Qt meta type system.
	int dataType() const { return storage()->dataType(); }

	/// \brief Returns the number of bytes per value.
	/// \return Number of bytes used to store a single value of the data type
	///         specified by dataType().
	size_t dataTypeSize() const { return storage()->dataTypeSize(); }

	/// \brief Returns the number of bytes used per field element.
	size_t stride() const { return storage()->stride(); }

	/// \brief Returns the number of array elements per field value.
	/// \return The number of data values stored per particle in this storage object.
	size_t componentCount() const { return storage()->componentCount(); }

	/// \brief Returns the human-readable names for the components stored per field value.
	/// \return The names of the vector components if this channel contains more than one value per field point.
	const QStringList& componentNames() const { return storage()->componentNames(); }

	/// \brief Returns the display name of the property including the name of the given
	///        vector component.
	QString nameWithComponent(int vectorComponent) const {
		if(componentCount() <= 1 || vectorComponent < 0)
			return name();
		else if(vectorComponent < componentNames().size())
			return QString("%1.%2").arg(name()).arg(componentNames()[vectorComponent]);
		else
			return QString("%1.%2").arg(name()).arg(vectorComponent + 1);
	}

	//////////////////////////////// from RefTarget //////////////////////////////

	/// Returns whether this object, when returned as an editable sub-object by another object,
	/// should be displayed in the modification stack.
	virtual bool isSubObjectEditable() const override { return false; }

	/// \brief Returns the title of this object.
	virtual QString objectTitle() override {
		return name();
	}

	/// This helper method returns a specific field quantity (if present) from the given pipeline state.
	static FieldQuantityObject* findInState(const PipelineFlowState& state, const QString& name);

protected:

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream) override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

private:

	Q_OBJECT
	OVITO_OBJECT
};

/**
 * \brief A reference to a field quantity.
 *
 * This class is a reference to a field quantity. For instance, it is used by modifiers
 * to store the input quantity selected by the user, which they will act upon. When the modifier
 * is evaluated, the reference is resolved by looking up the corresponding FieldQuantityObject
 * from the current input dataset, which contains the actual field data.
 */
class OVITO_PARTICLES_EXPORT FieldQuantityReference
{
public:

	/// \brief Default constructor. Creates a null reference.
	FieldQuantityReference() : _vectorComponent(-1) {}
	/// \brief Constructs a reference to a field quantity.
	FieldQuantityReference(const QString& name, int vectorComponent = -1) : _name(name), _vectorComponent(vectorComponent) {}
	/// \brief Constructs a reference based on an existing FieldQuantity.
	FieldQuantityReference(FieldQuantity* q, int vectorComponent = -1) : _name(q->name()), _vectorComponent(vectorComponent) {}
	/// \brief Constructs a reference based on an existing FieldQuantityObject.
	FieldQuantityReference(FieldQuantityObject* q, int vectorComponent = -1) : _name(q->name()), _vectorComponent(vectorComponent) {}

	/// \brief Gets the human-readable name of the referenced field quantity.
	/// \return The quantity name.
	const QString& name() const { return _name; }

	/// Returns the selected component index.
	int vectorComponent() const { return _vectorComponent; }

	/// Selects a component index if the property is a vector quantity.
	void setVectorComponent(int index) { _vectorComponent = index; }

	/// \brief Compares two references for equality.
	bool operator==(const FieldQuantityReference& other) const {
		if(vectorComponent() != other.vectorComponent()) return false;
		return name() == other.name();
	}

	/// \brief Compares two references for inequality.
	bool operator!=(const FieldQuantityReference& other) const { return !(*this == other); }

	/// \brief Returns true if this reference does not point to any field quantity.
	/// \return true if this is a default constructed FieldQuantityReference.
	bool isNull() const { return name().isEmpty(); }

	/// \brief This method retrieves the actual quantity from a pipeline state.
	/// \return The actual quantity after resolving this reference; or NULL if the quantity does not exist.
	FieldQuantityObject* findInState(const PipelineFlowState& state) const;

	/// \brief Returns the display name of the referenced property including the optional vector component.
	QString nameWithComponent() const {
		if(vectorComponent() < 0)
			return name();
		else
			return QString("%1.%2").arg(name()).arg(vectorComponent() + 1);
	}

private:

	/// The human-readable name of the quantity.
	QString _name;

	/// The zero-based component index if its a vectorial quantity (or zero if scalar).
	int _vectorComponent;

	friend SaveStream& operator<<(SaveStream& stream, const FieldQuantityReference& r);
	friend LoadStream& operator>>(LoadStream& stream, FieldQuantityReference& r);
};

/// Writes a FieldQuantityReference to an output stream.
/// \relates FieldQuantityReference
inline SaveStream& operator<<(SaveStream& stream, const FieldQuantityReference& r)
{
	stream << r.name();
	stream << r.vectorComponent();
	return stream;
}

/// Reads a FieldQuantityReference from an input stream.
/// \relates FieldQuantityReference
inline LoadStream& operator>>(LoadStream& stream, FieldQuantityReference& r)
{
	stream >> r._name;
	stream >> r._vectorComponent;
	return stream;
}

}	// End of namespace
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::Particles::FieldQuantityReference);


