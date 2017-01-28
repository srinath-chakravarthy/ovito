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
#include <plugins/particles/modifier/analysis/centrosymmetry/CentroSymmetryModifier.h>
#include <gui/properties/IntegerParameterUI.h>
#include "CentroSymmetryModifierEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(CentroSymmetryModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(CentroSymmetryModifier, CentroSymmetryModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void CentroSymmetryModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Centrosymmetry parameter"), rolloutParams, "particles.modifiers.centrosymmetry.html");

    // Create the rollout contents.
	QVBoxLayout* layout1 = new QVBoxLayout(rollout);
	layout1->setContentsMargins(4,4,4,4);
	layout1->setSpacing(4);

	QGridLayout* layout2 = new QGridLayout();
	layout2->setContentsMargins(0,0,0,0);
	layout2->setSpacing(4);
	layout2->setColumnStretch(1, 1);
	layout1->addLayout(layout2);

	// Num neighbors parameter.
	IntegerParameterUI* numNeighborsPUI = new IntegerParameterUI(this, PROPERTY_FIELD(CentroSymmetryModifier::numNeighbors));
	layout2->addWidget(numNeighborsPUI->label(), 0, 0);
	layout2->addLayout(numNeighborsPUI->createFieldLayout(), 0, 1);

	QLabel* infoLabel = new QLabel(tr("This parameter specifies the number of nearest neighbors in the underlying lattice of atoms. For FCC and BCC lattices, set this to 12 and 8 respectively. More generally, it must be a positive, even integer."));
	infoLabel->setWordWrap(true);
	layout1->addWidget(infoLabel);

	// Status label.
	layout1->addSpacing(10);
	layout1->addWidget(statusLabel());
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
