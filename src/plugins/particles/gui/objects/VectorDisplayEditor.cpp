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
#include <plugins/particles/objects/VectorDisplay.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/VariantComboBoxParameterUI.h>
#include <gui/properties/ColorParameterUI.h>
#include <gui/properties/BooleanParameterUI.h>
#include "VectorDisplayEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(ParticlesGui, VectorDisplayEditor, PropertiesEditor);
SET_OVITO_OBJECT_EDITOR(VectorDisplay, VectorDisplayEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void VectorDisplayEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Vector display"), rolloutParams, "display_objects.vectors.html");

    // Create the rollout contents.
	QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1, 1);
	int row = 0;

	// Shading mode.
	VariantComboBoxParameterUI* shadingModeUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(VectorDisplay::_shadingMode));
	shadingModeUI->comboBox()->addItem(tr("Normal"), QVariant::fromValue(ArrowPrimitive::NormalShading));
	shadingModeUI->comboBox()->addItem(tr("Flat"), QVariant::fromValue(ArrowPrimitive::FlatShading));
	layout->addWidget(new QLabel(tr("Shading mode:")), row, 0);
	layout->addWidget(shadingModeUI->comboBox(), row++, 1);

#if 0
	// Rendering quality.
	VariantComboBoxParameterUI* renderingQualityUI = new VariantComboBoxParameterUI(this, "renderingQuality");
	renderingQualityUI->comboBox()->addItem(tr("Low"), qVariantFromValue(ArrowPrimitive::LowQuality));
	renderingQualityUI->comboBox()->addItem(tr("Medium"), qVariantFromValue(ArrowPrimitive::MediumQuality));
	renderingQualityUI->comboBox()->addItem(tr("High"), qVariantFromValue(ArrowPrimitive::HighQuality));
	layout->addWidget(new QLabel(tr("Rendering quality:")), row, 0);
	layout->addWidget(renderingQualityUI->comboBox(), row++, 1);
#endif

	// Scaling factor.
	FloatParameterUI* scalingFactorUI = new FloatParameterUI(this, PROPERTY_FIELD(VectorDisplay::_scalingFactor));
	layout->addWidget(scalingFactorUI->label(), row, 0);
	layout->addLayout(scalingFactorUI->createFieldLayout(), row++, 1);
	scalingFactorUI->setMinValue(0);

	// Arrow width factor.
	FloatParameterUI* arrowWidthUI = new FloatParameterUI(this, PROPERTY_FIELD(VectorDisplay::_arrowWidth));
	layout->addWidget(arrowWidthUI->label(), row, 0);
	layout->addLayout(arrowWidthUI->createFieldLayout(), row++, 1);
	arrowWidthUI->setMinValue(0);

	VariantComboBoxParameterUI* arrowPositionUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(VectorDisplay::_arrowPosition));
	arrowPositionUI->comboBox()->addItem(QIcon(":/particles/icons/arrow_alignment_base.png"), tr("Base"), QVariant::fromValue(VectorDisplay::Base));
	arrowPositionUI->comboBox()->addItem(QIcon(":/particles/icons/arrow_alignment_center.png"), tr("Center"), QVariant::fromValue(VectorDisplay::Center));
	arrowPositionUI->comboBox()->addItem(QIcon(":/particles/icons/arrow_alignment_head.png"), tr("Head"), QVariant::fromValue(VectorDisplay::Head));
	layout->addWidget(new QLabel(tr("Alignment:")), row, 0);
	layout->addWidget(arrowPositionUI->comboBox(), row++, 1);

	ColorParameterUI* arrowColorUI = new ColorParameterUI(this, PROPERTY_FIELD(VectorDisplay::_arrowColor));
	layout->addWidget(arrowColorUI->label(), row, 0);
	layout->addWidget(arrowColorUI->colorPicker(), row++, 1);

	BooleanParameterUI* reverseArrowDirectionUI = new BooleanParameterUI(this, PROPERTY_FIELD(VectorDisplay::_reverseArrowDirection));
	layout->addWidget(reverseArrowDirectionUI->checkBox(), row++, 1, 1, 1);
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
