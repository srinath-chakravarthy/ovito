///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2015) Alexander Stukowski
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

#include <plugins/particles/Particles.h>
#include <core/gui/properties/ColorParameterUI.h>
#include <core/gui/properties/FloatParameterUI.h>
#include <core/gui/properties/StringParameterUI.h>

#include "BondType.h"

namespace Ovito { namespace Particles {

OVITO_BEGIN_INLINE_NAMESPACE(Internal)
	IMPLEMENT_OVITO_OBJECT(Particles, BondTypeEditor, PropertiesEditor);
OVITO_END_INLINE_NAMESPACE

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(Particles, BondType, RefTarget);
SET_OVITO_OBJECT_EDITOR(BondType, BondTypeEditor);
DEFINE_PROPERTY_FIELD(BondType, _id, "Identifier");
DEFINE_PROPERTY_FIELD(BondType, _color, "Color");
DEFINE_PROPERTY_FIELD(BondType, _radius, "Radius");
DEFINE_PROPERTY_FIELD(BondType, _name, "Name");
SET_PROPERTY_FIELD_LABEL(BondType, _id, "Id");
SET_PROPERTY_FIELD_LABEL(BondType, _color, "Color");
SET_PROPERTY_FIELD_LABEL(BondType, _radius, "Radius");
SET_PROPERTY_FIELD_LABEL(BondType, _name, "Name");
SET_PROPERTY_FIELD_UNITS(BondType, _radius, WorldParameterUnit);

/******************************************************************************
* Constructs a new BondType.
******************************************************************************/
BondType::BondType(DataSet* dataset) : RefTarget(dataset), _color(1,1,1), _radius(0), _id(0)
{
	INIT_PROPERTY_FIELD(BondType::_id);
	INIT_PROPERTY_FIELD(BondType::_color);
	INIT_PROPERTY_FIELD(BondType::_radius);
	INIT_PROPERTY_FIELD(BondType::_name);
}

OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void BondTypeEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Bond Type"), rolloutParams);

    // Create the rollout contents.
	QGridLayout* layout1 = new QGridLayout(rollout);
	layout1->setContentsMargins(4,4,4,4);
#ifndef Q_OS_MACX
	layout1->setSpacing(0);
#endif
	layout1->setColumnStretch(1, 1);
	
	// Text box for the name of particle type.
	StringParameterUI* namePUI = new StringParameterUI(this, PROPERTY_FIELD(BondType::_name));
	layout1->addWidget(new QLabel(tr("Name:")), 0, 0);
	layout1->addWidget(namePUI->textBox(), 0, 1);
	
	// Display color parameter.
	ColorParameterUI* colorPUI = new ColorParameterUI(this, PROPERTY_FIELD(BondType::_color));
	layout1->addWidget(colorPUI->label(), 1, 0);
	layout1->addWidget(colorPUI->colorPicker(), 1, 1);
}

OVITO_END_INLINE_NAMESPACE

}	// End of namespace
}	// End of namespace
