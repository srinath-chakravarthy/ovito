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
#include <core/scene/pipeline/Modifier.h>
#include "ParticleModifierEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers)

IMPLEMENT_OVITO_OBJECT(ParticlesGui, ParticleModifierEditor, PropertiesEditor);

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool ParticleModifierEditor::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	if(source == editObject() && event->type() == ReferenceEvent::ObjectStatusChanged) {
		updateStatusLabel();
	}
	return PropertiesEditor::referenceEvent(source, event);
}

/******************************************************************************
* Updates the text of the result label.
******************************************************************************/
void ParticleModifierEditor::updateStatusLabel()
{
	if(!_statusLabel)
		return;

	if(Modifier* modifier = dynamic_object_cast<Modifier>(editObject()))
		_statusLabel->setStatus(modifier->status());
	else
		_statusLabel->clearStatus();
}

/******************************************************************************
* Returns a widget that displays a message sent by the modifier that
* states the outcome of the modifier evaluation. Derived classes of this
* editor base class can add the widget to their user interface.
******************************************************************************/
StatusWidget* ParticleModifierEditor::statusLabel()
{
	if(!_statusLabel)
		_statusLabel = new StatusWidget();
	return _statusLabel;
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace

