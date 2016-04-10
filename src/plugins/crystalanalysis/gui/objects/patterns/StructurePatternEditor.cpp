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

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/crystalanalysis/objects/patterns/StructurePattern.h>
#include <gui/properties/ColorParameterUI.h>
#include <gui/properties/StringParameterUI.h>
#include <gui/properties/RefTargetListParameterUI.h>
#include "StructurePatternEditor.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_OBJECT(CrystalAnalysisGui, StructurePatternEditor, PropertiesEditor);
SET_OVITO_OBJECT_EDITOR(StructurePattern, StructurePatternEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void StructurePatternEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Structure type"), rolloutParams);

    // Create the rollout contents.
	QVBoxLayout* layout1 = new QVBoxLayout(rollout);
	layout1->setContentsMargins(4,4,4,4);
	layout1->setSpacing(4);

	// Derive a custom class from the list parameter UI to
	// give the items a color.
	class CustomRefTargetListParameterUI : public RefTargetListParameterUI {
	public:
		CustomRefTargetListParameterUI(PropertiesEditor* parentEditor, const PropertyFieldDescriptor& refField)
			: RefTargetListParameterUI(parentEditor, refField, RolloutInsertionParameters(), nullptr) {}
	protected:

		virtual QVariant getItemData(RefTarget* target, const QModelIndex& index, int role) override {
			if(target) {
				if(role == Qt::DisplayRole) {
					if(index.column() == 1)
						return target->objectTitle();
				}
				else if(role == Qt::DecorationRole) {
					if(index.column() == 0)
						return (QColor)static_object_cast<BurgersVectorFamily>(target)->color();
				}
			}
			return QVariant();
		}

		/// Returns the number of columns for the table view.
		virtual int tableColumnCount() override { return 3; }

		/// Returns the header data under the given role for the given RefTarget.
		/// This method is part of the data model used by the list widget and can be overridden
		/// by sub-classes.
		virtual QVariant getHorizontalHeaderData(int index, int role) override {
			if(index == 0)
				return tr("Color");
			else
				return tr("Name");
		}

		/// Do not open sub-editor for selected item.
		virtual void openSubEditor() override {}
	};

	layout1->addWidget(new QLabel(tr("Burgers vector families:")));
	familiesListUI = new CustomRefTargetListParameterUI(this, PROPERTY_FIELD(StructurePattern::_burgersVectorFamilies));
	layout1->addWidget(familiesListUI->tableWidget(200));
	familiesListUI->tableWidget()->setAutoScroll(false);
	connect(familiesListUI->tableWidget(), &QTableWidget::doubleClicked, this, &StructurePatternEditor::onDoubleClickBurgersFamily);

	QLabel* label = new QLabel(tr("<p style=\"font-size: small;\">Double-click to change colors.</p>"));
	label->setWordWrap(true);
	layout1->addWidget(label);
}

/******************************************************************************
* Is called when the user has double-clicked on one of the entries in the
* list widget.
******************************************************************************/
void StructurePatternEditor::onDoubleClickBurgersFamily(const QModelIndex& index)
{
	// Let the user select a color for the Burgers vector family.
	BurgersVectorFamily* family = static_object_cast<BurgersVectorFamily>(familiesListUI->selectedObject());
	if(!family) return;

	QColor oldColor = Color(family->color());
	QColor newColor = QColorDialog::getColor(oldColor, container());
	if(!newColor.isValid() || newColor == oldColor)
		return;

	undoableTransaction(tr("Change Burgers vector family color"), [family, &newColor]() {
		family->setColor(Color(newColor));
	});
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
