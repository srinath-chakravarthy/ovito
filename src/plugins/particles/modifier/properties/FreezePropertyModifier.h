///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2014) Alexander Stukowski
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
 * \brief Saves the current state of a particle property and preserves it over time.
 */
class OVITO_PARTICLES_EXPORT FreezePropertyModifier : public ParticleModifier
{
public:

	/// Constructor.
	Q_INVOKABLE FreezePropertyModifier(DataSet* dataset);

	/// Asks the modifier for its validity interval at the given time.
	virtual TimeInterval modifierValidity(TimePoint time) override { return TimeInterval::infinite(); }

	/// Takes a snapshot of the source property for a specific ModifierApplication of this modifier.
	void takePropertySnapshot(ModifierApplication* modApp, const PipelineFlowState& state);

	/// Takes a snapshot of the source property for every ModifierApplication of this modifier.
	bool takePropertySnapshot(TimePoint time, TaskManager& taskManager, bool waitUntilReady);

protected:

	/// This virtual method is called by the modification system when the modifier is being inserted into a PipelineObject.
	virtual void initializeModifier(PipelineObject* pipelineObject, ModifierApplication* modApp) override;

	/// Modifies the particle object.
	virtual PipelineStatus modifyParticles(TimePoint time, TimeInterval& validityInterval) override;

private:

	/// The particle property that is preserved by this modifier.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(ParticlePropertyReference, sourceProperty, setSourceProperty);

	/// The particle property to which the stored values should be written
	DECLARE_MODIFIABLE_PROPERTY_FIELD(ParticlePropertyReference, destinationProperty, setDestinationProperty);

	/// The cached display objects that are attached to the output particle property.
	DECLARE_VECTOR_REFERENCE_FIELD(DisplayObject, cachedDisplayObjects);

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Freeze property");
	Q_CLASSINFO("ModifierCategory", "Modification");
};

OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * Helper class used by the FreezePropertyModifier to store the values of
 * the selected particle property.
 */
class OVITO_PARTICLES_EXPORT SavedParticleProperty : public RefTarget
{
public:

	/// Constructor.
	Q_INVOKABLE SavedParticleProperty(DataSet* dataset) : RefTarget(dataset) {
		INIT_PROPERTY_FIELD(property);
		INIT_PROPERTY_FIELD(identifiers);
	}

	/// Makes a copy of the given source property and, optionally, of the provided
	/// particle identifier list, which will allow to restore the saved property
	/// values even if the order of particles changes.
	void reset(ParticlePropertyObject* property, ParticlePropertyObject* identifiers);

private:

	/// The stored copy of the particle property.
	DECLARE_REFERENCE_FIELD(ParticlePropertyObject, property);

	/// A copy of the particle identifiers, taken at the time when the property values were saved.
	DECLARE_REFERENCE_FIELD(ParticlePropertyObject, identifiers);

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


