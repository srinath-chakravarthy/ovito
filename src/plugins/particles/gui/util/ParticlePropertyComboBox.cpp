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

#include <plugins/particles/gui/ParticlesGui.h>
#include "ParticlePropertyComboBox.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Util)

/******************************************************************************
* Returns the particle property that is currently selected in the combo box.
******************************************************************************/
ParticlePropertyReference ParticlePropertyComboBox::currentProperty() const
{
	if(!isEditable()) {
		int index = currentIndex();
		if(index < 0)
			return ParticlePropertyReference();
		return itemData(index).value<ParticlePropertyReference>();
	}
	else {
		QString name = currentText().simplified();
		if(!name.isEmpty()) {
			QMap<QString, ParticleProperty::Type> stdplist = ParticleProperty::standardPropertyList();
			auto entry = stdplist.find(name);
			if(entry != stdplist.end())
				return ParticlePropertyReference(entry.value());
			return ParticlePropertyReference(name);
		}
		return ParticlePropertyReference();
	}
}

/******************************************************************************
* Sets the selection of the combo box to the given particle property.
******************************************************************************/
void ParticlePropertyComboBox::setCurrentProperty(const ParticlePropertyReference& property)
{
	for(int index = 0; index < count(); index++) {
		if(property == itemData(index).value<ParticlePropertyReference>()) {
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
void ParticlePropertyComboBox::focusOutEvent(QFocusEvent* event)
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
