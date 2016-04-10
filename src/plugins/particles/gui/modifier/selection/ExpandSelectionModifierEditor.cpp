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
#include <plugins/particles/modifier/selection/ExpandSelectionModifier.h>
#include <gui/properties/IntegerRadioButtonParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/IntegerParameterUI.h>
#include "ExpandSelectionModifierEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Selection) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(ParticlesGui, ExpandSelectionModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(ExpandSelectionModifier, ExpandSelectionModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void ExpandSelectionModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Expand selection"), rolloutParams, "particles.modifiers.expand_selection.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(6);

	QLabel* label = new QLabel(tr("Expand current selection to include particles that are..."));
	label->setWordWrap(true);
	layout->addWidget(label);

	IntegerRadioButtonParameterUI* modePUI = new IntegerRadioButtonParameterUI(this, PROPERTY_FIELD(ExpandSelectionModifier::_mode));
	QRadioButton* cutoffModeBtn = modePUI->addRadioButton(ExpandSelectionModifier::CutoffRange, tr("... within the range:"));
	layout->addSpacing(10);
	layout->addWidget(cutoffModeBtn);

	// Cutoff parameter.
	FloatParameterUI* cutoffRadiusPUI = new FloatParameterUI(this, PROPERTY_FIELD(ExpandSelectionModifier::_cutoffRange));
	QHBoxLayout* sublayout = new QHBoxLayout();
	sublayout->setContentsMargins(0,0,0,0);
	sublayout->addSpacing(20);
	sublayout->addWidget(cutoffRadiusPUI->label());
	sublayout->addLayout(cutoffRadiusPUI->createFieldLayout(), 1);
	layout->addLayout(sublayout);
	cutoffRadiusPUI->setMinValue(0);
	cutoffRadiusPUI->setEnabled(false);
	connect(cutoffModeBtn, &QRadioButton::toggled, cutoffRadiusPUI, &FloatParameterUI::setEnabled);

	QRadioButton* nearestNeighborsModeBtn = modePUI->addRadioButton(ExpandSelectionModifier::NearestNeighbors, tr("... among the N nearest neighbors:"));
	layout->addSpacing(10);
	layout->addWidget(nearestNeighborsModeBtn);

	// Number of nearest neighbors.
	IntegerParameterUI* numNearestNeighborsPUI = new IntegerParameterUI(this, PROPERTY_FIELD(ExpandSelectionModifier::_numNearestNeighbors));
	sublayout = new QHBoxLayout();
	sublayout->setContentsMargins(0,0,0,0);
	sublayout->addSpacing(20);
	sublayout->addWidget(numNearestNeighborsPUI->label());
	sublayout->addLayout(numNearestNeighborsPUI->createFieldLayout(), 1);
	layout->addLayout(sublayout);
	numNearestNeighborsPUI->setMinValue(1);
	numNearestNeighborsPUI->setMaxValue(ExpandSelectionModifier::MAX_NEAREST_NEIGHBORS);
	numNearestNeighborsPUI->setEnabled(false);
	connect(nearestNeighborsModeBtn, &QRadioButton::toggled, numNearestNeighborsPUI, &FloatParameterUI::setEnabled);

	QRadioButton* bondModeBtn = modePUI->addRadioButton(ExpandSelectionModifier::BondedNeighbors, tr("... bonded to a selected particle."));
	layout->addSpacing(10);
	layout->addWidget(bondModeBtn);

	layout->addSpacing(10);
	IntegerParameterUI* numIterationsPUI = new IntegerParameterUI(this, PROPERTY_FIELD(ExpandSelectionModifier::_numIterations));
	sublayout = new QHBoxLayout();
	sublayout->setContentsMargins(0,0,0,0);
	sublayout->addWidget(numIterationsPUI->label());
	sublayout->addLayout(numIterationsPUI->createFieldLayout(), 1);
	layout->addLayout(sublayout);
	numIterationsPUI->setMinValue(1);

	// Status label.
	layout->addSpacing(10);
	layout->addWidget(statusLabel());
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
