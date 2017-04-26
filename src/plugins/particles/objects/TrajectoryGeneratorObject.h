///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2015) Alexander Stukowski
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
#include "TrajectoryObject.h"

namespace Ovito { namespace Particles {

/**
 * \brief Generates trajectory data from a particles object.
 */
class OVITO_PARTICLES_EXPORT TrajectoryGeneratorObject : public TrajectoryObject
{
public:

	/// \brief Constructor.
	Q_INVOKABLE TrajectoryGeneratorObject(DataSet* dataset);

	/// Returns the the custom time interval.
	TimeInterval customInterval() const { return TimeInterval(_customIntervalStart, _customIntervalEnd); }

	/// Updates the stored trajectories from the source particle object.
	bool generateTrajectories(TaskManager& taskManager);

private:

	/// The object node providing the input particles.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(ObjectNode, source, setSource);

	/// Controls which particles trajectories are created for.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, onlySelectedParticles, setOnlySelectedParticles);

	/// Controls whether the created trajectories span the entire animation interval or a sub-interval.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, useCustomInterval, setUseCustomInterval);

	/// The start of the custom time interval.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(TimePoint, customIntervalStart, setCustomIntervalStart);

	/// The end of the custom time interval.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(TimePoint, customIntervalEnd, setCustomIntervalEnd);

	/// The sampling frequency for creating trajectories.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, everyNthFrame, setEveryNthFrame);

	/// Controls whether trajectories are unwrapped when crossing periodic boundaries.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, unwrapTrajectories, setUnwrapTrajectories);

	Q_OBJECT
	OVITO_OBJECT
};

}	// End of namespace
}	// End of namespace


