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
#include <plugins/particles/modifier/analysis/structural_clustering/StructuralClusteringModifier.h>
#include <gui/properties/IntegerParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include "StructuralClusteringModifierEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(ParticlesGui, StructuralClusteringModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(StructuralClusteringModifier, StructuralClusteringModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void StructuralClusteringModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Structural clustering"), rolloutParams);

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QGridLayout* gridlayout = new QGridLayout();
	gridlayout->setContentsMargins(4,4,4,4);
	gridlayout->setColumnStretch(1, 1);

	// Num neighbors parameter.
	IntegerParameterUI* numNeighborsPUI = new IntegerParameterUI(this, PROPERTY_FIELD(StructuralClusteringModifier::_numNeighbors));
	gridlayout->addWidget(numNeighborsPUI->label(), 0, 0);
	gridlayout->addLayout(numNeighborsPUI->createFieldLayout(), 0, 1);

	// Cutoff parameter.
	FloatParameterUI* distanceCutoffPUI = new FloatParameterUI(this, PROPERTY_FIELD(StructuralClusteringModifier::_cutoff));
	gridlayout->addWidget(distanceCutoffPUI->label(), 1, 0);
	gridlayout->addLayout(distanceCutoffPUI->createFieldLayout(), 1, 1);

	// RMSD threshold parameter.
	FloatParameterUI* rmsdThresholdPUI = new FloatParameterUI(this, PROPERTY_FIELD(StructuralClusteringModifier::_rmsdThreshold));
	gridlayout->addWidget(rmsdThresholdPUI->label(), 2, 0);
	gridlayout->addLayout(rmsdThresholdPUI->createFieldLayout(), 2, 1);

	layout->addLayout(gridlayout);

	// Status label.
	layout->addSpacing(6);
	layout->addWidget(statusLabel());
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
