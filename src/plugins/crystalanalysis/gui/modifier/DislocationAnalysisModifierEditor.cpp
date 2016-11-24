///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2015) Alexander Stukowski
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
#include <plugins/crystalanalysis/modifier/dxa/DislocationAnalysisModifier.h>
#include <plugins/particles/gui/modifier/analysis/StructureListParameterUI.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/properties/IntegerParameterUI.h>
#include <gui/properties/VariantComboBoxParameterUI.h>
#include <gui/properties/SubObjectParameterUI.h>
#include "DislocationAnalysisModifierEditor.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_OBJECT(CrystalAnalysisGui, DislocationAnalysisModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(DislocationAnalysisModifier, DislocationAnalysisModifierEditor);

IMPLEMENT_OVITO_OBJECT(CrystalAnalysisGui, DislocationTypeListParameterUI, RefTargetListParameterUI);
DEFINE_FLAGS_REFERENCE_FIELD(DislocationTypeListParameterUI, _modifier, "Modifier", DislocationAnalysisModifier, PROPERTY_FIELD_NO_UNDO | PROPERTY_FIELD_WEAK_REF | PROPERTY_FIELD_NO_CHANGE_MESSAGE);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void DislocationAnalysisModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create the rollout.
	QWidget* rollout = createRollout(tr("Dislocation analysis"), rolloutParams, "particles.modifiers.dislocation_analysis.html");

    QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(6);

	QGroupBox* structureBox = new QGroupBox(tr("Input crystal type"));
	layout->addWidget(structureBox);
	QVBoxLayout* sublayout1 = new QVBoxLayout(structureBox);
	sublayout1->setContentsMargins(4,4,4,4);
	VariantComboBoxParameterUI* crystalStructureUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(DislocationAnalysisModifier::_inputCrystalStructure));

	crystalStructureUI->comboBox()->addItem(tr("Face-centered cubic (FCC)"), QVariant::fromValue((int)StructureAnalysis::LATTICE_FCC));
	crystalStructureUI->comboBox()->addItem(tr("Hexagonal close-packed (HCP)"), QVariant::fromValue((int)StructureAnalysis::LATTICE_HCP));
	crystalStructureUI->comboBox()->addItem(tr("Body-centered cubic (BCC)"), QVariant::fromValue((int)StructureAnalysis::LATTICE_BCC));
	crystalStructureUI->comboBox()->addItem(tr("Diamond cubic / Zinc blende"), QVariant::fromValue((int)StructureAnalysis::LATTICE_CUBIC_DIAMOND));
	crystalStructureUI->comboBox()->addItem(tr("Diamond hexagonal / Wurtzite"), QVariant::fromValue((int)StructureAnalysis::LATTICE_HEX_DIAMOND));
	sublayout1->addWidget(crystalStructureUI->comboBox());

	QGroupBox* dxaParamsBox = new QGroupBox(tr("DXA parameters"));
	layout->addWidget(dxaParamsBox);
	QGridLayout* sublayout = new QGridLayout(dxaParamsBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);
	sublayout->setColumnStretch(1, 1);

	IntegerParameterUI* maxTrialCircuitSizeUI = new IntegerParameterUI(this, PROPERTY_FIELD(DislocationAnalysisModifier::_maxTrialCircuitSize));
	sublayout->addWidget(maxTrialCircuitSizeUI->label(), 0, 0);
	sublayout->addLayout(maxTrialCircuitSizeUI->createFieldLayout(), 0, 1);

	IntegerParameterUI* circuitStretchabilityUI = new IntegerParameterUI(this, PROPERTY_FIELD(DislocationAnalysisModifier::_circuitStretchability));
	sublayout->addWidget(circuitStretchabilityUI->label(), 1, 0);
	sublayout->addLayout(circuitStretchabilityUI->createFieldLayout(), 1, 1);

	QGroupBox* advancedParamsBox = new QGroupBox(tr("Advanced settings"));
	layout->addWidget(advancedParamsBox);
	sublayout = new QGridLayout(advancedParamsBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);
	sublayout->setColumnStretch(0, 1);

#if 0
	BooleanParameterUI* reconstructEdgeVectorsUI = new BooleanParameterUI(this, PROPERTY_FIELD(DislocationAnalysisModifier::_reconstructEdgeVectors));
	sublayout->addWidget(reconstructEdgeVectorsUI->checkBox(), 0, 0);
