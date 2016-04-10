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
#include <plugins/particles/modifier/selection/SelectParticleTypeModifier.h>
#include <plugins/particles/gui/util/ParticlePropertyComboBox.h>
#include <plugins/particles/objects/ParticleTypeProperty.h>
#include "SelectParticleTypeModifierEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Selection) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(ParticlesGui, SelectParticleTypeModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(SelectParticleTypeModifier, SelectParticleTypeModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void SelectParticleTypeModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	QWidget* rollout = createRollout(tr("Select particle type"), rolloutParams, "particles.modifiers.select_particle_type.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	propertyListBox = new ParticlePropertyComboBox();
	layout->addWidget(new QLabel(tr("Property:"), rollout));
	layout->addWidget(propertyListBox);

	class MyListWidget : public QListWidget {
	public:
		MyListWidget() : QListWidget() {}
		virtual QSize sizeHint() const { return QSize(256, 192); }
	};
	particleTypesBox = new MyListWidget();
	particleTypesBox->setSelectionMode(QAbstractItemView::ExtendedSelection);
	layout->addWidget(new QLabel(tr("Types:"), rollout));
	layout->addWidget(particleTypesBox);

	// Update property list if another modifier has been loaded into the editor.
	connect(this, &SelectParticleTypeModifierEditor::contentsReplaced, this, &SelectParticleTypeModifierEditor::updatePropertyList);

	// Status label.
	layout->addSpacing(12);
	layout->addWidget(statusLabel());
}

/******************************************************************************
* Updates the contents of the combo box.
******************************************************************************/
void SelectParticleTypeModifierEditor::updatePropertyList()
{
	disconnect(propertyListBox, (void (QComboBox::*)(int))&QComboBox::activated, this, &SelectParticleTypeModifierEditor::onPropertySelected);
	propertyListBox->clear();

	SelectParticleTypeModifier* mod = static_object_cast<SelectParticleTypeModifier>(editObject());
	if(!mod) {
		propertyListBox->setEnabled(false);
	}
	else {
		propertyListBox->setEnabled(true);

		// Populate type property list based on modifier input.
		PipelineFlowState inputState = mod->getModifierInput();
		for(DataObject* o : inputState.objects()) {
			ParticleTypeProperty* ptypeProp = dynamic_object_cast<ParticleTypeProperty>(o);
			if(ptypeProp && ptypeProp->particleTypes().empty() == false && ptypeProp->componentCount() == 1) {
				propertyListBox->addItem(ptypeProp);
			}
		}

		propertyListBox->setCurrentProperty(mod->sourceProperty());
	}
	connect(propertyListBox, (void (QComboBox::*)(int))&QComboBox::activated, this, &SelectParticleTypeModifierEditor::onPropertySelected);

	updateParticleTypeList();
}

/******************************************************************************
* Updates the contents of the list box.
******************************************************************************/
void SelectParticleTypeModifierEditor::updateParticleTypeList()
{
	disconnect(particleTypesBox, &QListWidget::itemChanged, this, &SelectParticleTypeModifierEditor::onParticleTypeSelected);
	particleTypesBox->setUpdatesEnabled(false);
	particleTypesBox->clear();

	SelectParticleTypeModifier* mod = static_object_cast<SelectParticleTypeModifier>(editObject());
	if(!mod) {
		particleTypesBox->setEnabled(false);
	}
	else {
		particleTypesBox->setEnabled(true);

		// Populate atom types list based on the input type property.
		ParticleTypeProperty* inputProperty = dynamic_object_cast<ParticleTypeProperty>(mod->sourceProperty().findInState(mod->getModifierInput()));
		if(inputProperty) {
			for(ParticleType* ptype : inputProperty->particleTypes()) {
				if(!ptype) continue;
				QListWidgetItem* item = new QListWidgetItem(ptype->name(), particleTypesBox);
				item->setData(Qt::UserRole, ptype->id());
				item->setData(Qt::DecorationRole, (QColor)ptype->color());
				if(mod->selectedParticleTypes().contains(ptype->id()))
					item->setCheckState(Qt::Checked);
				else
					item->setCheckState(Qt::Unchecked);
				item->setFlags(Qt::ItemFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemNeverHasChildren));
			}
		}
	}

	connect(particleTypesBox, &QListWidget::itemChanged, this, &SelectParticleTypeModifierEditor::onParticleTypeSelected);
	particleTypesBox->setUpdatesEnabled(true);
}

/******************************************************************************
* This is called when the user has selected a new item in the property list.
******************************************************************************/
void SelectParticleTypeModifierEditor::onPropertySelected(int index)
{
	SelectParticleTypeModifier* mod = static_object_cast<SelectParticleTypeModifier>(editObject());
	if(!mod) return;

	undoableTransaction(tr("Select property"), [this, mod]() {
		mod->setSourceProperty(propertyListBox->currentProperty());
	});
}

/******************************************************************************
* This is called when the user has selected another atom type.
******************************************************************************/
void SelectParticleTypeModifierEditor::onParticleTypeSelected(QListWidgetItem* item)
{
	SelectParticleTypeModifier* mod = static_object_cast<SelectParticleTypeModifier>(editObject());
	if(!mod) return;

	QSet<int> types = mod->selectedParticleTypes();
	if(item->checkState() == Qt::Checked)
		types.insert(item->data(Qt::UserRole).toInt());
	else
		types.remove(item->data(Qt::UserRole).toInt());

	undoableTransaction(tr("Select type"), [mod, &types]() {
		mod->setSelectedParticleTypes(types);
	});
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool SelectParticleTypeModifierEditor::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	if(source == editObject() && event->type() == ReferenceEvent::TargetChanged) {
		updatePropertyList();
	}
	return ParticleModifierEditor::referenceEvent(source, event);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
