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
#include <core/viewport/Viewport.h>
#include <core/viewport/ViewportConfiguration.h>
#include <core/rendering/RenderSettings.h>
#include <core/app/Application.h>
#include <gui/viewport/input/ViewportInputManager.h>
#include <gui/rendering/ViewportSceneRenderer.h>
#include <gui/viewport/picking/PickingSceneRenderer.h>
#include <gui/mainwin/MainWindow.h>
#include "ViewportWindow.h"
#include "ViewportMenu.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)


/******************************************************************************
* Constructor.
******************************************************************************/
ViewportWindow::ViewportWindow(Viewport* owner, QWidget* parentWidget) : QOpenGLWidget(parentWidget),
		_viewport(owner), _updateRequested(false),
		_mainWindow(MainWindow::fromDataset(owner->dataset())),
		_renderDebugCounter(0), _cursorInContextMenuArea(false)
{
	// Associate the viewport with this window.
	owner->setWindow(this);

	setMouseTracking(true);
	setFocusPolicy(Qt::ClickFocus);

	// Determine OpenGL vendor string so other parts of the code can decide
	// which OpenGL features are save to use.
	OpenGLSceneRenderer::determineOpenGLInfo();

	// Create the viewport renderer.
	// It is shared by all viewports of a dataset.
	for(Viewport* vp : owner->dataset()->viewportConfig()->viewports()) {
		if(vp->window() != nullptr) {
			_viewportRenderer = static_cast<ViewportWindow*>(vp->window())->_viewportRenderer;
			if(_viewportRenderer) break;
		}
	}
	if(!_viewportRenderer)
		_viewportRenderer = new ViewportSceneRenderer(owner->dataset());

	// Create the object picking renderer.
	_pickingRenderer = new PickingSceneRenderer(owner->dataset());
}

/******************************************************************************
* Destructor.
******************************************************************************/
ViewportWindow::~ViewportWindow()
{
	// Detach from Viewport class.
	if(viewport())
		viewport()->setWindow(nullptr);
}

/******************************************************************************
* Puts an update request event for this viewport on the event loop.
******************************************************************************/
void ViewportWindow::renderLater()
{
	_updateRequested = true;
	update();
}

/******************************************************************************
* If an update request is pending for this viewport window, immediately
* processes it and redraw the window contents.
******************************************************************************/
void ViewportWindow::processViewportUpdate()
{
	if(_updateRequested) {
		OVITO_ASSERT_MSG(!_viewport->isRendering(), "ViewportWindow::processUpdateRequest()", "Recursive viewport repaint detected.");
		OVITO_ASSERT_MSG(!_viewport->dataset()->viewportConfig()->isRendering(), "ViewportWindow::processUpdateRequest()", "Recursive viewport repaint detected.");
		repaint();
	}
}

/******************************************************************************
* Displays the context menu for this viewport.
******************************************************************************/
void ViewportWindow::showViewportMenu(const QPoint& pos)
{
	// Create the context menu for the viewport.
	ViewportMenu contextMenu(this);

	// Show menu.
	contextMenu.show(pos);
}

