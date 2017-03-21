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

#pragma once


#include <plugins/particles/gui/ParticlesGui.h>
#include <gui/properties/PropertiesEditor.h>
#include <gui/widgets/display/StatusWidget.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers)

/**
 * \brief Base class for properties editors for ParticleModifier derived classes.
 */
class OVITO_PARTICLES_GUI_EXPORT ParticleModifierEditor : public PropertiesEditor
{
public:

	/// Constructor.
	ParticleModifierEditor() {
		connect(this, &ParticleModifierEditor::contentsReplaced, this, &ParticleModifierEditor::updateStatusLabel);
	}

	/// Returns a widget that displays a message sent by the modifier that
	/// states the outcome of the modifier evaluation. Derived classes of this
	/// editor base class can add the widget to their user interface.
	StatusWidget* statusLabel();

protected:

	/// This method is called when a reference target changes.
	virtual bool referenceEvent(RefTarget* source, ReferenceEvent* event) override;

private Q_SLOTS:

	/// Updates the text of the result label.
	void updateStatusLabel();

private:

	QPointer<StatusWidget> _statusLabel;

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


