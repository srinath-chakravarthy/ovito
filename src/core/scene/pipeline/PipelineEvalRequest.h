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
#include <core/animation/TimeInterval.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/**
 * \brief Describes when and how the data pipeline should be evaluated.
 */
class OVITO_CORE_EXPORT PipelineEvalRequest
{
public:

	/// \brief Constructor
	explicit PipelineEvalRequest(TimePoint time, bool prepareDisplayObjects, ModifierApplication* upToThisModifier = nullptr, bool includeLastModifier = false) : 
			_time(time), _prepareDisplayObjects(prepareDisplayObjects), _upToThisModifier(upToThisModifier), _includeLastModifier(includeLastModifier) {}

	/// Returns the animation time at which the pipeline shoud be evaluated.
	TimePoint time() const { return _time; }

	/// Indicates whether display objects should be prepared for rendering.
	bool prepareDisplayObjects() const { return _prepareDisplayObjects; }

	/// Indicates whether the pipeline should be evaluated only up to a certain modifier.
	ModifierApplication* upToThisModifier() const { return _upToThisModifier; }

	/// If a partial pipeline evaluation up to a certain modifier is requested, then this indicates whether 
	/// that modifier should be included in the results.
	bool includeLastModifier() const { return _includeLastModifier; }

	/// Compares two requests for equality.
	bool operator==(const PipelineEvalRequest& other) const {
		return (time() == other.time() && prepareDisplayObjects() == other.prepareDisplayObjects() && 
			upToThisModifier() == other.upToThisModifier() && includeLastModifier() == other.includeLastModifier());
	}

private:

	/// The animation time at which the pipeline shoud be evaluated.
	TimePoint _time;

	/// Indicates whether display objects should be prepared for rendering.
	bool _prepareDisplayObjects;

	/// Used to indicate that the pipeline should be evaluated only up to this modifier.
	ModifierApplication* _upToThisModifier;

	/// When requesting a partial pipeline evaluation, indicates whether the last modifier
	/// should be included in the result.
	bool _includeLastModifier;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


