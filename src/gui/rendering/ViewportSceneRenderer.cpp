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

#include <gui/GUI.h>
#include <core/scene/SceneNode.h>
#include <core/scene/SceneRoot.h>
#include <core/scene/ObjectNode.h>
#include <core/scene/pipeline/PipelineObject.h>
#include <core/scene/pipeline/Modifier.h>
#include <core/scene/objects/DisplayObject.h>
#include <core/dataset/DataSet.h>
#include <core/app/Application.h>
#include <core/rendering/RenderSettings.h>
#include <gui/viewport/input/ViewportInputManager.h>
#include <gui/viewport/ViewportWindow.h>
#include <gui/mainwin/ViewportsPanel.h>
#include <gui/mainwin/MainWindow.h>
#include "ViewportSceneRenderer.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Rendering)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(ViewportSceneRenderer, OpenGLSceneRenderer);

/******************************************************************************
* This method is called just before renderFrame() is called.
******************************************************************************/
void ViewportSceneRenderer::beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp)
{
	OpenGLSceneRenderer::beginFrame(time, params, vp);

	// Set viewport background color.
	Color backgroundColor;
	if(!viewport()->renderPreviewMode())
		backgroundColor = Viewport::viewportColor(ViewportSettings::COLOR_VIEWPORT_BKG);
	else
		backgroundColor = renderSettings()->backgroundColor();
	setClearColor(ColorA(backgroundColor));
}

/******************************************************************************
* This virtual method is responsible for rendering additional content that is only
* visible in the interactive viewports.
******************************************************************************/
void ViewportSceneRenderer::renderInteractiveContent()
{
	// Render construction grid.
	if(viewport()->isGridVisible())
		renderGrid();

	// Render visual 3D representation of the modifiers.
	renderModifiers(false);

	// Render visual 2D representation of the modifiers.
	renderModifiers(true);

	// Render overlays of active input modes.
	if(MainWindow* mainWindow = MainWindow::fromDataset(renderDataset())) {

		// Render 3D overlays.
		for(const auto& handler : mainWindow->viewportInputManager()->stack()) {
			if(handler->hasOverlay())
				handler->renderOverlay3D(viewport(), this);
		}

		// Render 2D overlays on top.
		for(const auto& handler : mainWindow->viewportInputManager()->stack()) {
			if(handler->hasOverlay())
				handler->renderOverlay2D(viewport(), this);
		}
	}
}

/******************************************************************************
* Returns the final size of the rendered image in pixels.
******************************************************************************/
QSize ViewportSceneRenderer::outputSize() const
{
	return viewport()->windowSize();
}

/******************************************************************************
* Computes the bounding box of the the 3D visual elements
* shown only in the interactive viewports.
******************************************************************************/
Box3 ViewportSceneRenderer::boundingBoxInteractive(TimePoint time, Viewport* viewport)
{
	OVITO_CHECK_POINTER(viewport);
	Box3 bb;

	// Visit all pipeline objects in the scene.
	renderDataset()->sceneRoot()->visitObjectNodes([this, viewport, time, &bb](ObjectNode* node) -> bool {

		// Ignore node if it is the view node of the viewport or if it is the target of the view node.
		if(viewport->viewNode()) {
			if(viewport->viewNode() == node || viewport->viewNode()->lookatTargetNode() == node)
				return true;
		}

		// Evaluate data pipeline of object node.
		const PipelineFlowState& state = node->evaluatePipelineImmediately(PipelineEvalRequest(time, true));
		for(const auto& dataObj : state.objects()) {
			for(DisplayObject* displayObj : dataObj->displayObjects()) {
				if(displayObj && displayObj->isEnabled()) {
					TimeInterval interval;
					bb.addBox(displayObj->viewDependentBoundingBox(time, viewport,
							dataObj, node, state).transformed(node->getWorldTransform(time, interval)));
				}
			}
		}

		if(PipelineObject* pipelineObj = dynamic_object_cast<PipelineObject>(node->dataProvider()))
			boundingBoxModifiers(pipelineObj, node, bb);

		return true;
	});

	// Include visual geometry of input mode overlays in bounding box.
	if(MainWindow* mainWindow = MainWindow::fromDataset(viewport->dataset())) {
		for(const auto& handler : mainWindow->viewportInputManager()->stack()) {
			if(handler->hasOverlay())
				bb.addBox(handler->overlayBoundingBox(viewport, this));
		}
	}

	// Include construction grid in bounding box.
	if(viewport->isGridVisible()) {
		FloatType gridSpacing;
		Box2I gridRange;
		std::tie(gridSpacing, gridRange) = determineGridRange(viewport);
		if(gridSpacing > 0) {
			bb.addBox(viewport->gridMatrix() * Box3(
					Point3(gridRange.minc.x() * gridSpacing, gridRange.minc.y() * gridSpacing, 0),
					Point3(gridRange.maxc.x() * gridSpacing, gridRange.maxc.y() * gridSpacing, 0)));
		}
	}

	return bb;
}

