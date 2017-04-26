///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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
#include <plugins/particles/modifier/fields/CreateIsosurfaceModifier.h>
#include <plugins/particles/gui/util/FieldQuantityParameterUI.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/properties/IntegerParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/SubObjectParameterUI.h>
#include "CreateIsosurfaceModifierEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Fields) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(CreateIsosurfaceModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(CreateIsosurfaceModifier, CreateIsosurfaceModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void CreateIsosurfaceModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Create isosurface"), rolloutParams, "particles.modifiers.create_isosurface.html");

    // Create the rollout contents.
	QVBoxLayout* layout1 = new QVBoxLayout(rollout);
	layout1->setContentsMargins(4,4,4,4);
	layout1->setSpacing(4);

	QGridLayout* layout2 = new QGridLayout();
	layout2->setContentsMargins(0,0,0,0);
	layout2->setSpacing(4);
	layout2->setColumnStretch(1, 1);
	layout1->addLayout(layout2);

	FieldQuantityParameterUI* fieldQuantityUI = new FieldQuantityParameterUI(this, PROPERTY_FIELD(CreateIsosurfaceModifier::sourceQuantity));
	layout2->addWidget(new QLabel(tr("Field quantity:")), 0, 0);
	layout2->addWidget(fieldQuantityUI->comboBox(), 0, 1);

	// Isolevel parameter.
	FloatParameterUI* isolevelPUI = new FloatParameterUI(this, PROPERTY_FIELD(CreateIsosurfaceModifier::isolevelController));
	layout2->addWidget(isolevelPUI->label(), 1, 0);
	layout2->addLayout(isolevelPUI->createFieldLayout(), 1, 1);

	// Status label.
	layout1->addSpacing(8);
	layout1->addWidget(statusLabel());

	// Open a sub-editor for the mesh display object.
	new SubObjectParameterUI(this, PROPERTY_FIELD(CreateIsosurfaceModifier::surfaceMeshDisplay), rolloutParams.after(rollout));
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
