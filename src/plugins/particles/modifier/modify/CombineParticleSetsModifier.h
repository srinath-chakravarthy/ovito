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
#include <plugins/particles/modifier/ParticleModifier.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

/**
 * \brief Combines two particle datasets into one.
 */
class OVITO_PARTICLES_EXPORT CombineParticleSetsModifier : public ParticleModifier
{
public:

	/// Constructor.
	Q_INVOKABLE CombineParticleSetsModifier(DataSet* dataset);

protected:

	/// Modifies the particle object.
	virtual PipelineStatus modifyParticles(TimePoint time, TimeInterval& validityInterval) override;

	/// The source for particle data to be merged into the pipeline.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(DataObject, secondaryDataSource, setSecondaryDataSource);

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Combine particle sets");
	Q_CLASSINFO("ModifierCategory", "Modification");
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