/******************************************************************************
* Renders the viewport caption text.
******************************************************************************/
void ViewportWindow::renderViewportTitle()
{
	// Create a rendering buffer that is responsible for rendering the viewport's caption text.
	if(!_captionBuffer || !_captionBuffer->isValid(_viewportRenderer)) {
		_captionBuffer = _viewportRenderer->createTextPrimitive();
		_captionBuffer->setFont(ViewportSettings::getSettings().viewportFont());
	}

	if(_cursorInContextMenuArea && !_captionBuffer->font().underline()) {
		QFont font = _captionBuffer->font();
		font.setUnderline(true);
		_captionBuffer->setFont(font);
	}
	else if(!_cursorInContextMenuArea && _captionBuffer->font().underline()) {
		QFont font = _captionBuffer->font();
		font.setUnderline(false);
		_captionBuffer->setFont(font);
	}

	QString str = viewport()->viewportTitle();
	if(viewport()->renderPreviewMode())
		str += tr(" (preview)");
#ifdef OVITO_DEBUG
	str += QString(" [%1]").arg(++_renderDebugCounter);
#endif
	_captionBuffer->setText(str);
	Color textColor = Viewport::viewportColor(ViewportSettings::COLOR_VIEWPORT_CAPTION);
	if(viewport()->renderPreviewMode() && textColor == _viewportRenderer->renderSettings()->backgroundColor())
		textColor = Vector3(1,1,1) - (Vector3)textColor;
	_captionBuffer->setColor(ColorA(textColor));

	QFontMetricsF metrics(_captionBuffer->font());
	Point2 pos = Point2(2, 2) * (FloatType)devicePixelRatio();
	_contextMenuArea = QRect(0, 0, std::max(metrics.width(_captionBuffer->text()), 30.0) + pos.x(), metrics.height() + pos.y());
	_captionBuffer->renderWindow(_viewportRenderer, pos, Qt::AlignLeft | Qt::AlignTop);
}

/******************************************************************************
* Sets whether mouse grab should be enabled or not for this viewport window.
******************************************************************************/
bool ViewportWindow::setMouseGrabEnabled(bool grab)
{
	if(grab) {
		grabMouse();
		return true;
	}
	else {
		releaseMouse();
		return false;
	}
}

/******************************************************************************
* Render the axis tripod symbol in the corner of the viewport that indicates
* the coordinate system orientation.
******************************************************************************/
void ViewportWindow::renderOrientationIndicator()
{
	const FloatType tripodSize = FloatType(80);			// device-independent pixels
	const FloatType tripodArrowSize = FloatType(0.17); 	// percentage of the above value.

	// Turn off depth-testing.
	_viewportRenderer->setDepthTestEnabled(false);

	// Setup projection matrix.
	const FloatType tripodPixelSize = tripodSize * _viewportRenderer->devicePixelRatio();
	_viewportRenderer->setRenderingViewport(0, 0, tripodPixelSize, tripodPixelSize);
	ViewProjectionParameters projParams = viewport()->projectionParams();
	projParams.projectionMatrix = Matrix4::ortho(-1.4f, 1.4f, -1.4f, 1.4f, -2, 2);
	projParams.inverseProjectionMatrix = projParams.projectionMatrix.inverse();
	projParams.viewMatrix.setIdentity();
	projParams.inverseViewMatrix.setIdentity();
	projParams.isPerspective = false;
	_viewportRenderer->setProjParams(projParams);
	_viewportRenderer->setWorldTransform(AffineTransformation::Identity());

    static const ColorA axisColors[3] = { ColorA(1, 0, 0), ColorA(0, 1, 0), ColorA(0.4f, 0.4f, 1) };
	static const QString labels[3] = { QStringLiteral("x"), QStringLiteral("y"), QStringLiteral("z") };

	// Create line buffer.
	if(!_orientationTripodGeometry || !_orientationTripodGeometry->isValid(_viewportRenderer)) {
		_orientationTripodGeometry = _viewportRenderer->createLinePrimitive();
		_orientationTripodGeometry->setVertexCount(18);
		ColorA vertexColors[18];
		for(int i = 0; i < 18; i++)
			vertexColors[i] = axisColors[i / 6];
		_orientationTripodGeometry->setVertexColors(vertexColors);
	}

	// Render arrows.
	Point3 vertices[18];
	for(int axis = 0, index = 0; axis < 3; axis++) {
		Vector3 dir = viewport()->projectionParams().viewMatrix.column(axis).normalized();
		vertices[index++] = Point3::Origin();
		vertices[index++] = Point3::Origin() + dir;
		vertices[index++] = Point3::Origin() + dir;
		vertices[index++] = Point3::Origin() + (dir + tripodArrowSize * Vector3(dir.y() - dir.x(), -dir.x() - dir.y(), dir.z()));
		vertices[index++] = Point3::Origin() + dir;
		vertices[index++] = Point3::Origin() + (dir + tripodArrowSize * Vector3(-dir.y() - dir.x(), dir.x() - dir.y(), dir.z()));
	}
	_orientationTripodGeometry->setVertexPositions(vertices);
	_orientationTripodGeometry->render(_viewportRenderer);

	// Render x,y,z labels.
	for(int axis = 0; axis < 3; axis++) {

		// Create a rendering buffer that is responsible for rendering the text label.
		if(!_orientationTripodLabels[axis] || !_orientationTripodLabels[axis]->isValid(_viewportRenderer)) {
			_orientationTripodLabels[axis] = _viewportRenderer->createTextPrimitive();
			_orientationTripodLabels[axis]->setFont(ViewportSettings::getSettings().viewportFont());
			_orientationTripodLabels[axis]->setColor(axisColors[axis]);
			_orientationTripodLabels[axis]->setText(labels[axis]);
		}

		Point3 p = Point3::Origin() + viewport()->projectionParams().viewMatrix.column(axis).resized(1.2f);
		Point3 ndcPoint = projParams.projectionMatrix * p;
		Point2 windowPoint(( ndcPoint.x() + FloatType(1)) * tripodPixelSize / 2,
							(-ndcPoint.y() + FloatType(1)) * tripodPixelSize / 2);
		_orientationTripodLabels[axis]->renderWindow(_viewportRenderer, windowPoint, Qt::AlignHCenter | Qt::AlignVCenter);
	}

	// Restore old rendering attributes.
	_viewportRenderer->setDepthTestEnabled(true);
	_viewportRenderer->setRenderingViewport(0, 0, viewportWindowDeviceSize().width(), viewportWindowDeviceSize().height());
}

