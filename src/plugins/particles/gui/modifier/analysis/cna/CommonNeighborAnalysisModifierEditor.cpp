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
#include <plugins/particles/modifier/analysis/cna/CommonNeighborAnalysisModifier.h>
#include <plugins/particles/gui/modifier/analysis/StructureListParameterUI.h>
#include <plugins/particles/gui/util/CutoffRadiusPresetsUI.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/properties/IntegerRadioButtonParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include "CommonNeighborAnalysisModifierEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(CommonNeighborAnalysisModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(CommonNeighborAnalysisModifier, CommonNeighborAnalysisModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void CommonNeighborAnalysisModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Common neighbor analysis"), rolloutParams, "particles.modifiers.common_neighbor_analysis.html");

    // Create the rollout contents.
	QVBoxLayout* layout1 = new QVBoxLayout(rollout);
	layout1->setContentsMargins(4,4,4,4);
	layout1->setSpacing(6);

	IntegerRadioButtonParameterUI* modeUI = new IntegerRadioButtonParameterUI(this, PROPERTY_FIELD(CommonNeighborAnalysisModifier::mode));
	QRadioButton* bondModeBtn = modeUI->addRadioButton(CommonNeighborAnalysisModifier::BondMode, tr("Bond-based CNA (without cutoff)"));
	QRadioButton* adaptiveModeBtn = modeUI->addRadioButton(CommonNeighborAnalysisModifier::AdaptiveCutoffMode, tr("Adaptive CNA (variable cutoff)"));
	QRadioButton* fixedCutoffModeBtn = modeUI->addRadioButton(CommonNeighborAnalysisModifier::FixedCutoffMode, tr("Conventional CNA (fixed cutoff)"));
	layout1->addWidget(bondModeBtn);
	layout1->addWidget(adaptiveModeBtn);
	layout1->addWidget(fixedCutoffModeBtn);

	QGridLayout* gridlayout = new QGridLayout();
	gridlayout->setContentsMargins(0,0,0,0);
	gridlayout->setColumnStretch(2, 1);
	gridlayout->setColumnMinimumWidth(0, 20);

	// Cutoff parameter.
	FloatParameterUI* cutoffRadiusPUI = new FloatParameterUI(this, PROPERTY_FIELD(CommonNeighborAnalysisModifier::cutoff));
	gridlayout->addWidget(cutoffRadiusPUI->label(), 0, 1);
	gridlayout->addLayout(cutoffRadiusPUI->createFieldLayout(), 0, 2);

	CutoffRadiusPresetsUI* cutoffPresetsPUI = new CutoffRadiusPresetsUI(this, PROPERTY_FIELD(CommonNeighborAnalysisModifier::cutoff));
	gridlayout->addWidget(cutoffPresetsPUI->comboBox(), 1, 1, 1, 2);
	layout1->addLayout(gridlayout);

	connect(fixedCutoffModeBtn, &QRadioButton::toggled, cutoffRadiusPUI, &FloatParameterUI::setEnabled);
	connect(fixedCutoffModeBtn, &QRadioButton::toggled, cutoffPresetsPUI, &CutoffRadiusPresetsUI::setEnabled);
	cutoffRadiusPUI->setEnabled(false);
	cutoffPresetsPUI->setEnabled(false);

	// Use only selected particles.
	BooleanParameterUI* onlySelectedParticlesUI = new BooleanParameterUI(this, PROPERTY_FIELD(StructureIdentificationModifier::onlySelectedParticles));
	layout1->addWidget(onlySelectedParticlesUI->checkBox());

	// Status label.
	layout1->addSpacing(10);
	layout1->addWidget(statusLabel());

	StructureListParameterUI* structureTypesPUI = new StructureListParameterUI(this, true);
	layout1->addSpacing(10);
	layout1->addWidget(new QLabel(tr("Structure types:")));
	layout1->addWidget(structureTypesPUI->tableWidget());
	QLabel* label = new QLabel(tr("<p style=\"font-size: small;\">Double-click to change colors. Defaults can be set in the application settings.</p>"));
	label->setWordWrap(true);
	layout1->addWidget(label);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
