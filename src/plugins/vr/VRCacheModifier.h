///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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


#include <core/Core.h>
#include <core/scene/pipeline/Modifier.h>

namespace VRPlugin {

using namespace Ovito;

/**
 * \brief A modifier that caches the results of the data pipeline.
 */
class VRCacheModifier : public Modifier
{
public:

	/// \brief Default constructor.
	Q_INVOKABLE VRCacheModifier(DataSet* dataset) : Modifier(dataset) {}

	/// \brief This modifies the input object in a specific way.
	virtual PipelineStatus modifyObject(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

	/// \brief Asks the modifier for its validity interval at the given time.
	virtual TimeInterval modifierValidity(TimePoint time) override { return TimeInterval::infinite(); }

	/// Asks the modifier whether it can be applied to the given input data.
	virtual bool isApplicableTo(const PipelineFlowState& input) override { return true; }

private:

	/// The cached state.
	PipelineFlowState _cache;

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "VR Display Cache");
	Q_CLASSINFO("ModifierCategory", "VR");

};

}	// End of namespace
