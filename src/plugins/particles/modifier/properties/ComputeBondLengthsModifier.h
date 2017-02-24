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


#include <plugins/particles/Particles.h>
#include "../ParticleModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Properties)

/**
 * \brief This modifier computes the length of bonds and stores them in a new bond property.
 */
class OVITO_PARTICLES_EXPORT ComputeBondLengthsModifier : public ParticleModifier
{
public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE ComputeBondLengthsModifier(DataSet* dataset);

	/// \brief Asks the modifier for its validity interval at the given time.
	virtual TimeInterval modifierValidity(TimePoint time) override { return TimeInterval::infinite(); }

protected:

	/// Modifies the particle object. The time interval passed
	/// to the function is reduced to the interval where the modified object is valid/constant.
	virtual PipelineStatus modifyParticles(TimePoint time, TimeInterval& validityInterval) override;

private:

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Compute bond lengths");
	Q_CLASSINFO("ModifierCategory", "Modification");
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


