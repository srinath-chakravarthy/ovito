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
#include <gui/properties/ColorParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <core/scene/objects/geometry/TriMeshDisplay.h>
#include "TriMeshDisplayEditor.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene) OVITO_BEGIN_INLINE_NAMESPACE(StdObj) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(TriMeshDisplayEditor, PropertiesEditor);
SET_OVITO_OBJECT_EDITOR(TriMeshDisplay, TriMeshDisplayEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void TriMeshDisplayEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Mesh display"), rolloutParams);

    // Create the rollout contents.
	QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1, 1);

	ColorParameterUI* colorUI = new ColorParameterUI(this, PROPERTY_FIELD(TriMeshDisplay::_color));
	layout->addWidget(colorUI->label(), 0, 0);
	layout->addWidget(colorUI->colorPicker(), 0, 1);

	FloatParameterUI* transparencyUI = new FloatParameterUI(this, PROPERTY_FIELD(TriMeshDisplay::_transparency));
	layout->addWidget(new QLabel(tr("Transparency:")), 1, 0);
	layout->addLayout(transparencyUI->createFieldLayout(), 1, 1);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
