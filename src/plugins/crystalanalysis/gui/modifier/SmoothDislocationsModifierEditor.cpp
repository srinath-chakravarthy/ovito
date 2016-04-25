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
#include <plugins/crystalanalysis/modifier/SmoothDislocationsModifier.h>
#include <gui/properties/IntegerParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/BooleanGroupBoxParameterUI.h>
#include "SmoothDislocationsModifierEditor.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_OBJECT(CrystalAnalysisGui, SmoothDislocationsModifierEditor, PropertiesEditor);
SET_OVITO_OBJECT_EDITOR(SmoothDislocationsModifier, SmoothDislocationsModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void SmoothDislocationsModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create the first rollout.
	QWidget* rollout = createRollout(tr("Smooth dislocations"), rolloutParams);

    QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);

	BooleanGroupBoxParameterUI* smoothingEnabledUI = new BooleanGroupBoxParameterUI(this, PROPERTY_FIELD(SmoothDislocationsModifier::_smoothingEnabled));
	smoothingEnabledUI->groupBox()->setTitle(tr("Line smoothing"));
    QGridLayout* sublayout = new QGridLayout(smoothingEnabledUI->childContainer());
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setColumnStretch(1, 1);
	layout->addWidget(smoothingEnabledUI->groupBox());

	IntegerParameterUI* smoothingLevelUI = new IntegerParameterUI(this, PROPERTY_FIELD(SmoothDislocationsModifier::_smoothingLevel));
	sublayout->addWidget(smoothingLevelUI->label(), 0, 0);
	sublayout->addLayout(smoothingLevelUI->createFieldLayout(), 0, 1);

	BooleanGroupBoxParameterUI* coarseningEnabledUI = new BooleanGroupBoxParameterUI(this, PROPERTY_FIELD(SmoothDislocationsModifier::_coarseningEnabled));
	coarseningEnabledUI->groupBox()->setTitle(tr("Line coarsening"));
    sublayout = new QGridLayout(coarseningEnabledUI->childContainer());
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setColumnStretch(1, 1);
	layout->addWidget(coarseningEnabledUI->groupBox());

	FloatParameterUI* linePointIntervalUI = new FloatParameterUI(this, PROPERTY_FIELD(SmoothDislocationsModifier::_linePointInterval));
	sublayout->addWidget(linePointIntervalUI->label(), 0, 0);
	sublayout->addLayout(linePointIntervalUI->createFieldLayout(), 0, 1);
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
