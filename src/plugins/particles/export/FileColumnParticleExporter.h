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
#include "ParticleExporter.h"
#include "OutputColumnMapping.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Export)

/**
 * \brief Abstract base class for export services that can export an arbitrary list of particle properties.
 */
class OVITO_PARTICLES_EXPORT FileColumnParticleExporter : public ParticleExporter
{
protected:

	/// \brief Constructs a new instance of this class.
	FileColumnParticleExporter(DataSet* dataset) : ParticleExporter(dataset) {}

public:

	/// \brief Returns the mapping of particle properties to output file columns.
	const OutputColumnMapping& columnMapping() const { return _columnMapping; }

	/// \brief Sets the mapping of particle properties to output file columns.
	void setColumnMapping(const OutputColumnMapping& mapping) { _columnMapping = mapping; }

	/// \brief Loads the user-defined default values of this object's parameter fields from the
	///        application's settings store.
	virtual void loadUserDefaults() override;

public:

	Q_PROPERTY(Ovito::Particles::OutputColumnMapping columnMapping READ columnMapping WRITE setColumnMapping);

private:

	/// The mapping of particle properties to output file columns.
	OutputColumnMapping _columnMapping;

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


