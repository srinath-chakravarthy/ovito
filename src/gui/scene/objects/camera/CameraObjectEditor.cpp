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

#include <gui/GUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/BooleanRadioButtonParameterUI.h>
#include <gui/properties/VariantComboBoxParameterUI.h>
#include <core/scene/objects/camera/CameraObject.h>
#include "CameraObjectEditor.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene) OVITO_BEGIN_INLINE_NAMESPACE(StdObj) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(Gui, CameraObjectEditor, PropertiesEditor);
SET_OVITO_OBJECT_EDITOR(CameraObject, CameraObjectEditor);

/******************************************************************************
* Constructor that creates the UI controls for the editor.
******************************************************************************/
void CameraObjectEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create the rollout.
	QWidget* rollout = createRollout(tr("Camera"), rolloutParams);

	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(2);

	QGridLayout* sublayout = new QGridLayout();
	sublayout->setContentsMargins(0,0,0,0);
	sublayout->setColumnStretch(2, 1);
	sublayout->setColumnMinimumWidth(0, 12);
	layout->addLayout(sublayout);

	// Camera projection parameter.
	BooleanRadioButtonParameterUI* isPerspectivePUI = new BooleanRadioButtonParameterUI(this, PROPERTY_FIELD(CameraObject::_isPerspective));
	isPerspectivePUI->buttonTrue()->setText(tr("Perspective camera:"));
	sublayout->addWidget(isPerspectivePUI->buttonTrue(), 0, 0, 1, 3);

	// FOV parameter.
	FloatParameterUI* fovPUI = new FloatParameterUI(this, PROPERTY_FIELD(CameraObject::_fov));
	sublayout->addWidget(fovPUI->label(), 1, 1);
	sublayout->addLayout(fovPUI->createFieldLayout(), 1, 2);
	fovPUI->setMinValue(1e-3f);
	fovPUI->setMaxValue(FLOATTYPE_PI - 1e-2f);

	isPerspectivePUI->buttonFalse()->setText(tr("Orthographic camera:"));
	sublayout->addWidget(isPerspectivePUI->buttonFalse(), 2, 0, 1, 3);

	// Zoom parameter.
	FloatParameterUI* zoomPUI = new FloatParameterUI(this, PROPERTY_FIELD(CameraObject::_zoom));
	sublayout->addWidget(zoomPUI->label(), 3, 1);
	sublayout->addLayout(zoomPUI->createFieldLayout(), 3, 2);
	zoomPUI->setMinValue(0);

	fovPUI->setEnabled(false);
	zoomPUI->setEnabled(false);
	connect(isPerspectivePUI->buttonTrue(), &QRadioButton::toggled, fovPUI, &FloatParameterUI::setEnabled);
	connect(isPerspectivePUI->buttonFalse(), &QRadioButton::toggled, zoomPUI, &FloatParameterUI::setEnabled);

	// Camera type.
	layout->addSpacing(10);
	VariantComboBoxParameterUI* typePUI = new VariantComboBoxParameterUI(this, "isTargetCamera");
	typePUI->comboBox()->addItem(tr("Free camera"), QVariant::fromValue(false));
	typePUI->comboBox()->addItem(tr("Target camera"), QVariant::fromValue(true));
	layout->addWidget(new QLabel(tr("Camera type:")));
	layout->addWidget(typePUI->comboBox());
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
