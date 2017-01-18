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
#include <plugins/particles/modifier/coloring/AmbientOcclusionModifier.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/properties/IntegerParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include "AmbientOcclusionModifierEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Coloring) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(AmbientOcclusionModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(AmbientOcclusionModifier, AmbientOcclusionModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void AmbientOcclusionModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Ambient occlusion"), rolloutParams, "particles.modifiers.ambient_occlusion.html");

    // Create the rollout contents.
	QVBoxLayout* layout1 = new QVBoxLayout(rollout);
	layout1->setContentsMargins(4,4,4,4);
	layout1->setSpacing(4);

	QGridLayout* layout2 = new QGridLayout();
	layout2->setContentsMargins(0,0,0,0);
	layout2->setSpacing(4);
	layout2->setColumnStretch(1, 1);
	layout1->addLayout(layout2);

	// Intensity parameter.
	FloatParameterUI* intensityPUI = new FloatParameterUI(this, PROPERTY_FIELD(AmbientOcclusionModifier::_intensity));
	layout2->addWidget(intensityPUI->label(), 0, 0);
	layout2->addLayout(intensityPUI->createFieldLayout(), 0, 1);

	// Sampling level parameter.
	IntegerParameterUI* samplingCountPUI = new IntegerParameterUI(this, PROPERTY_FIELD(AmbientOcclusionModifier::_samplingCount));
	layout2->addWidget(samplingCountPUI->label(), 1, 0);
	layout2->addLayout(samplingCountPUI->createFieldLayout(), 1, 1);

	// Buffer resolution parameter.
	IntegerParameterUI* bufferResPUI = new IntegerParameterUI(this, PROPERTY_FIELD(AmbientOcclusionModifier::_bufferResolution));
	layout2->addWidget(bufferResPUI->label(), 2, 0);
	layout2->addLayout(bufferResPUI->createFieldLayout(), 2, 1);

	// Status label.
	layout1->addSpacing(10);
	layout1->addWidget(statusLabel());
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
