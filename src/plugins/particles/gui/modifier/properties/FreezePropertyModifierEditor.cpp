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
#include <plugins/particles/modifier/properties/FreezePropertyModifier.h>
#include <plugins/particles/gui/util/ParticlePropertyParameterUI.h>
#include <core/animation/AnimationSettings.h>
#include <core/dataset/DataSetContainer.h>
#include <gui/utilities/concurrent/ProgressDialog.h>
#include "FreezePropertyModifierEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Properties) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(FreezePropertyModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(FreezePropertyModifier, FreezePropertyModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void FreezePropertyModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	QWidget* rollout = createRollout(tr("Freeze property"), rolloutParams, "particles.modifiers.freeze_property.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(2);

	ParticlePropertyParameterUI* sourcePropertyUI = new ParticlePropertyParameterUI(this, PROPERTY_FIELD(FreezePropertyModifier::sourceProperty), false, true);
	layout->addWidget(new QLabel(tr("Property to freeze:"), rollout));
	layout->addWidget(sourcePropertyUI->comboBox());
	connect(sourcePropertyUI, &ParticlePropertyParameterUI::valueEntered, this, &FreezePropertyModifierEditor::onSourcePropertyChanged);
	layout->addSpacing(8);

	ParticlePropertyParameterUI* destPropertyUI = new ParticlePropertyParameterUI(this, PROPERTY_FIELD(FreezePropertyModifier::destinationProperty), false, false);
	layout->addWidget(new QLabel(tr("Output property:"), rollout));
	layout->addWidget(destPropertyUI->comboBox());
	layout->addSpacing(8);

	QPushButton* takeSnapshotBtn = new QPushButton(tr("Take new snapshot"), rollout);
	connect(takeSnapshotBtn, &QPushButton::clicked, this, &FreezePropertyModifierEditor::takeSnapshot);
	layout->addWidget(takeSnapshotBtn);

	// Status label.
	layout->addSpacing(12);
	layout->addWidget(statusLabel());
}

/******************************************************************************
* Takes a new snapshot of the current property values.
******************************************************************************/
void FreezePropertyModifierEditor::takeSnapshot()
{
	FreezePropertyModifier* mod = static_object_cast<FreezePropertyModifier>(editObject());
	if(!mod) return;

	undoableTransaction(tr("Take property snapshot"), [this,mod]() {
		ProgressDialog progressDialog(container(), mod->dataset()->container()->taskManager(), tr("Property snapshot"));
		mod->takePropertySnapshot(mod->dataset()->animationSettings()->time(), progressDialog.taskManager(), true);
	});
}

/******************************************************************************
* Is called when the user has selected a different source property.
******************************************************************************/
void FreezePropertyModifierEditor::onSourcePropertyChanged()
{
	FreezePropertyModifier* mod = static_object_cast<FreezePropertyModifier>(editObject());
	if(!mod) return;

	undoableTransaction(tr("Freeze property"), [this,mod]() {
		// When the user selects a different source property, adjust the destination property automatically.
		mod->setDestinationProperty(mod->sourceProperty());
		// Also take a current snapshot of the source property values.
		ProgressDialog progressDialog(container(), mod->dataset()->container()->taskManager());
		mod->takePropertySnapshot(mod->dataset()->animationSettings()->time(), progressDialog.taskManager(), true);
	});
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
