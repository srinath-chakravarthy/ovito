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

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Selection)

/**
 * \brief Selects particles of one or more types.
 */
class OVITO_PARTICLES_EXPORT SelectParticleTypeModifier : public ParticleModifier
{
public:

	/// Constructor.
	Q_INVOKABLE SelectParticleTypeModifier(DataSet* dataset) : ParticleModifier(dataset),
		_sourceProperty(ParticleProperty::ParticleTypeProperty) {
		INIT_PROPERTY_FIELD(sourceProperty);
		INIT_PROPERTY_FIELD(selectedParticleTypes);
	}

	/// Loads the user-defined default values of this object's parameter fields from the
	/// application's settings store.
	virtual void loadUserDefaults() override;

	/// Sets a single particle type identifier to be selected.
	void setSelectedParticleType(int type) { setSelectedParticleTypes(QSet<int>{type}); }

protected:

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

	/// This virtual method is called by the system when the modifier has been inserted into a PipelineObject.
	virtual void initializeModifier(PipelineObject* pipelineObject, ModifierApplication* modApp) override;

	/// Modifies the particle object.
	virtual PipelineStatus modifyParticles(TimePoint time, TimeInterval& validityInterval) override;

	/// The particle type property that is used as source for the selection.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(ParticlePropertyReference, sourceProperty, setSourceProperty);

	/// The identifiers of the particle types to select.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QSet<int>, selectedParticleTypes, setSelectedParticleTypes);

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Select particle type");
	Q_CLASSINFO("ModifierCategory", "Selection");
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


