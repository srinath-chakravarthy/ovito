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

#pragma once


#include <plugins/particles/Particles.h>
#include <core/animation/controller/Controller.h>
#include <core/animation/AnimationSettings.h>
#include "../ParticleModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Coloring)

/**
 * \brief This modifier assigns a certain color to all selected particles.
 */
class OVITO_PARTICLES_EXPORT AssignColorModifier : public ParticleModifier
{
public:

	/// Constructor.
	Q_INVOKABLE AssignColorModifier(DataSet* dataset);

	/// Asks the modifier for its validity interval at the given time.
	virtual TimeInterval modifierValidity(TimePoint time) override;

	/// Returns the color that is assigned to the selected atoms.
	Color color() const { return colorController() ? colorController()->currentColorValue() : Color(0,0,0); }

	/// Sets the color that is assigned to the selected atoms.
	void setColor(const Color& color) { if(colorController()) colorController()->setCurrentColorValue(color); }

protected:

	/// Modifies the particles.
	virtual PipelineStatus modifyParticles(TimePoint time, TimeInterval& validityInterval) override;

	/// This controller stores the constant color to be assigned to all atoms.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(Controller, colorController, setColorController);

	/// Controls whether the input particle selection is preserved.
	/// If false, the selection is cleared by the modifier.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, keepSelection, setKeepSelection);

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Assign color");
	Q_CLASSINFO("ModifierCategory", "Coloring");
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


