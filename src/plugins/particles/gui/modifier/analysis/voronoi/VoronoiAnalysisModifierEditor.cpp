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
#include <plugins/particles/modifier/analysis/voronoi/VoronoiAnalysisModifier.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/properties/BooleanGroupBoxParameterUI.h>
#include <gui/properties/IntegerParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include "VoronoiAnalysisModifierEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(ParticlesGui, VoronoiAnalysisModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(VoronoiAnalysisModifier, VoronoiAnalysisModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void VoronoiAnalysisModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Voronoi analysis"), rolloutParams, "particles.modifiers.voronoi_analysis.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QGridLayout* gridlayout = new QGridLayout();
	QGridLayout* sublayout;
	gridlayout->setContentsMargins(4,4,4,4);
	gridlayout->setSpacing(4);
	gridlayout->setColumnStretch(1, 1);
	int row = 0;

	// Face threshold.
	FloatParameterUI* faceThresholdPUI = new FloatParameterUI(this, PROPERTY_FIELD(VoronoiAnalysisModifier::_faceThreshold));
	gridlayout->addWidget(faceThresholdPUI->label(), row, 0);
	gridlayout->addLayout(faceThresholdPUI->createFieldLayout(), row++, 1);

	// Compute indices.
	BooleanGroupBoxParameterUI* computeIndicesPUI = new BooleanGroupBoxParameterUI(this, PROPERTY_FIELD(VoronoiAnalysisModifier::_computeIndices));
	gridlayout->addWidget(computeIndicesPUI->groupBox(), row++, 0, 1, 2);
	sublayout = new QGridLayout(computeIndicesPUI->childContainer());
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);
	sublayout->setColumnStretch(1, 1);

	// Edge count parameter.
	IntegerParameterUI* edgeCountPUI = new IntegerParameterUI(this, PROPERTY_FIELD(VoronoiAnalysisModifier::_edgeCount));
	sublayout->addWidget(edgeCountPUI->label(), 0, 0);
	sublayout->addLayout(edgeCountPUI->createFieldLayout(), 0, 1);

	// Edge threshold.
	FloatParameterUI* edgeThresholdPUI = new FloatParameterUI(this, PROPERTY_FIELD(VoronoiAnalysisModifier::_edgeThreshold));
	sublayout->addWidget(edgeThresholdPUI->label(), 1, 0);
	sublayout->addLayout(edgeThresholdPUI->createFieldLayout(), 1, 1);

	// Generate bonds.
	BooleanParameterUI* computeBondsPUI = new BooleanParameterUI(this, PROPERTY_FIELD(VoronoiAnalysisModifier::_computeBonds));
	gridlayout->addWidget(computeBondsPUI->checkBox(), row++, 0, 1, 2);

	// Atomic radii.
	BooleanParameterUI* useRadiiPUI = new BooleanParameterUI(this, PROPERTY_FIELD(VoronoiAnalysisModifier::_useRadii));
	gridlayout->addWidget(useRadiiPUI->checkBox(), row++, 0, 1, 2);

	// Only selected particles.
	BooleanParameterUI* onlySelectedPUI = new BooleanParameterUI(this, PROPERTY_FIELD(VoronoiAnalysisModifier::_onlySelected));
	gridlayout->addWidget(onlySelectedPUI->checkBox(), row++, 0, 1, 2);

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
