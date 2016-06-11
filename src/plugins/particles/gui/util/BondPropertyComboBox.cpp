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

#include <plugins/particles/gui/ParticlesGui.h>
#include "BondPropertyComboBox.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Util)

/******************************************************************************
* Returns the property that is currently selected in the combo box.
******************************************************************************/
BondPropertyReference BondPropertyComboBox::currentProperty() const
{
	if(!isEditable()) {
		int index = currentIndex();
		if(index < 0)
			return BondPropertyReference();
		return itemData(index).value<BondPropertyReference>();
	}
	else {
		QString name = currentText().simplified();
		if(!name.isEmpty()) {
			QMap<QString, BondProperty::Type> stdplist = BondProperty::standardPropertyList();
			auto entry = stdplist.find(name);
			if(entry != stdplist.end())
				return BondPropertyReference(entry.value());
			return BondPropertyReference(name);
		}
		return BondPropertyReference();
	}
}

/******************************************************************************
* Sets the selection of the combo box to the given property.
******************************************************************************/
void BondPropertyComboBox::setCurrentProperty(const BondPropertyReference& property)
{
	for(int index = 0; index < count(); index++) {
		if(property == itemData(index).value<BondPropertyReference>()) {
			setCurrentIndex(index);
			return;
		}
	}
	if(isEditable() && !property.isNull()) {
		setCurrentText(property.name());
	}
	else {
		setCurrentIndex(-1);
	}
}

/******************************************************************************
* Is called when the widget loses the input focus.
******************************************************************************/
void BondPropertyComboBox::focusOutEvent(QFocusEvent* event)
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