/******************************************************************************
* Renders the frame on top of the scene that indicates the visible rendering area.
******************************************************************************/
void ViewportWindow::renderRenderFrame()
{
	// Create a rendering buffer that is responsible for rendering the frame.
	if(!_renderFrameOverlay || !_renderFrameOverlay->isValid(_viewportRenderer)) {
		_renderFrameOverlay = _viewportRenderer->createImagePrimitive();
		QImage image(1, 1, QImage::Format_ARGB32);
		image.fill(0xA0A0A0A0);
		_renderFrameOverlay->setImage(image);
	}

	Box2 rect = viewport()->renderFrameRect();

	// Render rectangle borders
	_renderFrameOverlay->renderViewport(_viewportRenderer, Point2(-1,-1), Vector2(FloatType(1) + rect.minc.x(), 2));
	_renderFrameOverlay->renderViewport(_viewportRenderer, Point2(rect.maxc.x(),-1), Vector2(FloatType(1) - rect.maxc.x(), 2));
	_renderFrameOverlay->renderViewport(_viewportRenderer, Point2(rect.minc.x(),-1), Vector2(rect.width(), FloatType(1) + rect.minc.y()));
	_renderFrameOverlay->renderViewport(_viewportRenderer, Point2(rect.minc.x(),rect.maxc.y()), Vector2(rect.width(), FloatType(1) - rect.maxc.y()));
}

