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
#include <plugins/particles/gui/modifier/ParticleModifierEditor.h>
#include <plugins/particles/gui/util/ParticlePickingHelper.h>
#include <plugins/particles/util/ParticleSelectionSet.h>
#include <gui/viewport/input/ViewportInputMode.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Selection) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * A properties editor for the ManualSelectionModifier class.
 */
class ManualSelectionModifierEditor : public ParticleModifierEditor
{
public:

	/// Default constructor
	Q_INVOKABLE ManualSelectionModifierEditor() {}

	/// This is called when the user has selected a particle.
	void onParticlePicked(const ParticlePickingHelper::PickResult& pickResult);

	/// This is called when the user has drawn a fence around particles.
	void onFence(const QVector<Point2>& fence, Viewport* viewport, ParticleSelectionSet::SelectionMode mode);

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

protected Q_SLOTS:

	/// Adopts the selection state from the modifier's input.
	void resetSelection();

	/// Selects all particles
	void selectAll();

	/// Clears the selection.
	void clearSelection();

private:

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


