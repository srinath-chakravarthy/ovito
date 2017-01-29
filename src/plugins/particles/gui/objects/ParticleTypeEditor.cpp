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

#include <plugins/particles/gui/ParticlesGui.h>
#include <plugins/particles/objects/ParticleType.h>
#include <plugins/particles/objects/ParticleTypeProperty.h>
#include <plugins/particles/data/ParticleProperty.h>
#include <gui/properties/ColorParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/StringParameterUI.h>
#include <gui/mainwin/MainWindow.h>
#include "ParticleTypeEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(ParticleTypeEditor, PropertiesEditor);
SET_OVITO_OBJECT_EDITOR(ParticleType, ParticleTypeEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void ParticleTypeEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Atom Type"), rolloutParams);

    // Create the rollout contents.
	QGridLayout* layout1 = new QGridLayout(rollout);
	layout1->setContentsMargins(4,4,4,4);
#ifndef Q_OS_MACX
	layout1->setSpacing(0);
#endif
	layout1->setColumnStretch(1, 1);
	
	// Text box for the name of particle type.
	StringParameterUI* namePUI = new StringParameterUI(this, PROPERTY_FIELD(ParticleType::name));
	layout1->addWidget(new QLabel(tr("Name:")), 0, 0);
	layout1->addWidget(namePUI->textBox(), 0, 1);
	
	// Display color parameter.
	ColorParameterUI* colorPUI = new ColorParameterUI(this, PROPERTY_FIELD(ParticleType::color));
	layout1->addWidget(colorPUI->label(), 1, 0);
	layout1->addWidget(colorPUI->colorPicker(), 1, 1);
   
	// Display radius parameter.
	FloatParameterUI* radiusPUI = new FloatParameterUI(this, PROPERTY_FIELD(ParticleType::radius));
	layout1->addWidget(radiusPUI->label(), 2, 0);
	layout1->addLayout(radiusPUI->createFieldLayout(), 2, 1);

	// "Set as default" button
	QPushButton* setAsDefaultBtn = new QPushButton(tr("Set as default"));
	setAsDefaultBtn->setToolTip(tr("Set current color and radius as defaults for this particle type."));
	setAsDefaultBtn->setEnabled(false);
	layout1->addWidget(setAsDefaultBtn, 3, 0, 1, 2, Qt::AlignRight);
	connect(setAsDefaultBtn, &QPushButton::clicked, [this]() {
		ParticleType* ptype = static_object_cast<ParticleType>(editObject());
		if(!ptype) return;

		ParticleTypeProperty::setDefaultParticleColor(ParticleProperty::ParticleTypeProperty, ptype->name(), ptype->color());
		ParticleTypeProperty::setDefaultParticleRadius(ParticleProperty::ParticleTypeProperty, ptype->name(), ptype->radius());

		mainWindow()->statusBar()->showMessage(tr("Stored current color and radius as defaults for particle type '%1'.").arg(ptype->name()), 4000);
	});
	connect(this, &PropertiesEditor::contentsReplaced, [setAsDefaultBtn](RefTarget* newEditObject) {
		setAsDefaultBtn->setEnabled(newEditObject != nullptr);
	});
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
