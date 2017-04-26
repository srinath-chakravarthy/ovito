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

#include <plugins/particles/gui/ParticlesGui.h>
#include <core/viewport/Viewport.h>
#include <core/scene/ObjectNode.h>
#include <core/animation/AnimationSettings.h>
#include <gui/viewport/ViewportWindow.h>
#include <plugins/particles/objects/ParticlePropertyObject.h>
#include <plugins/particles/objects/ParticleTypeProperty.h>
#include <plugins/particles/objects/ParticleDisplay.h>
#include "ParticlePickingHelper.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Util)

/******************************************************************************
* Finds the particle under the mouse cursor.
******************************************************************************/
bool ParticlePickingHelper::pickParticle(ViewportWindow* vpwin, const QPoint& clickPoint, PickResult& result)
{
	ViewportPickResult vpPickResult = vpwin->pick(clickPoint);
	// Check if user has clicked on something.
	if(vpPickResult) {

		// Check if that was a particle.
		ParticlePickInfo* pickInfo = dynamic_object_cast<ParticlePickInfo>(vpPickResult.pickInfo);
		if(pickInfo) {
			ParticlePropertyObject* posProperty = ParticlePropertyObject::findInState(pickInfo->pipelineState(), ParticleProperty::PositionProperty);
			int particleIndex = pickInfo->particleIndexFromSubObjectID(vpPickResult.subobjectId);
			if(posProperty && particleIndex >= 0) {
				// Save reference to the selected particle.
				TimeInterval iv;
				result.objNode = vpPickResult.objectNode;
				result.particleIndex = particleIndex;
				result.localPos = posProperty->getPoint3(result.particleIndex);
				result.worldPos = result.objNode->getWorldTransform(vpwin->viewport()->dataset()->animationSettings()->time(), iv) * result.localPos;

				// Determine particle ID.
				ParticlePropertyObject* identifierProperty = ParticlePropertyObject::findInState(pickInfo->pipelineState(), ParticleProperty::IdentifierProperty);
				if(identifierProperty && result.particleIndex < identifierProperty->size()) {
					result.particleId = identifierProperty->getInt(result.particleIndex);
				}
				else result.particleId = -1;

				return true;
			}
		}
	}

	result.objNode = nullptr;
	return false;
}

/******************************************************************************
* Computes the world space bounding box of the particle selection marker.
******************************************************************************/
Box3 ParticlePickingHelper::selectionMarkerBoundingBox(Viewport* vp, const PickResult& pickRecord)
{
	if(!pickRecord.objNode)
		return Box3();

	const PipelineFlowState& flowState = pickRecord.objNode->evaluatePipelineImmediately(PipelineEvalRequest(vp->dataset()->animationSettings()->time(), true));

	// If particle selection is based on ID, find particle with the given ID.
	size_t particleIndex = pickRecord.particleIndex;
	if(pickRecord.particleId >= 0) {
		ParticlePropertyObject* identifierProperty = ParticlePropertyObject::findInState(flowState, ParticleProperty::IdentifierProperty);
		if(identifierProperty) {
			const int* begin = identifierProperty->constDataInt();
			const int* end = begin + identifierProperty->size();
			const int* iter = std::find(begin, end, pickRecord.particleId);
			if(iter != end)
				particleIndex = (iter - begin);
		}
	}

	// Fetch properties of selected particle needed to compute the bounding box.
	ParticlePropertyObject* posProperty = ParticlePropertyObject::findInState(flowState, ParticleProperty::PositionProperty);
	if(!posProperty)
		return Box3();

	// Get the particle display object, which is attached to the position property object.
	ParticleDisplay* particleDisplay = nullptr;
	for(DisplayObject* displayObj : posProperty->displayObjects()) {
		if((particleDisplay = dynamic_object_cast<ParticleDisplay>(displayObj)) != nullptr)
			break;
	}
	if(!particleDisplay)
		return Box3();

	TimeInterval iv;
	const AffineTransformation& nodeTM = pickRecord.objNode->getWorldTransform(vp->dataset()->animationSettings()->time(), iv);

	return nodeTM * particleDisplay->highlightParticleBoundingBox(particleIndex, flowState, nodeTM, vp);
}

/******************************************************************************
* Renders the atom selection overlay in a viewport.
******************************************************************************/
void ParticlePickingHelper::renderSelectionMarker(Viewport* vp, ViewportSceneRenderer* renderer, const PickResult& pickRecord)
{
	if(!pickRecord.objNode)
		return;

	if(!renderer->isInteractive() || renderer->isPicking())
		return;

	const PipelineFlowState& flowState = pickRecord.objNode->evaluatePipelineImmediately(PipelineEvalRequest(vp->dataset()->animationSettings()->time(), true));

	// If particle selection is based on ID, find particle with the given ID.
	size_t particleIndex = pickRecord.particleIndex;
	if(pickRecord.particleId >= 0) {
		ParticlePropertyObject* identifierProperty = ParticlePropertyObject::findInState(flowState, ParticleProperty::IdentifierProperty);
		if(identifierProperty) {
			const int* begin = identifierProperty->constDataInt();
			const int* end = begin + identifierProperty->size();
			const int* iter = std::find(begin, end, pickRecord.particleId);
			if(iter != end)
				particleIndex = (iter - begin);
		}
	}

	// Fetch properties of selected particle needed to render the overlay.
	ParticlePropertyObject* posProperty = ParticlePropertyObject::findInState(flowState, ParticleProperty::PositionProperty);
	if(!posProperty)
		return;

	// Get the particle display object, which is attached to the position property object.
	ParticleDisplay* particleDisplay = nullptr;
	for(DisplayObject* displayObj : posProperty->displayObjects()) {
		if((particleDisplay = dynamic_object_cast<ParticleDisplay>(displayObj)) != nullptr)
			break;
	}
	if(!particleDisplay)
		return;

	// Set up transformation.
	TimeInterval iv;
	const AffineTransformation& nodeTM = pickRecord.objNode->getWorldTransform(vp->dataset()->animationSettings()->time(), iv);
	renderer->setWorldTransform(nodeTM);

	// Render highlight marker.
	particleDisplay->highlightParticle(particleIndex, flowState, renderer);
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
