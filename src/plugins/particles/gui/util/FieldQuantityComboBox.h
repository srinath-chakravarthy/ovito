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


#include <plugins/particles/gui/ParticlesGui.h>
#include <plugins/particles/objects/FieldQuantityObject.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Util)

/**
 * \brief Widget that allows the user to select a field quantity from a list.
 */
class OVITO_PARTICLES_GUI_EXPORT FieldQuantityComboBox : public QComboBox
{
	Q_OBJECT

public:

	/// \brief Default constructor.
	/// \param parent The parent widget of the combo box.
	FieldQuantityComboBox(QWidget* parent = nullptr) : QComboBox(parent) {}

	/// \brief Adds a quantity to the end of the list.
	/// \param quantity The quantity to be added.
	void addItem(const FieldQuantityReference& quantity, const QString& label = QString()) {
		QComboBox::addItem(label.isEmpty() ? quantity.name() : label, QVariant::fromValue(quantity));
	}

	/// \brief Adds a quantity to the end of the list.
	/// \param quantity The quantity to be added.
	void addItem(FieldQuantityObject* quantity, int vectorComponent = -1) {
		QString label = quantity->nameWithComponent(vectorComponent);
		FieldQuantityReference quantityRef(quantity, vectorComponent);
		QComboBox::addItem(label, QVariant::fromValue(quantityRef));
	}

	/// \brief Adds multiple quantities to the combo box.
	/// \param list The list of quantities to be added.
	void addItems(const QVector<FieldQuantityObject*>& list) {
		for(FieldQuantityObject* q : list)
			addItem(q);
	}

	/// \brief Returns the field quantity that is currently selected in the
	///        combo box.
	/// \return The selected item. The returned reference can be null if
	///         no item is currently selected.
	FieldQuantityReference currentFieldQuantity() const;

	/// \brief Sets the selection of the combo box to the given quantity.
	/// \param quantity The field quantity to be selected.
	void setCurrentFieldQuantity(const FieldQuantityReference& quantity);

	/// \brief Returns the list index of the given quantity, or -1 if not found.
	int quantityIndex(const FieldQuantityReference& quantity) const {
		for(int index = 0; index < count(); index++) {
			if(quantity == itemData(index).value<FieldQuantityReference>())
				return index;
		}
		return -1;
	}

	/// \brief Returns the quantity at the given index.
	FieldQuantityReference quantity(int index) const {
		return itemData(index).value<FieldQuantityReference>();
	}

protected:

	/// Is called when the widget loses the input focus.
	virtual void focusOutEvent(QFocusEvent* event) override;

};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


