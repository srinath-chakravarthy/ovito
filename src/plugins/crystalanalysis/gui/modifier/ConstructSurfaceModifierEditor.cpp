///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2014) Alexander Stukowski
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
#include <plugins/crystalanalysis/modifier/ConstructSurfaceModifier.h>
#include <gui/properties/IntegerParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/properties/SubObjectParameterUI.h>
#include "ConstructSurfaceModifierEditor.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_OBJECT(CrystalAnalysisGui, ConstructSurfaceModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(ConstructSurfaceModifier, ConstructSurfaceModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void ConstructSurfaceModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create the rollout.
	QWidget* rollout = createRollout(tr("Construct surface mesh"), rolloutParams, "particles.modifiers.construct_surface_mesh.html");

    QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(6);
	layout->setColumnStretch(1, 1);

	FloatParameterUI* radiusUI = new FloatParameterUI(this, PROPERTY_FIELD(ConstructSurfaceModifier::_probeSphereRadius));
	layout->addWidget(radiusUI->label(), 0, 0);
	layout->addLayout(radiusUI->createFieldLayout(), 0, 1);

	IntegerParameterUI* smoothingLevelUI = new IntegerParameterUI(this, PROPERTY_FIELD(ConstructSurfaceModifier::_smoothingLevel));
	layout->addWidget(smoothingLevelUI->label(), 1, 0);
	layout->addLayout(smoothingLevelUI->createFieldLayout(), 1, 1);

	BooleanParameterUI* onlySelectedUI = new BooleanParameterUI(this, PROPERTY_FIELD(ConstructSurfaceModifier::_onlySelectedParticles));
	layout->addWidget(onlySelectedUI->checkBox(), 2, 0, 1, 2);

	// Status label.
	layout->setRowMinimumHeight(3, 10);
	layout->addWidget(statusLabel(), 4, 0, 1, 2);
	statusLabel()->setMinimumHeight(100);

	// Open a sub-editor for the mesh display object.
	new SubObjectParameterUI(this, PROPERTY_FIELD(ConstructSurfaceModifier::_surfaceMeshDisplay), rolloutParams.after(rollout));
}

}	// End of namespace
}	// End of namespace
}	// End of namespace

