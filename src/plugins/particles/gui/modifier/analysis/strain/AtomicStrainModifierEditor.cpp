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
#include <plugins/particles/modifier/analysis/strain/AtomicStrainModifier.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/properties/BooleanRadioButtonParameterUI.h>
#include <gui/properties/IntegerParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/SubObjectParameterUI.h>
#include "AtomicStrainModifierEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(AtomicStrainModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(AtomicStrainModifier, AtomicStrainModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void AtomicStrainModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Atomic strain"), rolloutParams, "particles.modifiers.atomic_strain.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QGridLayout* gridlayout = new QGridLayout();
	gridlayout->setContentsMargins(4,4,4,4);
	gridlayout->setColumnStretch(1, 1);

	// Cutoff parameter.
	FloatParameterUI* cutoffRadiusPUI = new FloatParameterUI(this, PROPERTY_FIELD(AtomicStrainModifier::cutoff));
	gridlayout->addWidget(cutoffRadiusPUI->label(), 0, 0);
	gridlayout->addLayout(cutoffRadiusPUI->createFieldLayout(), 0, 1);

	layout->addLayout(gridlayout);

	BooleanParameterUI* eliminateCellDeformationUI = new BooleanParameterUI(this, PROPERTY_FIELD(AtomicStrainModifier::eliminateCellDeformation));
	layout->addWidget(eliminateCellDeformationUI->checkBox());

	BooleanParameterUI* assumeUnwrappedUI = new BooleanParameterUI(this, PROPERTY_FIELD(AtomicStrainModifier::assumeUnwrappedCoordinates));
	layout->addWidget(assumeUnwrappedUI->checkBox());

#if 0
	BooleanParameterUI* showReferenceUI = new BooleanParameterUI(this, PROPERTY_FIELD(AtomicStrainModifier::referenceShown));
	layout->addWidget(showReferenceUI->checkBox());
#endif

	QCheckBox* calculateShearStrainsBox = new QCheckBox(tr("Output von Mises shear strains"));
	calculateShearStrainsBox->setEnabled(false);
	calculateShearStrainsBox->setChecked(true);
	layout->addWidget(calculateShearStrainsBox);

	QCheckBox* calculateVolumetricStrainsBox = new QCheckBox(tr("Output volumetric strains"));
	calculateVolumetricStrainsBox->setEnabled(false);
	calculateVolumetricStrainsBox->setChecked(true);
	layout->addWidget(calculateVolumetricStrainsBox);

	BooleanParameterUI* calculateDeformationGradientsUI = new BooleanParameterUI(this, PROPERTY_FIELD(AtomicStrainModifier::calculateDeformationGradients));
	layout->addWidget(calculateDeformationGradientsUI->checkBox());

	BooleanParameterUI* calculateStrainTensorsUI = new BooleanParameterUI(this, PROPERTY_FIELD(AtomicStrainModifier::calculateStrainTensors));
	layout->addWidget(calculateStrainTensorsUI->checkBox());

	BooleanParameterUI* calculateNonaffineSquaredDisplacementsUI = new BooleanParameterUI(this, PROPERTY_FIELD(AtomicStrainModifier::calculateNonaffineSquaredDisplacements));
	layout->addWidget(calculateNonaffineSquaredDisplacementsUI->checkBox());

	BooleanParameterUI* calculateRotationsUI = new BooleanParameterUI(this, PROPERTY_FIELD(AtomicStrainModifier::calculateRotations));
	layout->addWidget(calculateRotationsUI->checkBox());

	BooleanParameterUI* calculateStretchTensorsUI = new BooleanParameterUI(this, PROPERTY_FIELD(AtomicStrainModifier::calculateStretchTensors));
	layout->addWidget(calculateStretchTensorsUI->checkBox());

	BooleanParameterUI* selectInvalidParticlesUI = new BooleanParameterUI(this, PROPERTY_FIELD(AtomicStrainModifier::selectInvalidParticles));
	layout->addWidget(selectInvalidParticlesUI->checkBox());

	QGroupBox* referenceFrameGroupBox = new QGroupBox(tr("Reference frame"));
	layout->addWidget(referenceFrameGroupBox);

	QGridLayout* sublayout = new QGridLayout(referenceFrameGroupBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);
	sublayout->setColumnStretch(0, 5);
	sublayout->setColumnStretch(2, 95);

	// Add box for selection between absolute and relative reference frames.
	BooleanRadioButtonParameterUI* useFrameOffsetUI = new BooleanRadioButtonParameterUI(this, PROPERTY_FIELD(AtomicStrainModifier::useReferenceFrameOffset));
	useFrameOffsetUI->buttonTrue()->setText(tr("Relative to current frame"));
	useFrameOffsetUI->buttonFalse()->setText(tr("Fixed reference configuration"));
	sublayout->addWidget(useFrameOffsetUI->buttonFalse(), 0, 0, 1, 3);

	IntegerParameterUI* frameNumberUI = new IntegerParameterUI(this, PROPERTY_FIELD(AtomicStrainModifier::referenceFrameNumber));
	frameNumberUI->label()->setText(tr("Frame number:"));
	sublayout->addWidget(frameNumberUI->label(), 1, 1, 1, 1);
	sublayout->addLayout(frameNumberUI->createFieldLayout(), 1, 2, 1, 1);
	frameNumberUI->setEnabled(false);
	connect(useFrameOffsetUI->buttonFalse(), &QRadioButton::toggled, frameNumberUI, &IntegerParameterUI::setEnabled);

	sublayout->addWidget(useFrameOffsetUI->buttonTrue(), 2, 0, 1, 3);
	IntegerParameterUI* frameOffsetUI = new IntegerParameterUI(this, PROPERTY_FIELD(AtomicStrainModifier::referenceFrameOffset));
	frameOffsetUI->label()->setText(tr("Frame offset:"));
	sublayout->addWidget(frameOffsetUI->label(), 3, 1, 1, 1);
	sublayout->addLayout(frameOffsetUI->createFieldLayout(), 3, 2, 1, 1);
	frameOffsetUI->setEnabled(false);
	connect(useFrameOffsetUI->buttonTrue(), &QRadioButton::toggled, frameOffsetUI, &IntegerParameterUI::setEnabled);

	// Status label.
	layout->addSpacing(6);
	layout->addWidget(statusLabel());

	// Open a sub-editor for the reference object.
	new SubObjectParameterUI(this, PROPERTY_FIELD(AtomicStrainModifier::referenceConfiguration), RolloutInsertionParameters().setTitle(tr("Reference")));
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
