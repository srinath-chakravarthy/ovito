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
#include <plugins/particles/modifier/modify/ShowPeriodicImagesModifier.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/properties/IntegerParameterUI.h>
#include "ShowPeriodicImagesModifierEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(ParticlesGui, ShowPeriodicImagesModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(ShowPeriodicImagesModifier, ShowPeriodicImagesModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void ShowPeriodicImagesModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	QWidget* panel = createRollout(tr("Show periodic images"), rolloutParams, "particles.modifiers.show_periodic_images.html");

    // Create the rollout contents.
	QGridLayout* layout = new QGridLayout(panel);
	layout->setContentsMargins(4,4,4,4);
#ifndef Q_OS_MACX
	layout->setHorizontalSpacing(2);
	layout->setVerticalSpacing(2);
#endif
	layout->setColumnStretch(1, 1);

	BooleanParameterUI* showPeriodicImageXUI = new BooleanParameterUI(this, PROPERTY_FIELD(ShowPeriodicImagesModifier::_showImageX));
	layout->addWidget(showPeriodicImageXUI->checkBox(), 0, 0);
	IntegerParameterUI* numImagesXPUI = new IntegerParameterUI(this, PROPERTY_FIELD(ShowPeriodicImagesModifier::_numImagesX));
	numImagesXPUI->setMinValue(1);
	layout->addLayout(numImagesXPUI->createFieldLayout(), 0, 1);

	BooleanParameterUI* showPeriodicImageYUI = new BooleanParameterUI(this, PROPERTY_FIELD(ShowPeriodicImagesModifier::_showImageY));
	layout->addWidget(showPeriodicImageYUI->checkBox(), 1, 0);
	IntegerParameterUI* numImagesYPUI = new IntegerParameterUI(this, PROPERTY_FIELD(ShowPeriodicImagesModifier::_numImagesY));
	numImagesYPUI->setMinValue(1);
	layout->addLayout(numImagesYPUI->createFieldLayout(), 1, 1);

	BooleanParameterUI* showPeriodicImageZUI = new BooleanParameterUI(this, PROPERTY_FIELD(ShowPeriodicImagesModifier::_showImageZ));
	layout->addWidget(showPeriodicImageZUI->checkBox(), 2, 0);
	IntegerParameterUI* numImagesZPUI = new IntegerParameterUI(this, PROPERTY_FIELD(ShowPeriodicImagesModifier::_numImagesZ));
	numImagesZPUI->setMinValue(1);
	layout->addLayout(numImagesZPUI->createFieldLayout(), 2, 1);

	BooleanParameterUI* adjustBoxSizeUI = new BooleanParameterUI(this, PROPERTY_FIELD(ShowPeriodicImagesModifier::_adjustBoxSize));
	layout->addWidget(adjustBoxSizeUI->checkBox(), 3, 0, 1, 2);

	BooleanParameterUI* uniqueIdentifiersUI = new BooleanParameterUI(this, PROPERTY_FIELD(ShowPeriodicImagesModifier::_uniqueIdentifiers));
	layout->addWidget(uniqueIdentifiersUI->checkBox(), 4, 0, 1, 2);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