#endif

	BooleanParameterUI* onlySelectedParticlesUI = new BooleanParameterUI(this, PROPERTY_FIELD(StructureIdentificationModifier::_onlySelectedParticles));
	sublayout->addWidget(onlySelectedParticlesUI->checkBox(), 0, 0);

	BooleanParameterUI* outputInterfaceMeshUI = new BooleanParameterUI(this, PROPERTY_FIELD(DislocationAnalysisModifier::_outputInterfaceMesh));
	sublayout->addWidget(outputInterfaceMeshUI->checkBox(), 1, 0);

	BooleanParameterUI* onlyPerfectDislocationsUI = new BooleanParameterUI(this, PROPERTY_FIELD(DislocationAnalysisModifier::_onlyPerfectDislocations));
	sublayout->addWidget(onlyPerfectDislocationsUI->checkBox(), 2, 0);

	// Status label.
	layout->addWidget(statusLabel());
	//statusLabel()->setMinimumHeight(60);

	// Structure list.
	StructureListParameterUI* structureTypesPUI = new StructureListParameterUI(this);
	layout->addSpacing(10);
	layout->addWidget(new QLabel(tr("Structure analysis results:")));
	layout->addWidget(structureTypesPUI->tableWidget());

	// Open a sub-editor for the mesh display object.
	//new SubObjectParameterUI(this, PROPERTY_FIELD(DislocationAnalysisModifier::_defectMeshDisplay), rolloutParams.after(rollout));

	// Open a sub-editor for the internal surface smoothing modifier.
	new SubObjectParameterUI(this, PROPERTY_FIELD(DislocationAnalysisModifier::_smoothSurfaceModifier), rolloutParams.after(rollout).setTitle(tr("Post-processing")));

	// Open a sub-editor for the dislocation display object.
	//new SubObjectParameterUI(this, PROPERTY_FIELD(DislocationAnalysisModifier::_dislocationDisplay), rolloutParams.after(rollout));

	// Open a sub-editor for the internal line smoothing modifier.
	new SubObjectParameterUI(this, PROPERTY_FIELD(DislocationAnalysisModifier::_smoothDislocationsModifier), rolloutParams.after(rollout).setTitle(tr("Post-processing")));

	// Burgers vector list.
	_burgersFamilyListUI.reset(new DislocationTypeListParameterUI());
	layout->addSpacing(10);
	layout->addWidget(new QLabel(tr("Dislocation analysis results:")));
	layout->addWidget(_burgersFamilyListUI->tableWidget());
	connect(this, &PropertiesEditor::contentsChanged, [this](RefTarget* editObject) {
		_burgersFamilyListUI->setModifier(static_object_cast<DislocationAnalysisModifier>(editObject));
	});
}

/******************************************************************************
* Constructor.
******************************************************************************/
DislocationTypeListParameterUI::DislocationTypeListParameterUI(QObject* parent)
	: RefTargetListParameterUI(parent, PROPERTY_FIELD(StructurePattern::_burgersVectorFamilies))
{
	INIT_PROPERTY_FIELD(DislocationTypeListParameterUI::_modifier);

	connect(tableWidget(160), &QTableWidget::doubleClicked, this, &DislocationTypeListParameterUI::onDoubleClickDislocationType);
	tableWidget()->setAutoScroll(false);
}

/******************************************************************************
* Sets the modifier whose results should be displayed.
******************************************************************************/
void DislocationTypeListParameterUI::setModifier(DislocationAnalysisModifier* modifier)
{
	if(modifier)
		setEditObject(modifier->patternCatalog()->structureById(modifier->inputCrystalStructure()));
	else
		setEditObject(nullptr);
	_modifier = modifier;
}

/******************************************************************************
* Returns a data item from the list data model.
******************************************************************************/
QVariant DislocationTypeListParameterUI::getItemData(RefTarget* target, const QModelIndex& index, int role)
{
	BurgersVectorFamily* family = dynamic_object_cast<BurgersVectorFamily>(target);
	if(family && _modifier) {
		if(role == Qt::DisplayRole) {
			if(index.column() == 1) {
				return family->name();
			}
			else if(index.column() == 2) {
				auto entry = _modifier->segmentCounts().find(family);
				if(entry != _modifier->segmentCounts().end())
					return entry->second;
			}
			else if(index.column() == 3) {
				auto entry = _modifier->dislocationLengths().find(family);
				if(entry != _modifier->dislocationLengths().end())
					return QString::number(entry->second);
			}
		}
		else if(role == Qt::DecorationRole) {
			if(index.column() == 0)
				return (QColor)family->color();
		}
	}
	return QVariant();
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool DislocationTypeListParameterUI::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	if(source == _modifier && event->type() == ReferenceEvent::ObjectStatusChanged) {
		// Update the result columns.
		_model->updateColumns(2, 3);
	}
	return RefTargetListParameterUI::referenceEvent(source, event);
}

/******************************************************************************
* Is called when the user has double-clicked on one of the dislocation
* types in the list widget.
******************************************************************************/
void DislocationTypeListParameterUI::onDoubleClickDislocationType(const QModelIndex& index)
{
	// Let the user select a color for the structure type.
	BurgersVectorFamily* family = static_object_cast<BurgersVectorFamily>(selectedObject());
	if(!family) return;

	QColor oldColor = (QColor)family->color();
	QColor newColor = QColorDialog::getColor(oldColor, _viewWidget);
	if(!newColor.isValid() || newColor == oldColor) return;

	undoableTransaction(tr("Change dislocation type color"), [family, newColor]() {
		family->setColor(Color(newColor));
	});
}

}	// End of namespace
}	// End of namespace
}	// End of namespace

