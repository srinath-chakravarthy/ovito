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
#include <plugins/particles/modifier/analysis/StructureIdentificationModifier.h>
#include "StructureListParameterUI.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/******************************************************************************
* Constructor.
******************************************************************************/
StructureListParameterUI::StructureListParameterUI(PropertiesEditor* parentEditor, bool showCheckBoxes)
	: RefTargetListParameterUI(parentEditor, PROPERTY_FIELD(StructureIdentificationModifier::structureTypes)),
	  _showCheckBoxes(showCheckBoxes)
{
	connect(tableWidget(220), &QTableWidget::doubleClicked, this, &StructureListParameterUI::onDoubleClickStructureType);
	tableWidget()->setAutoScroll(false);
}

/******************************************************************************
* Returns a data item from the list data model.
******************************************************************************/
QVariant StructureListParameterUI::getItemData(RefTarget* target, const QModelIndex& index, int role)
{
	ParticleType* stype = dynamic_object_cast<ParticleType>(target);
	StructureIdentificationModifier* modifier = dynamic_object_cast<StructureIdentificationModifier>(editor()->editObject());

	if(stype && modifier) {
		if(role == Qt::DisplayRole) {
			if(index.column() == 1)
				return stype->name();
			else if(index.column() == 2) {
				if(stype->id() >= 0 && stype->id() < modifier->structureCounts().size())
					return modifier->structureCounts()[stype->id()];
				else
					return QString();
			}
			else if(index.column() == 3) {
				if(stype->id() >= 0 && stype->id() < modifier->structureCounts().size()) {
					size_t totalCount = 0;
					for(int c : modifier->structureCounts())
						totalCount += c;
					return QString("%1%").arg((double)modifier->structureCounts()[stype->id()] * 100.0 / std::max((size_t)1, totalCount), 0, 'f', 1);
				}
				else
					return QString();
			}
			else if(index.column() == 4)
				return stype->id();
		}
		else if(role == Qt::DecorationRole) {
			if(index.column() == 0)
				return (QColor)stype->color();
		}
		else if(role == Qt::CheckStateRole && _showCheckBoxes) {
			if(index.column() == 0)
				return stype->enabled() ? Qt::Checked : Qt::Unchecked;
		}
	}
	return QVariant();
}

/******************************************************************************
* Returns the model/view item flags for the given entry.
******************************************************************************/
Qt::ItemFlags StructureListParameterUI::getItemFlags(RefTarget* target, const QModelIndex& index)
{
	if(index.column() == 0 && _showCheckBoxes)
		return RefTargetListParameterUI::getItemFlags(target, index) | Qt::ItemIsUserCheckable;
	else
		return RefTargetListParameterUI::getItemFlags(target, index);
}

/******************************************************************************
* Sets the role data for the item at index to value.
******************************************************************************/
bool StructureListParameterUI::setItemData(RefTarget* target, const QModelIndex& index, const QVariant& value, int role)
{
	if(index.column() == 0 && role == Qt::CheckStateRole) {
		if(ParticleType* stype = static_object_cast<ParticleType>(objectAtIndex(index.row()))) {
			bool enabled = (value.toInt() == Qt::Checked);
			undoableTransaction(tr("Enable/disable structure type"), [stype, enabled]() {
				stype->setEnabled(enabled);
			});
			return true;
		}
	}

	return RefTargetListParameterUI::setItemData(target, index, value, role);
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool StructureListParameterUI::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	if(source == editObject()) {
		if(event->type() == ReferenceEvent::ObjectStatusChanged) {
			// Update the structure count and fraction columns.
			_model->updateColumns(2, 3);
		}
	}
	return RefTargetListParameterUI::referenceEvent(source, event);
}

/******************************************************************************
* Is called when the user has double-clicked on one of the structure
* types in the list widget.
******************************************************************************/
void StructureListParameterUI::onDoubleClickStructureType(const QModelIndex& index)
{
	// Let the user select a color for the structure type.
	ParticleType* stype = static_object_cast<ParticleType>(selectedObject());
	if(!stype) return;

	QColor oldColor = (QColor)stype->color();
	QColor newColor = QColorDialog::getColor(oldColor, editor()->container());
	if(!newColor.isValid() || newColor == oldColor) return;

	undoableTransaction(tr("Change structure type color"), [stype, newColor]() {
		stype->setColor(Color(newColor));
	});
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
