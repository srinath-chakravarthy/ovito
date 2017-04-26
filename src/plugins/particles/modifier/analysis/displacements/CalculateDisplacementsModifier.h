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
#include <plugins/particles/objects/VectorDisplay.h>
#include <core/dataset/importexport/FileImporter.h>
#include "../../ParticleModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * \brief Calculates the per-particle displacement vectors based on a reference configuration.
 */
class OVITO_PARTICLES_EXPORT CalculateDisplacementsModifier : public ParticleModifier
{
public:

	/// Constructor.
	Q_INVOKABLE CalculateDisplacementsModifier(DataSet* dataset);

protected:

	/// Handles reference events sent by reference targets of this object.
	virtual bool referenceEvent(RefTarget* source, ReferenceEvent* event) override;

	/// Modifies the particle object.
	virtual PipelineStatus modifyParticles(TimePoint time, TimeInterval& validityInterval) override;

	/// The reference configuration.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(DataObject, referenceConfiguration, setReferenceConfiguration);

	/// Controls the whether the reference configuration is shown instead of the current configuration.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, referenceShown, setReferenceShown);

	/// Controls the whether the homogeneous deformation of the simulation cell is eliminated from the calculated displacement vectors.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, eliminateCellDeformation, setEliminateCellDeformation);

	/// Controls the whether we assume the particle coordinates are unwrapped when calculating the displacement vectors.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, assumeUnwrappedCoordinates, setAssumeUnwrappedCoordinates);

	/// Specify reference frame relative to current frame.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, useReferenceFrameOffset, setUseReferenceFrameOffset);

	/// Absolute frame number from reference file to use when calculating displacement vectors.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, referenceFrameNumber, setReferenceFrameNumber);

	/// Relative frame offset for reference coordinates.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, referenceFrameOffset, setReferenceFrameOffset);

	/// The vector display object for rendering the displacement vectors.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(VectorDisplay, vectorDisplay, setVectorDisplay);
	
	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Displacement vectors");
	Q_CLASSINFO("ModifierCategory", "Analysis");
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


