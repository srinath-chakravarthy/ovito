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

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/crystalanalysis/objects/partition_mesh/PartitionMeshDisplay.h>
#include <gui/properties/ColorParameterUI.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/BooleanGroupBoxParameterUI.h>
#include "PartitionMeshDisplayEditor.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_OBJECT(CrystalAnalysisGui, PartitionMeshDisplayEditor, PropertiesEditor);
SET_OVITO_OBJECT_EDITOR(PartitionMeshDisplay, PartitionMeshDisplayEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void PartitionMeshDisplayEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(QString(), rolloutParams);

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QGroupBox* surfaceGroupBox = new QGroupBox(tr("Surface"));
	QGridLayout* sublayout = new QGridLayout(surfaceGroupBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);
	sublayout->setColumnStretch(1, 1);
	layout->addWidget(surfaceGroupBox);

	ColorParameterUI* surfaceColorUI = new ColorParameterUI(this, PROPERTY_FIELD(PartitionMeshDisplay::_surfaceColor));
	sublayout->addWidget(surfaceColorUI->label(), 0, 0);
	sublayout->addWidget(surfaceColorUI->colorPicker(), 0, 1);

	FloatParameterUI* surfaceTransparencyUI = new FloatParameterUI(this, PROPERTY_FIELD(PartitionMeshDisplay::_surfaceTransparency));
	sublayout->addWidget(new QLabel(tr("Transparency:")), 1, 0);
	sublayout->addLayout(surfaceTransparencyUI->createFieldLayout(), 1, 1);
	surfaceTransparencyUI->setMinValue(0);
	surfaceTransparencyUI->setMaxValue(1);

	BooleanParameterUI* smoothShadingUI = new BooleanParameterUI(this, PROPERTY_FIELD(PartitionMeshDisplay::_smoothShading));
	sublayout->addWidget(smoothShadingUI->checkBox(), 2, 0, 1, 2);

	BooleanParameterUI* flipOrientationUI = new BooleanParameterUI(this, PROPERTY_FIELD(PartitionMeshDisplay::_flipOrientation));
	sublayout->addWidget(flipOrientationUI->checkBox(), 3, 0, 1, 2);

	BooleanGroupBoxParameterUI* capGroupUI = new BooleanGroupBoxParameterUI(this, PROPERTY_FIELD(PartitionMeshDisplay::_showCap));
	capGroupUI->groupBox()->setTitle(tr("Cap polygons"));
	sublayout = new QGridLayout(capGroupUI->childContainer());
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);
	sublayout->setColumnStretch(1, 1);
	layout->addWidget(capGroupUI->groupBox());

	FloatParameterUI* capTransparencyUI = new FloatParameterUI(this, PROPERTY_FIELD(PartitionMeshDisplay::_capTransparency));
	sublayout->addWidget(new QLabel(tr("Transparency:")), 0, 0);
	sublayout->addLayout(capTransparencyUI->createFieldLayout(), 0, 1);
	capTransparencyUI->setMinValue(0);
	capTransparencyUI->setMaxValue(1);
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
