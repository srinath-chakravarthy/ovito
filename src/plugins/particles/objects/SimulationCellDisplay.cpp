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

#include <plugins/particles/Particles.h>
#include <core/utilities/units/UnitsManager.h>
#include <core/rendering/SceneRenderer.h>
#include "SimulationCellDisplay.h"
#include "SimulationCellObject.h"

namespace Ovito { namespace Particles {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(SimulationCellDisplay, DisplayObject);
DEFINE_PROPERTY_FIELD(SimulationCellDisplay, renderCellEnabled, "RenderSimulationCell");
DEFINE_PROPERTY_FIELD(SimulationCellDisplay, cellLineWidth, "SimulationCellLineWidth");
DEFINE_FLAGS_PROPERTY_FIELD(SimulationCellDisplay, cellColor, "SimulationCellRenderingColor", PROPERTY_FIELD_MEMORIZE);
SET_PROPERTY_FIELD_LABEL(SimulationCellDisplay, cellLineWidth, "Line width");
SET_PROPERTY_FIELD_LABEL(SimulationCellDisplay, renderCellEnabled, "Render cell");
SET_PROPERTY_FIELD_LABEL(SimulationCellDisplay, cellColor, "Line color");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(SimulationCellDisplay, cellLineWidth, WorldParameterUnit, 0);

/******************************************************************************
* Constructor.
******************************************************************************/
SimulationCellDisplay::SimulationCellDisplay(DataSet* dataset) : DisplayObject(dataset),
	_renderCellEnabled(true),
	_cellLineWidth(0.5),
	_cellColor(0, 0, 0)
{
	INIT_PROPERTY_FIELD(renderCellEnabled);
	INIT_PROPERTY_FIELD(cellLineWidth);
	INIT_PROPERTY_FIELD(cellColor);
}

/******************************************************************************
* Computes the bounding box of the object.
******************************************************************************/
Box3 SimulationCellDisplay::boundingBox(TimePoint time, DataObject* dataObject, ObjectNode* contextNode, const PipelineFlowState& flowState)
{
	SimulationCellObject* cellObject = dynamic_object_cast<SimulationCellObject>(dataObject);
	OVITO_CHECK_OBJECT_POINTER(cellObject);

	AffineTransformation matrix = cellObject->cellMatrix();
	if(cellObject->is2D()) {
		matrix.column(2).setZero();
		matrix.translation().z() = 0;
	}

	return Box3(Point3(0), Point3(1)).transformed(matrix).padBox(cellLineWidth());
}

/******************************************************************************
* Lets the display object render the data object.
******************************************************************************/
void SimulationCellDisplay::render(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState, SceneRenderer* renderer, ObjectNode* contextNode)
{
	SimulationCellObject* cell = dynamic_object_cast<SimulationCellObject>(dataObject);
	OVITO_CHECK_OBJECT_POINTER(cell);

	if(renderer->isInteractive() && !renderer->viewport()->renderPreviewMode()) {
		renderWireframe(cell, renderer, contextNode);
	}
	else {
		if(!renderCellEnabled())
			return;		// Do nothing if rendering has been disabled by the user.

		renderSolid(cell, renderer, contextNode);
	}
}

/******************************************************************************
* Renders the given simulation cell using lines.
******************************************************************************/
void SimulationCellDisplay::renderWireframe(SimulationCellObject* cell, SceneRenderer* renderer, ObjectNode* contextNode)
{
	ColorA color = ViewportSettings::getSettings().viewportColor(contextNode->isSelected() ? ViewportSettings::COLOR_SELECTION : ViewportSettings::COLOR_UNSELECTED);

	if(_wireframeGeometryCacheHelper.updateState(cell, color)
			|| !_wireframeGeometry
			|| !_wireframeGeometry->isValid(renderer)
			|| !_wireframePickingGeometry
			|| !_wireframePickingGeometry->isValid(renderer)) {
		_wireframeGeometry = renderer->createLinePrimitive();
		_wireframePickingGeometry = renderer->createLinePrimitive();
		_wireframeGeometry->setVertexCount(cell->is2D() ? 8 : 24);
		_wireframePickingGeometry->setVertexCount(_wireframeGeometry->vertexCount(), renderer->defaultLinePickingWidth());
		Point3 corners[8];
		corners[0] = cell->cellOrigin();
		if(cell->is2D()) corners[0].z() = 0;
		corners[1] = corners[0] + cell->cellVector1();
		corners[2] = corners[1] + cell->cellVector2();
		corners[3] = corners[0] + cell->cellVector2();
		corners[4] = corners[0] + cell->cellVector3();
		corners[5] = corners[1] + cell->cellVector3();
		corners[6] = corners[2] + cell->cellVector3();
		corners[7] = corners[3] + cell->cellVector3();
		Point3 vertices[24] = {
			corners[0], corners[1],
			corners[1], corners[2],
			corners[2], corners[3],
			corners[3], corners[0],
			corners[4], corners[5],
			corners[5], corners[6],
			corners[6], corners[7],
			corners[7], corners[4],
			corners[0], corners[4],
			corners[1], corners[5],
			corners[2], corners[6],
			corners[3], corners[7]};
		_wireframeGeometry->setVertexPositions(vertices);
		_wireframeGeometry->setLineColor(color);
		_wireframePickingGeometry->setVertexPositions(vertices);
		_wireframePickingGeometry->setLineColor(color);
	}

	renderer->beginPickObject(contextNode);
	if(!renderer->isPicking())
		_wireframeGeometry->render(renderer);
	else
		_wireframePickingGeometry->render(renderer);
	renderer->endPickObject();
}

/******************************************************************************
* Renders the given simulation cell using solid shading mode.
******************************************************************************/
void SimulationCellDisplay::renderSolid(SimulationCellObject* cell, SceneRenderer* renderer, ObjectNode* contextNode)
{
	if(_solidGeometryCacheHelper.updateState(cell, cellLineWidth(), cellColor())
			|| !_edgeGeometry || !_cornerGeometry
			|| !_edgeGeometry->isValid(renderer)
			|| !_cornerGeometry->isValid(renderer)) {
		_edgeGeometry = renderer->createArrowPrimitive(ArrowPrimitive::CylinderShape, ArrowPrimitive::NormalShading, ArrowPrimitive::HighQuality);
		_cornerGeometry = renderer->createParticlePrimitive(ParticlePrimitive::NormalShading, ParticlePrimitive::HighQuality);
		_edgeGeometry->startSetElements(cell->is2D() ? 4 : 12);
		ColorA color = (ColorA)cellColor();
		Point3 corners[8];
		corners[0] = cell->cellOrigin();
		if(cell->is2D()) corners[0].z() = 0;
		corners[1] = corners[0] + cell->cellVector1();
		corners[2] = corners[1] + cell->cellVector2();
		corners[3] = corners[0] + cell->cellVector2();
		corners[4] = corners[0] + cell->cellVector3();
		corners[5] = corners[1] + cell->cellVector3();
		corners[6] = corners[2] + cell->cellVector3();
		corners[7] = corners[3] + cell->cellVector3();
		_edgeGeometry->setElement(0, corners[0], corners[1] - corners[0], color, cellLineWidth());
		_edgeGeometry->setElement(1, corners[1], corners[2] - corners[1], color, cellLineWidth());
		_edgeGeometry->setElement(2, corners[2], corners[3] - corners[2], color, cellLineWidth());
		_edgeGeometry->setElement(3, corners[3], corners[0] - corners[3], color, cellLineWidth());
		if(cell->is2D() == false) {
			_edgeGeometry->setElement(4, corners[4], corners[5] - corners[4], color, cellLineWidth());
			_edgeGeometry->setElement(5, corners[5], corners[6] - corners[5], color, cellLineWidth());
			_edgeGeometry->setElement(6, corners[6], corners[7] - corners[6], color, cellLineWidth());
			_edgeGeometry->setElement(7, corners[7], corners[4] - corners[7], color, cellLineWidth());
			_edgeGeometry->setElement(8, corners[0], corners[4] - corners[0], color, cellLineWidth());
			_edgeGeometry->setElement(9, corners[1], corners[5] - corners[1], color, cellLineWidth());
			_edgeGeometry->setElement(10, corners[2], corners[6] - corners[2], color, cellLineWidth());
			_edgeGeometry->setElement(11, corners[3], corners[7] - corners[3], color, cellLineWidth());
		}
		_edgeGeometry->endSetElements();
		_cornerGeometry->setSize(cell->is2D() ? 4 : 8);
		_cornerGeometry->setParticlePositions(corners);
		_cornerGeometry->setParticleRadius(cellLineWidth());
		_cornerGeometry->setParticleColor(cellColor());
	}
	renderer->beginPickObject(contextNode);
	_edgeGeometry->render(renderer);
	_cornerGeometry->render(renderer);
	renderer->endPickObject();
}

}	// End of namespace
}	// End of namespace
