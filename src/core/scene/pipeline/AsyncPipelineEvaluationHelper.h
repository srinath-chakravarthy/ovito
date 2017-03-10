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
#include <core/scene/pipeline/PipelineEvalRequest.h>
#include <core/scene/pipeline/PipelineFlowState.h>
#include <core/utilities/concurrent/Future.h>
#include <core/utilities/concurrent/Promise.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/**
 * \brief A helper class that manages requests for asynchronous pipeline evaluations
 *        and the corresponding promises issued by a DataObject.
 */
class OVITO_CORE_EXPORT AsyncPipelineEvaluationHelper
{
public:

	/// Creates a Future/Promise for a pipeline evaluation request.
	Future<PipelineFlowState> createRequest(DataObject* owner, const PipelineEvalRequest& request);

	/// Checks if the data pipeline evaluation is completed and pending requests can be served.
	void serveRequests(DataObject* owner);

private:

	/// Checks if the data pipeline evaluation is completed and pending requests can be served.
	void serveRequestsDeferred(DataObject* owner);

	/// List of pending pipeline requests and their corresponding promises.
	std::vector<std::pair<PipelineEvalRequest, PromisePtr<PipelineFlowState>>> _requests;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


