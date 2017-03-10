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

#include <core/Core.h>
#include <core/scene/objects/DataObject.h>
#include <core/app/Application.h>
#include "AsyncPipelineEvaluationHelper.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/******************************************************************************
* Creates a Future for a pipeline evaluation request.
******************************************************************************/
Future<PipelineFlowState> AsyncPipelineEvaluationHelper::createRequest(DataObject* owner, const PipelineEvalRequest& request)
{
	// Check if there is already an active request pending that is identical.
	for(const auto& req : _requests) {
		if(req.first == request)
			return Future<PipelineFlowState>(req.second);
	}

	// Check if we can satisfy the request immediately.
	if(_requests.empty()) {
		const PipelineFlowState& state = owner->evaluateImmediately(request);
		if(state.status().type() != PipelineStatus::Pending)
			return Future<PipelineFlowState>::createImmediate(state);
	}

	// Create a new record for this evaluation request.
	auto future = Future<PipelineFlowState>::createWithPromise();
	_requests.emplace_back(request, future.promise());
	future.promise()->setStarted();
	return future;
}

/******************************************************************************
* Checks if the data pipeline evaluation is completed and pending requests 
* can be served.
******************************************************************************/
void AsyncPipelineEvaluationHelper::serveRequests(DataObject* owner)
{
	if(_requests.empty()) return;
	Application::instance()->runOnceLater(owner, [this,owner]() {
		serveRequestsDeferred(owner);
	});
}

/******************************************************************************
* Checks if the data pipeline evaluation is completed and pending requests 
* can be served.
******************************************************************************/
void AsyncPipelineEvaluationHelper::serveRequestsDeferred(DataObject* owner)
{
	while(!_requests.empty()) {
		// Sort out canceled requests.
		Promise<PipelineFlowState>* promise = _requests.front().second.get(); 
		if(promise->isCanceled()) {
			promise->setFinished();
			_requests.erase(_requests.begin());
			continue;
		}
		
		// Check if we can now satisfy the oldest request.
		const PipelineFlowState& state = owner->evaluateImmediately(_requests.front().first);

		// The call above might have led to re-entrant call to this function; detect this kind of situation.
		if(_requests.empty() || promise != _requests.front().second.get())
			break;

		if(state.status().type() != PipelineStatus::Pending) {
			promise->setResult(state);
			promise->setFinished();
			OVITO_ASSERT(promise == _requests.front().second.get());
			_requests.erase(_requests.begin());
		}
		else {
			// Check back again later.
			break;
		}
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
