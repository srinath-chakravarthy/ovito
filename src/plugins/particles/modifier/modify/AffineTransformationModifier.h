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
 * \brief This modifier applies an arbitrary affine transformation to the
 *        particles and/or the simulation box.
 *
 * The affine transformation is given by a 3x4 matrix.
 */
class OVITO_PARTICLES_EXPORT AffineTransformationModifier : public ParticleModifier
{
public:

	/// \brief Constructor.
	Q_INVOKABLE AffineTransformationModifier(DataSet* dataset);

protected:

	/// \brief This virtual method is called by the system when the modifier has been inserted into a PipelineObject.
	virtual void initializeModifier(PipelineObject* pipeline, ModifierApplication* modApp) override;

	/// Modifies the particle object.
	virtual PipelineStatus modifyParticles(TimePoint time, TimeInterval& validityInterval) override;

	/// This property fields stores the transformation matrix (used in 'relative' mode).
	DECLARE_MODIFIABLE_PROPERTY_FIELD(AffineTransformation, transformationTM, setTransformationTM);

	/// This property fields stores the simulation cell geometry (used in 'absolute' mode).
	DECLARE_MODIFIABLE_PROPERTY_FIELD(AffineTransformation, targetCell, setTargetCell);

	/// This controls whether the transformation is applied to the particles.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, applyToParticles, setApplyToParticles);

	/// This controls whether the transformation is applied only to the selected particles.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, selectionOnly, setSelectionOnly);

	/// This controls whether the transformation is applied to the simulation box.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, applyToSimulationBox, setApplyToSimulationBox);

	/// This controls whether a relative transformation is applied to the simulation box or
	/// the absolute cell geometry has been specified.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, relativeMode, setRelativeMode);

	/// This controls whether the transformation is applied to surface meshes.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, applyToSurfaceMesh, setApplyToSurfaceMesh);

	/// This controls whether the transformation is applied to vector particle and bond properties.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, applyToVectorProperties, setApplyToVectorProperties);

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Affine transformation");
	Q_CLASSINFO("ModifierCategory", "Modification");
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
