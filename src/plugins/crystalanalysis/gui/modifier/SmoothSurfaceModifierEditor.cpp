///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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
#include <plugins/crystalanalysis/modifier/SmoothSurfaceModifier.h>
#include <gui/properties/IntegerParameterUI.h>
#include "SmoothSurfaceModifierEditor.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_OBJECT(SmoothSurfaceModifierEditor, PropertiesEditor);
SET_OVITO_OBJECT_EDITOR(SmoothSurfaceModifier, SmoothSurfaceModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void SmoothSurfaceModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create the first rollout.
	QWidget* rollout = createRollout(tr("Smooth surface"), rolloutParams);

    QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(6);
	layout->setColumnStretch(1, 1);

	IntegerParameterUI* smoothingLevelUI = new IntegerParameterUI(this, PROPERTY_FIELD(SmoothSurfaceModifier::smoothingLevel));
	layout->addWidget(smoothingLevelUI->label(), 0, 0);
	layout->addLayout(smoothingLevelUI->createFieldLayout(), 0, 1);
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