/******************************************************************************
* Determines the range of the construction grid to display.
******************************************************************************/
std::tuple<FloatType, Box2I> ViewportSceneRenderer::determineGridRange(Viewport* vp)
{
	// Determine the area of the construction grid that is visible in the viewport.
	static const Point2 testPoints[] = {
		{-1,-1}, {1,-1}, {1, 1}, {-1, 1}, {0,1}, {0,-1}, {1,0}, {-1,0},
		{0,1}, {0,-1}, {1,0}, {-1,0}, {-1, 0.5}, {-1,-0.5}, {1,-0.5}, {1,0.5}, {0,0}
	};

	// Compute intersection points of test rays with grid plane.
	Box2 visibleGridRect;
	size_t numberOfIntersections = 0;
	for(size_t i = 0; i < sizeof(testPoints)/sizeof(testPoints[0]); i++) {
		Point3 p;
		if(vp->computeConstructionPlaneIntersection(testPoints[i], p, 0.1f)) {
			numberOfIntersections++;
			visibleGridRect.addPoint(p.x(), p.y());
		}
	}

	if(numberOfIntersections < 2) {
		// Cannot determine visible parts of the grid.
        return std::tuple<FloatType, Box2I>(0.0f, Box2I());
	}

	// Determine grid spacing adaptively.
	Point3 gridCenter(visibleGridRect.center().x(), visibleGridRect.center().y(), 0);
	FloatType gridSpacing = vp->nonScalingSize(vp->gridMatrix() * gridCenter) * 2.0f;
	// Round to nearest power of 10.
	gridSpacing = pow((FloatType)10, floor(log10(gridSpacing)));

	// Determine how many grid lines need to be rendered.
	int xstart = (int)floor(visibleGridRect.minc.x() / (gridSpacing * 10)) * 10;
	int xend = (int)ceil(visibleGridRect.maxc.x() / (gridSpacing * 10)) * 10;
	int ystart = (int)floor(visibleGridRect.minc.y() / (gridSpacing * 10)) * 10;
	int yend = (int)ceil(visibleGridRect.maxc.y() / (gridSpacing * 10)) * 10;

	return std::tuple<FloatType, Box2I>(gridSpacing, Box2I(Point2I(xstart, ystart), Point2I(xend, yend)));
}

/******************************************************************************
* Renders the construction grid.
******************************************************************************/
void ViewportSceneRenderer::renderGrid()
{
	if(isPicking())
		return;

	FloatType gridSpacing;
	Box2I gridRange;
	std::tie(gridSpacing, gridRange) = determineGridRange(viewport());
	if(gridSpacing <= 0) return;

	// Determine how many grid lines need to be rendered.
	int xstart = gridRange.minc.x();
	int ystart = gridRange.minc.y();
	int numLinesX = gridRange.size(0) + 1;
	int numLinesY = gridRange.size(1) + 1;

	FloatType xstartF = (FloatType)xstart * gridSpacing;
	FloatType ystartF = (FloatType)ystart * gridSpacing;
	FloatType xendF = (FloatType)(xstart + numLinesX - 1) * gridSpacing;
	FloatType yendF = (FloatType)(ystart + numLinesY - 1) * gridSpacing;

	// Allocate vertex buffer.
	int numVertices = 2 * (numLinesX + numLinesY);
	std::unique_ptr<Point3[]> vertexPositions(new Point3[numVertices]);
	std::unique_ptr<ColorA[]> vertexColors(new ColorA[numVertices]);

	// Build lines array.
	ColorA color = Viewport::viewportColor(ViewportSettings::COLOR_GRID);
	ColorA majorColor = Viewport::viewportColor(ViewportSettings::COLOR_GRID_INTENS);
	ColorA majorMajorColor = Viewport::viewportColor(ViewportSettings::COLOR_GRID_AXIS);

	Point3* v = vertexPositions.get();
	ColorA* c = vertexColors.get();
	FloatType x = xstartF;
	for(int i = xstart; i < xstart + numLinesX; i++, x += gridSpacing, c += 2) {
		*v++ = Point3(x, ystartF, 0);
		*v++ = Point3(x, yendF, 0);
		if((i % 10) != 0)
			c[0] = c[1] = color;
		else if(i != 0)
			c[0] = c[1] = majorColor;
		else
			c[0] = c[1] = majorMajorColor;
	}
	FloatType y = ystartF;
	for(int i = ystart; i < ystart + numLinesY; i++, y += gridSpacing, c += 2) {
		*v++ = Point3(xstartF, y, 0);
		*v++ = Point3(xendF, y, 0);
		if((i % 10) != 0)
			c[0] = c[1] = color;
		else if(i != 0)
			c[0] = c[1] = majorColor;
		else
			c[0] = c[1] = majorMajorColor;
	}
	OVITO_ASSERT(c == vertexColors.get() + numVertices);

	// Render grid lines.
	setWorldTransform(viewport()->gridMatrix());
	if(!_constructionGridGeometry || !_constructionGridGeometry->isValid(this))
		_constructionGridGeometry = createLinePrimitive();
	_constructionGridGeometry->setVertexCount(numVertices);
	_constructionGridGeometry->setVertexPositions(vertexPositions.get());
	_constructionGridGeometry->setVertexColors(vertexColors.get());
	_constructionGridGeometry->render(this);
}

/******************************************************************************
* Returns the device pixel ratio of the output device we are rendering to.
******************************************************************************/
qreal ViewportSceneRenderer::devicePixelRatio() const
{
	if(viewport()) {
		if(QWidget* vpWidget = ViewportsPanel::viewportWidget(viewport()))
			return vpWidget->devicePixelRatio();
	}

	return OpenGLSceneRenderer::devicePixelRatio();
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
