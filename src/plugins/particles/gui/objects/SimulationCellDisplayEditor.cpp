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
#include <plugins/particles/objects/SimulationCellDisplay.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/ColorParameterUI.h>
#include <gui/properties/BooleanParameterUI.h>
#include "SimulationCellDisplayEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(ParticlesGui, SimulationCellDisplayEditor, PropertiesEditor);
SET_OVITO_OBJECT_EDITOR(SimulationCellDisplay, SimulationCellDisplayEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void SimulationCellDisplayEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(QString(), rolloutParams);

    // Create the rollout contents.
	QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1, 1);

	// Render cell
	BooleanParameterUI* renderCellUI = new BooleanParameterUI(this, PROPERTY_FIELD(SimulationCellDisplay::_renderSimulationCell));
	layout->addWidget(renderCellUI->checkBox(), 0, 0, 1, 2);

	// Line width
	FloatParameterUI* scalingFactorUI = new FloatParameterUI(this, PROPERTY_FIELD(SimulationCellDisplay::_simulationCellLineWidth));
	layout->addWidget(scalingFactorUI->label(), 1, 0);
	layout->addLayout(scalingFactorUI->createFieldLayout(), 1, 1);
	scalingFactorUI->setMinValue(0);

	// Line color
	ColorParameterUI* lineColorUI = new ColorParameterUI(this, PROPERTY_FIELD(SimulationCellDisplay::_simulationCellColor));
	layout->addWidget(lineColorUI->label(), 2, 0);
	layout->addWidget(lineColorUI->colorPicker(), 2, 1);
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
