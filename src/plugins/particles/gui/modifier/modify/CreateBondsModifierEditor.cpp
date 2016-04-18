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
#include <plugins/particles/modifier/modify/CreateBondsModifier.h>
#include <plugins/particles/objects/ParticleTypeProperty.h>
#include <gui/properties/SubObjectParameterUI.h>
#include <gui/properties/IntegerRadioButtonParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/BooleanParameterUI.h>
#include "CreateBondsModifierEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(ParticlesGui, CreateBondsModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(CreateBondsModifier, CreateBondsModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void CreateBondsModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Create bonds"), rolloutParams, "particles.modifiers.create_bonds.html");

    // Create the rollout contents.
	QVBoxLayout* layout1 = new QVBoxLayout(rollout);
	layout1->setContentsMargins(4,4,4,4);
	layout1->setSpacing(6);

	QGridLayout* gridlayout = new QGridLayout();
	gridlayout->setContentsMargins(0,0,0,0);
	gridlayout->setColumnStretch(1, 1);

	IntegerRadioButtonParameterUI* cutoffModePUI = new IntegerRadioButtonParameterUI(this, PROPERTY_FIELD(CreateBondsModifier::_cutoffMode));
	QRadioButton* uniformCutoffModeBtn = cutoffModePUI->addRadioButton(CreateBondsModifier::UniformCutoff, tr("Uniform cutoff radius"));

	// Cutoff parameter.
	FloatParameterUI* cutoffRadiusPUI = new FloatParameterUI(this, PROPERTY_FIELD(CreateBondsModifier::_uniformCutoff));
	gridlayout->addWidget(uniformCutoffModeBtn, 0, 0);
	gridlayout->addLayout(cutoffRadiusPUI->createFieldLayout(), 0, 1);
	cutoffRadiusPUI->setMinValue(0);
	cutoffRadiusPUI->setEnabled(false);
	connect(uniformCutoffModeBtn, &QRadioButton::toggled, cutoffRadiusPUI, &FloatParameterUI::setEnabled);

	layout1->addLayout(gridlayout);

	QRadioButton* pairCutoffModeBtn = cutoffModePUI->addRadioButton(CreateBondsModifier::PairCutoff, tr("Pair-wise cutoff radii:"));
	layout1->addWidget(pairCutoffModeBtn);

	_pairCutoffTable = new QTableView();
	_pairCutoffTable->verticalHeader()->setVisible(false);
	_pairCutoffTable->setEnabled(false);
	_pairCutoffTableModel = new PairCutoffTableModel(_pairCutoffTable);
	_pairCutoffTable->setModel(_pairCutoffTableModel);
	connect(pairCutoffModeBtn,&QRadioButton::toggled, _pairCutoffTable, &QTableView::setEnabled);
	layout1->addWidget(_pairCutoffTable);

	BooleanParameterUI* onlyIntraMoleculeBondsUI = new BooleanParameterUI(this, PROPERTY_FIELD(CreateBondsModifier::_onlyIntraMoleculeBonds));
	layout1->addWidget(onlyIntraMoleculeBondsUI->checkBox());

	// Status label.
	layout1->addSpacing(10);
	layout1->addWidget(statusLabel());

	// Open a sub-editor for the bonds display object.
	new SubObjectParameterUI(this, PROPERTY_FIELD(CreateBondsModifier::_bondsDisplay), rolloutParams.after(rollout));

	// Update pair-wise cutoff table whenever a modifier has been loaded into the editor.
	connect(this, &CreateBondsModifierEditor::contentsReplaced, this, &CreateBondsModifierEditor::updatePairCutoffList);
	connect(this, &CreateBondsModifierEditor::contentsChanged, this, &CreateBondsModifierEditor::updatePairCutoffListValues);
}

/******************************************************************************
* Updates the contents of the pair-wise cutoff table.
******************************************************************************/
void CreateBondsModifierEditor::updatePairCutoffList()
{
	CreateBondsModifier* mod = static_object_cast<CreateBondsModifier>(editObject());
	if(!mod) return;

	// Obtain the list of particle types in the modifier's input.
	PairCutoffTableModel::ContentType pairCutoffs;
	PipelineFlowState inputState = mod->getModifierInput();
	ParticleTypeProperty* typeProperty = dynamic_object_cast<ParticleTypeProperty>(
			ParticlePropertyObject::findInState(inputState, ParticleProperty::ParticleTypeProperty));
	if(typeProperty) {
		for(auto ptype1 = typeProperty->particleTypes().constBegin(); ptype1 != typeProperty->particleTypes().constEnd(); ++ptype1) {
			for(auto ptype2 = ptype1; ptype2 != typeProperty->particleTypes().constEnd(); ++ptype2) {
				pairCutoffs.push_back(qMakePair((*ptype1)->name(), (*ptype2)->name()));
			}
		}
	}
	_pairCutoffTableModel->setContent(mod, pairCutoffs);
}

/******************************************************************************
* Updates the cutoff values in the pair-wise cutoff table.
******************************************************************************/
void CreateBondsModifierEditor::updatePairCutoffListValues()
{
	_pairCutoffTableModel->updateContent();
}

/******************************************************************************
* Returns data from the pair-cutoff table model.
******************************************************************************/
QVariant CreateBondsModifierEditor::PairCutoffTableModel::data(const QModelIndex& index, int role) const
{
	if(role == Qt::DisplayRole) {
		if(index.column() == 0) return _data[index.row()].first;
		else if(index.column() == 1) return _data[index.row()].second;
		else if(index.column() == 2) {
			FloatType cutoffRadius = _modifier->pairCutoffs()[_data[index.row()]];
			if(cutoffRadius > 0)
				return QString("%1").arg(cutoffRadius);
		}
	}
	return QVariant();
}

/******************************************************************************
* Sets data in the pair-cutoff table model.
******************************************************************************/
bool CreateBondsModifierEditor::PairCutoffTableModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if(role == Qt::EditRole && index.column() == 2) {
		bool ok;
		FloatType cutoff = (FloatType)value.toDouble(&ok);
		if(!ok) cutoff = 0;

		CreateBondsModifier::PairCutoffsList pairCutoffs = _modifier->pairCutoffs();
		pairCutoffs[_data[index.row()]] = cutoff;

		UndoableTransaction::handleExceptions(_modifier->dataset()->undoStack(), tr("Change cutoff"), [&pairCutoffs, this]() {
			_modifier->setPairCutoffs(pairCutoffs);
		});
		return true;
	}
	return false;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
