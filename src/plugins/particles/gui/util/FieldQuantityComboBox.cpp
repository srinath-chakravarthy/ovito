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

#include <plugins/particles/gui/ParticlesGui.h>
#include "FieldQuantityComboBox.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Util)

/******************************************************************************
* Returns the quantity that is currently selected in the combo box.
******************************************************************************/
FieldQuantityReference FieldQuantityComboBox::currentFieldQuantity() const
{
	if(!isEditable()) {
		int index = currentIndex();
		if(index < 0)
			return FieldQuantityReference();
		return itemData(index).value<FieldQuantityReference>();
	}
	else {
		QString name = currentText().simplified();
		if(!name.isEmpty()) {
			return FieldQuantityReference(name);
		}
		return FieldQuantityReference();
	}
}

/******************************************************************************
* Sets the selection of the combo box to the given quantity.
******************************************************************************/
void FieldQuantityComboBox::setCurrentFieldQuantity(const FieldQuantityReference& quantity)
{
	for(int index = 0; index < count(); index++) {
		if(quantity == itemData(index).value<FieldQuantityReference>()) {
			setCurrentIndex(index);
			return;
		}
	}
	if(isEditable() && !quantity.isNull()) {
		setCurrentText(quantity.name());
	}
	else {
		setCurrentIndex(-1);
	}
}

/******************************************************************************
* Is called when the widget loses the input focus.
******************************************************************************/
void FieldQuantityComboBox::focusOutEvent(QFocusEvent* event)
{
	if(isEditable()) {
		int index = findText(currentText());
		if(index == -1 && currentText().isEmpty() == false) {
			addItem(currentText());
			index = count() - 1;
		}
		setCurrentIndex(index);
		Q_EMIT activated(index);
		Q_EMIT activated(currentText());
	}
	QComboBox::focusOutEvent(event);
}


OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
