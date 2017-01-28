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
#include <plugins/particles/modifier/coloring/AssignColorModifier.h>
#include <gui/properties/ColorParameterUI.h>
#include <gui/properties/BooleanParameterUI.h>
#include "AssignColorModifierEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Coloring) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(AssignColorModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(AssignColorModifier, AssignColorModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void AssignColorModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Assign color"), rolloutParams, "particles.modifiers.assign_color.html");

    // Create the rollout contents.
	QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(0);
	layout->setColumnStretch(1, 1);

	// Color parameter.
	ColorParameterUI* constColorPUI = new ColorParameterUI(this, PROPERTY_FIELD(AssignColorModifier::colorController));
	layout->addWidget(constColorPUI->label(), 0, 0);
	layout->addWidget(constColorPUI->colorPicker(), 0, 1);

	// Keep selection parameter.
	BooleanParameterUI* keepSelectionPUI = new BooleanParameterUI(this, PROPERTY_FIELD(AssignColorModifier::keepSelection));
	layout->addWidget(keepSelectionPUI->checkBox(), 1, 0, 1, 2);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
