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

#include <core/Core.h>
#include <core/rendering/SceneRenderer.h>
#include <core/scene/objects/geometry/TriMeshObject.h>
#include "TriMeshDisplay.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene) OVITO_BEGIN_INLINE_NAMESPACE(StdObj)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(TriMeshDisplay, DisplayObject);
DEFINE_FLAGS_PROPERTY_FIELD(TriMeshDisplay, color, "Color", PROPERTY_FIELD_MEMORIZE);
DEFINE_REFERENCE_FIELD(TriMeshDisplay, transparencyController, "Transparency", Controller);
SET_PROPERTY_FIELD_LABEL(TriMeshDisplay, color, "Display color");
SET_PROPERTY_FIELD_LABEL(TriMeshDisplay, transparencyController, "Transparency");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(TriMeshDisplay, transparencyController, PercentParameterUnit, 0, 1);

/******************************************************************************
* Constructor.
******************************************************************************/
TriMeshDisplay::TriMeshDisplay(DataSet* dataset) : DisplayObject(dataset),
	_color(0.85, 0.85, 1)
{
	INIT_PROPERTY_FIELD(color);
	INIT_PROPERTY_FIELD(transparencyController);

	setTransparencyController(ControllerManager::createFloatController(dataset));
}

/******************************************************************************
* Computes the bounding box of the object.
******************************************************************************/
Box3 TriMeshDisplay::boundingBox(TimePoint time, DataObject* dataObject, ObjectNode* contextNode, const PipelineFlowState& flowState)
{
	// Detect if the input data has changed since the last time we computed the bounding box.
	if(_boundingBoxCacheHelper.updateState(dataObject) || _cachedBoundingBox.isEmpty()) {
		// Recompute bounding box.
		OORef<TriMeshObject> triMeshObj = dataObject->convertTo<TriMeshObject>(time);
		if(triMeshObj)
			_cachedBoundingBox = triMeshObj->mesh().boundingBox();
		else
			_cachedBoundingBox.setEmpty();
	}
	return _cachedBoundingBox;
}

/******************************************************************************
* Lets the display object render a data object.
******************************************************************************/
void TriMeshDisplay::render(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState, SceneRenderer* renderer, ObjectNode* contextNode)
{
	// Do we have to re-create the geometry buffer from scratch?
	bool recreateBuffer = !_buffer || !_buffer->isValid(renderer);

	FloatType transp = 0;
	TimeInterval iv;
	if(transparencyController()) transp = transparencyController()->getFloatValue(time, iv);
	ColorA color_mesh(color(), FloatType(1) - transp);

	// Do we have to update contents of the geometry buffer?
	bool updateContents = _geometryCacheHelper.updateState(dataObject, color_mesh) || recreateBuffer;

	// Re-create the geometry buffer if necessary.
	if(recreateBuffer)
		_buffer = renderer->createMeshPrimitive();

	// Update buffer contents.
	if(updateContents) {
		OORef<TriMeshObject> triMeshObj = dataObject->convertTo<TriMeshObject>(time);
		if(triMeshObj)
			_buffer->setMesh(triMeshObj->mesh(), color_mesh);
		else
			_buffer->setMesh(TriMesh(), ColorA(1,1,1,1));
	}

	renderer->beginPickObject(contextNode);
	_buffer->render(renderer);
	renderer->endPickObject();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