/******************************************************************************
* Determines the object that is visible under the given mouse cursor position.
******************************************************************************/
ViewportPickResult ViewportWindow::pick(const QPointF& pos)
{
	// Cannot perform picking while viewport is not visible or currently rendering or when updates are disabled.
	if(isVisible() && !viewport()->isRendering() && !viewport()->dataset()->viewportConfig()->isSuspended()) {
		try {
			if(_pickingRenderer->isRefreshRequired()) {
				// Let the viewport do the actual rendering work.
				viewport()->renderInteractive(_pickingRenderer);
			}

			// Query which object is located at the given window position.
			const QPoint pixelPos = (pos * devicePixelRatio()).toPoint();
			const PickingSceneRenderer::ObjectRecord* objInfo;
			quint32 subobjectId;
			std::tie(objInfo, subobjectId) = _pickingRenderer->objectAtLocation(pixelPos);
			if(objInfo) {
				ViewportPickResult result;
				result.objectNode = objInfo->objectNode;
				result.pickInfo = objInfo->pickInfo;
				result.worldPosition = _pickingRenderer->worldPositionFromLocation(pixelPos);
				result.subobjectId = subobjectId;
				return result;
			}
		}
		catch(const Exception& ex) {
			ex.reportError();
		}
	}
	return {};
}

/******************************************************************************
* Is called whenever the GL context needs to be initialized.
******************************************************************************/
void ViewportWindow::initializeGL()
{
}

/******************************************************************************
* Is called whenever the widget needs to be painted.
******************************************************************************/
void ViewportWindow::paintGL()
{
	OVITO_ASSERT_MSG(!_viewport->isRendering(), "ViewportWindow::paintGL()", "Recursive viewport repaint detected.");
	OVITO_ASSERT_MSG(!_viewport->dataset()->viewportConfig()->isRendering(), "ViewportWindow::paintGL()", "Recursive viewport repaint detected.");
	renderNow();
}

/******************************************************************************
* Handles the show events.
******************************************************************************/
void ViewportWindow::showEvent(QShowEvent* event)
{
	if(!event->spontaneous())
		update();
}

/******************************************************************************
* Handles double click events.
******************************************************************************/
void ViewportWindow::mouseDoubleClickEvent(QMouseEvent* event)
{
	ViewportInputMode* mode = _mainWindow->viewportInputManager()->activeMode();
	if(mode) {
		try {
			mode->mouseDoubleClickEvent(this, event);
		}
		catch(const Exception& ex) {
			qWarning() << "Uncaught exception in viewport mouse event handler:";
			ex.logError();
		}
	}
}

/******************************************************************************
* Handles mouse press events.
******************************************************************************/
void ViewportWindow::mousePressEvent(QMouseEvent* event)
{
	viewport()->dataset()->viewportConfig()->setActiveViewport(_viewport);

	// Intercept mouse clicks on the viewport caption.
	if(_contextMenuArea.contains(event->pos())) {
		showViewportMenu(event->pos());
		return;
	}

	ViewportInputMode* mode = _mainWindow->viewportInputManager()->activeMode();
	if(mode) {
		try {
			mode->mousePressEvent(this, event);
		}
		catch(const Exception& ex) {
			qWarning() << "Uncaught exception in viewport mouse event handler:";
			ex.logError();
		}
	}
}

/******************************************************************************
* Handles mouse release events.
******************************************************************************/
void ViewportWindow::mouseReleaseEvent(QMouseEvent* event)
{
	ViewportInputMode* mode = _mainWindow->viewportInputManager()->activeMode();
	if(mode) {
		try {
			mode->mouseReleaseEvent(this, event);
		}
		catch(const Exception& ex) {
			qWarning() << "Uncaught exception in viewport mouse event handler:";
			ex.logError();
		}
	}
}

/******************************************************************************
* Handles mouse move events.
******************************************************************************/
void ViewportWindow::mouseMoveEvent(QMouseEvent* event)
{
	if(_contextMenuArea.contains(event->pos()) && !_cursorInContextMenuArea) {
		_cursorInContextMenuArea = true;
		viewport()->updateViewport();
	}
	else if(!_contextMenuArea.contains(event->pos()) && _cursorInContextMenuArea) {
		_cursorInContextMenuArea = false;
		viewport()->updateViewport();
	}

	ViewportInputMode* mode = _mainWindow->viewportInputManager()->activeMode();
	if(mode) {
		try {
			mode->mouseMoveEvent(this, event);
		}
		catch(const Exception& ex) {
			qWarning() << "Uncaught exception in viewport mouse event handler:";
			ex.logError();
		}
	}
}

