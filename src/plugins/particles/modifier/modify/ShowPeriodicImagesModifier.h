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
#include "../ParticleModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

/**
 * \brief This modifier duplicates all atoms multiple times and shifts them by
 *        one of the simulation cell vectors to visualize the periodic images.
 *
 * \author Alexander Stukowski
 */
class OVITO_PARTICLES_EXPORT ShowPeriodicImagesModifier : public ParticleModifier
{
public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE ShowPeriodicImagesModifier(DataSet* dataset);

protected:

	/// Modifies the particle object. The time interval passed
	/// to the function is reduced to the interval where the modified object is valid/constant.
	virtual PipelineStatus modifyParticles(TimePoint time, TimeInterval& validityInterval) override;

private:

	/// Controls whether the periodic images are shown in the X direction.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, showImageX, setShowImageX);
	/// Controls whether the periodic images are shown in the Y direction.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, showImageY, setShowImageY);
	/// Controls whether the periodic images are shown in the Z direction.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, showImageZ, setShowImageZ);

	/// Controls the number of periodic images shown in the X direction.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, numImagesX, setNumImagesX);
	/// Controls the number of periodic images shown in the Y direction.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, numImagesY, setNumImagesY);
	/// Controls the number of periodic images shown in the Z direction.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, numImagesZ, setNumImagesZ);

	/// Controls whether the size of the simulation box is adjusted to the extended system.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, adjustBoxSize, setAdjustBoxSize);

	/// Controls whether the modifier assigns unique identifiers to particle copies.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, uniqueIdentifiers, setUniqueIdentifiers);

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Show periodic images");
	Q_CLASSINFO("ModifierCategory", "Modification");
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


