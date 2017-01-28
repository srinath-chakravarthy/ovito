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
#include <plugins/crystalanalysis/importer/CAImporter.h>
#include <gui/properties/BooleanParameterUI.h>
#include "CAImporterEditor.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_OBJECT(CAImporterEditor, PropertiesEditor);
SET_OVITO_OBJECT_EDITOR(CAImporter, CAImporterEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void CAImporterEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Crystal analysis file"), rolloutParams);

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	BooleanParameterUI* loadParticlesUI = new BooleanParameterUI(this, PROPERTY_FIELD(CAImporter::loadParticles));
	layout->addWidget(loadParticlesUI->checkBox());

	// Multi-timestep file
	BooleanParameterUI* multitimestepUI = new BooleanParameterUI(this, PROPERTY_FIELD(ParticleImporter::isMultiTimestepFile));
	layout->addWidget(multitimestepUI->checkBox());
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