/******************************************************************************
* Handles mouse wheel events.
******************************************************************************/
void ViewportWindow::wheelEvent(QWheelEvent* event)
{
	ViewportInputMode* mode = _mainWindow->viewportInputManager()->activeMode();
	if(mode) {
		try {
			mode->wheelEvent(this, event);
		}
		catch(const Exception& ex) {
			qWarning() << "Uncaught exception in viewport mouse event handler:";
			ex.logError();
		}
	}
}

/******************************************************************************
* Is called when the mouse cursor leaves the widget.
******************************************************************************/
void ViewportWindow::leaveEvent(QEvent* event)
{
	if(_cursorInContextMenuArea) {
		_cursorInContextMenuArea = false;
		viewport()->updateViewport();
	}
}

/******************************************************************************
* Is called when the widgets looses the input focus.
******************************************************************************/
void ViewportWindow::focusOutEvent(QFocusEvent* event)
{
	ViewportInputMode* mode = _mainWindow->viewportInputManager()->activeMode();
	if(mode) {
		try {
			mode->focusOutEvent(this, event);
		}
		catch(const Exception& ex) {
			qWarning() << "Uncaught exception in viewport event handler:";
			ex.logError();
		}
	}
}

/******************************************************************************
* Renders custom GUI elements in the viewport on top of the scene.
******************************************************************************/
void ViewportWindow::renderGui()
{
	if(viewport()->renderPreviewMode()) {
		// Render render frame.
		renderRenderFrame();
	}
	else {
		// Render orientation tripod.
		renderOrientationIndicator();
	}

	// Render viewport caption.
	renderViewportTitle();
}

