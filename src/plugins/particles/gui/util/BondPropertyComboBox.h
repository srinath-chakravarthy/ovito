///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
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


#include <plugins/particles/gui/ParticlesGui.h>
#include <plugins/particles/objects/BondPropertyObject.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Util)

/**
 * \brief Widget that allows the user to select a bond property from a list.
 */
class OVITO_PARTICLES_GUI_EXPORT BondPropertyComboBox : public QComboBox
{
	Q_OBJECT

public:

	/// \brief Default constructor.
	/// \param parent The parent widget of the combo box.
	BondPropertyComboBox(QWidget* parent = nullptr) : QComboBox(parent) {}

	/// \brief Adds a property to the end of the list.
	/// \param property The property to be added.
	void addItem(const BondPropertyReference& property, const QString& label = QString()) {
		QComboBox::addItem(label.isEmpty() ? property.name() : label, QVariant::fromValue(property));
	}

	/// \brief Adds a property to the end of the list.
	/// \param property The property to be added.
	void addItem(BondPropertyObject* property, int vectorComponent = -1) {
		QString label = property->nameWithComponent(vectorComponent);
		BondPropertyReference propRef(property, vectorComponent);
		QComboBox::addItem(label, QVariant::fromValue(propRef));
	}

	/// \brief Adds multiple bond properties to the combo box.
	/// \param list The list of properties to be added.
	void addItems(const QVector<BondPropertyObject*>& list) {
		for(BondPropertyObject* p : list)
			addItem(p);
	}

	/// \brief Returns the bond property that is currently selected in the
	///        combo box.
	/// \return The selected item. The returned reference can be null if
	///         no item is currently selected.
	BondPropertyReference currentProperty() const;

	/// \brief Sets the selection of the combo box to the given property.
	/// \param property The bond property to be selected.
	void setCurrentProperty(const BondPropertyReference& property);

	/// \brief Returns the list index of the given property, or -1 if not found.
	int propertyIndex(const BondPropertyReference& property) const {
		for(int index = 0; index < count(); index++) {
			if(property == itemData(index).value<BondPropertyReference>())
				return index;
		}
		return -1;
	}

	/// \brief Returns the property at the given index.
	BondPropertyReference property(int index) const {
		return itemData(index).value<BondPropertyReference>();
	}

protected:

	/// Is called when the widget loses the input focus.
	virtual void focusOutEvent(QFocusEvent* event) override;

};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