/******************************************************************************
* Immediately redraws the contents of this window.
******************************************************************************/
void ViewportWindow::renderNow()
{
	_updateRequested = false;

	// Do not re-enter rendering function of the same viewport.
	if(!viewport() || viewport()->isRendering())
		return;

	QSurfaceFormat format = context()->format();
	// OpenGL in a VirtualBox machine Windows guest reports "2.1 Chromium 1.9" as version string, which is
	// not correctly parsed by Qt. We have to workaround this.
	if(OpenGLSceneRenderer::openGLVersion().startsWith("2.1 ")) {
		format.setMajorVersion(2);
		format.setMinorVersion(1);
	}

	if(format.majorVersion() < OVITO_OPENGL_MINIMUM_VERSION_MAJOR || (format.majorVersion() == OVITO_OPENGL_MINIMUM_VERSION_MAJOR && format.minorVersion() < OVITO_OPENGL_MINIMUM_VERSION_MINOR)) {
		// Avoid infinite recursion.
		static bool errorMessageShown = false;
		if(!errorMessageShown) {
			errorMessageShown = true;
			_viewport->dataset()->viewportConfig()->suspendViewportUpdates();
			Exception ex(tr(
					"The OpenGL graphics driver installed on this system does not support OpenGL version %6.%7 or newer.\n\n"
					"Ovito requires modern graphics hardware and up-to-date graphics drivers to display 3D content. Your current system configuration is not compatible with Ovito and the application will quit now.\n\n"
					"To avoid this error, please install the newest graphics driver of the hardware vendor or, if necessary, consider replacing your graphics card with a newer model.\n\n"
					"The installed OpenGL graphics driver reports the following information:\n\n"
					"OpenGL vendor: %1\n"
					"OpenGL renderer: %2\n"
					"OpenGL version: %3.%4 (%5)\n\n"
					"Ovito requires at least OpenGL version %6.%7.")
					.arg(QString(OpenGLSceneRenderer::openGLVendor()))
					.arg(QString(OpenGLSceneRenderer::openGLRenderer()))
					.arg(format.majorVersion())
					.arg(format.minorVersion())
					.arg(QString(OpenGLSceneRenderer::openGLVersion()))
					.arg(OVITO_OPENGL_MINIMUM_VERSION_MAJOR)
					.arg(OVITO_OPENGL_MINIMUM_VERSION_MINOR)
				);
			QCoreApplication::removePostedEvents(nullptr, 0);
			if(_mainWindow) _mainWindow->close();
			ex.reportError(true);
			QMetaObject::invokeMethod(QCoreApplication::instance(), "quit", Qt::QueuedConnection);
			QCoreApplication::exit();
		}
		return;
	}

	// Invalidate picking buffer every time the visible contents of the viewport change.
	_pickingRenderer->reset();

	if(!viewport()->dataset()->viewportConfig()->isSuspended()) {
		try {
			// Let the Viewport class do the actual rendering work.
			viewport()->renderInteractive(_viewportRenderer);
		}
		catch(Exception& ex) {
			if(ex.context() == nullptr) ex.setContext(viewport()->dataset());
			ex.prependGeneralMessage(tr("An unexpected error occurred while rendering the viewport contents. The program will quit."));
			viewport()->dataset()->viewportConfig()->suspendViewportUpdates();

			QString openGLReport;
			QTextStream stream(&openGLReport, QIODevice::WriteOnly | QIODevice::Text);
			stream << "OpenGL version: " << context()->format().majorVersion() << QStringLiteral(".") << context()->format().minorVersion() << endl;
			stream << "OpenGL profile: " << (context()->format().profile() == QSurfaceFormat::CoreProfile ? "core" : (context()->format().profile() == QSurfaceFormat::CompatibilityProfile ? "compatibility" : "none")) << endl;
			stream << "OpenGL vendor: " << QString(OpenGLSceneRenderer::openGLVendor()) << endl;
			stream << "OpenGL renderer: " << QString(OpenGLSceneRenderer::openGLRenderer()) << endl;
			stream << "OpenGL version string: " << QString(OpenGLSceneRenderer::openGLVersion()) << endl;
			stream << "OpenGL shading language: " << QString(OpenGLSceneRenderer::openGLSLVersion()) << endl;
			stream << "OpenGL shader programs: " << QOpenGLShaderProgram::hasOpenGLShaderPrograms() << endl;
			stream << "OpenGL geometry shaders: " << QOpenGLShader::hasOpenGLShaders(QOpenGLShader::Geometry, context()) << endl;
			stream << "Using point sprites: " << OpenGLSceneRenderer::pointSpritesEnabled() << endl;
			stream << "Using geometry shaders: " << OpenGLSceneRenderer::geometryShadersEnabled() << endl;
			stream << "Context sharing: " << OpenGLSceneRenderer::contextSharingEnabled() << endl;
			ex.appendDetailMessage(openGLReport);

			QCoreApplication::removePostedEvents(nullptr, 0);
			if(_mainWindow) _mainWindow->close();
			ex.reportError(true);
			QMetaObject::invokeMethod(QCoreApplication::instance(), "quit", Qt::QueuedConnection);
			QCoreApplication::exit();
		}
	}
	else {
		// When viewport updates are disabled, just clear the frame buffer with the background color.
		Color backgroundColor = Viewport::viewportColor(ViewportSettings::COLOR_VIEWPORT_BKG);
		context()->functions()->glClearColor(backgroundColor.r(), backgroundColor.g(), backgroundColor.b(), 1);
		context()->functions()->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		// Make sure viewport is refreshed once updates are enables again.
		_viewport->dataset()->viewportConfig()->updateViewports();
	}

	// If viewport updates are current disables, make sure the viewports will be refreshed
	// as soon as updates are enabled again.
	if(viewport()->dataset()->viewportConfig()->isSuspended()) {
		_viewport->dataset()->viewportConfig()->updateViewports();
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
