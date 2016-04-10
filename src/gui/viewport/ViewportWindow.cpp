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
ViewportWindow::ViewportWindow(Viewport* owner, QWidget* parentWidget) :
#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
		QOpenGLWidget(parentWidget),
#endif
		_viewport(owner), _updateRequested(false),
		_mainWindow(MainWindow::fromDataset(owner->dataset())),
		_renderDebugCounter(0), _cursorInContextMenuArea(false)
{
	// Associate the viewport with this window.
	owner->setWindow(this);

#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
	_updatePending = false;

	if(contextSharingEnabled()) {
		// Get the master OpenGL context, which is managed by the main window.
		OVITO_CHECK_POINTER(_mainWindow);
		_context = _mainWindow->getOpenGLContext();
	}
	else {
		// Create a dedicated OpenGL context for this viewport window.
		// All contexts still share OpenGL resources.
		_context = new QOpenGLContext(this);
		_context->setFormat(ViewportSceneRenderer::getDefaultSurfaceFormat());
		_context->setShareContext(_mainWindow->getOpenGLContext());
		if(!_context->create())
			throw Exception(tr("Failed to create OpenGL context."));
	}

	// Indicate that the window is to be used for OpenGL rendering.
	setSurfaceType(QWindow::OpenGLSurface);
	setFormat(_context->format());
#else
	setMouseTracking(true);
#endif

	// Determine OpenGL vendor string so other parts of the code can decide
	// which OpenGL features are save to use.
	OpenGLSceneRenderer::determineOpenGLInfo();

#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
	// Create a QWidget for this QWindow.
	_widget = QWidget::createWindowContainer(this, parentWidget);
	_widget->setAttribute(Qt::WA_DeleteOnClose);
#endif

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
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
	// If not already done so, put an update request event on the event loop,
	// which leads to renderNow() being called once the event gets processed.
	if(!_updatePending) {
		_updatePending = true;
		QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateLater));
	}
#else
	update();
#endif
}

/******************************************************************************
* If an update request is pending for this viewport window, immediately
* processes it and redraw the window contents.
******************************************************************************/
void ViewportWindow::processViewportUpdate()
{
	if(_updateRequested) {
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
		renderNow();
#else
		OVITO_ASSERT_MSG(!_viewport->isRendering(), "ViewportWindow::processUpdateRequest()", "Recursive viewport repaint detected.");
		OVITO_ASSERT_MSG(!_viewport->dataset()->viewportConfig()->isRendering(), "ViewportWindow::processUpdateRequest()", "Recursive viewport repaint detected.");
		repaint();
#endif
	}
}

/******************************************************************************
* Displays the context menu for this viewport.
******************************************************************************/
void ViewportWindow::showViewportMenu(const QPoint& pos)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
	requestActivate();
#endif

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
	QPointF pos = QPointF(2, 2) * devicePixelRatio();
	_contextMenuArea = QRect(0, 0, std::max(metrics.width(_captionBuffer->text()), 30.0) + pos.x(), metrics.height() + pos.y());
	_captionBuffer->renderWindow(_viewportRenderer, Point2(pos.x(), pos.y()), Qt::AlignLeft | Qt::AlignTop);
}

/******************************************************************************
* Sets whether mouse grab should be enabled or not for this viewport window.
******************************************************************************/
bool ViewportWindow::setMouseGrabEnabled(bool grab)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
	return QWindow::setMouseGrabEnabled(grab);
#else
	if(grab) {
		grabMouse();
		return true;
	}
	else {
		releaseMouse();
		return false;
	}
#endif
}

/******************************************************************************
* Sets the cursor shape for this viewport window.
******************************************************************************/
void ViewportWindow::setCursor(const QCursor& cursor)
{
	// Changing the cursor leads to program crash on MacOS and Qt <= 5.2.0.
#if !defined(Q_OS_MACX) || (QT_VERSION >= QT_VERSION_CHECK(5, 2, 1))
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
	QWindow::setCursor(cursor);
#else
	QOpenGLWidget::setCursor(cursor);
#endif
#endif
}

/******************************************************************************
* Restores the default arrow cursor for this viewport window.
******************************************************************************/
void ViewportWindow::unsetCursor()
{
#if !defined(Q_OS_MACX) || (QT_VERSION >= QT_VERSION_CHECK(5, 2, 1))
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
	QWindow::unsetCursor();
#else
	QOpenGLWidget::unsetCursor();
#endif
#endif
}

/******************************************************************************
* Render the axis tripod symbol in the corner of the viewport that indicates
* the coordinate system orientation.
******************************************************************************/
void ViewportWindow::renderOrientationIndicator()
{
	const FloatType tripodSize = FloatType(60) * devicePixelRatio();			// pixels
	const FloatType tripodArrowSize = FloatType(0.17); 	// percentage of the above value.

	// Turn off depth-testing.
	OVITO_CHECK_OPENGL(_viewportRenderer->glfuncs()->glDisable(GL_DEPTH_TEST));

	// Setup projection matrix.
	ViewProjectionParameters projParams = viewport()->projectionParams();
	FloatType xscale = size().width() / tripodSize;
	FloatType yscale = size().height() / tripodSize;
	projParams.projectionMatrix = Matrix4::translation(Vector3(-1.0 + 1.3f/xscale, -1.0 + 1.3f/yscale, 0))
									* Matrix4::ortho(-xscale, xscale, -yscale, yscale, -2, 2);
	projParams.inverseProjectionMatrix = projParams.projectionMatrix.inverse();
	projParams.viewMatrix.setIdentity();
	projParams.inverseViewMatrix.setIdentity();
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
		Point2 windowPoint(( ndcPoint.x() + 1.0) * size().width()  / 2,
							(-ndcPoint.y() + 1.0) * size().height() / 2);
		_orientationTripodLabels[axis]->renderWindow(_viewportRenderer, windowPoint, Qt::AlignHCenter | Qt::AlignVCenter);
	}

	// Restore old rendering attributes.
	OVITO_CHECK_OPENGL(_viewportRenderer->glfuncs()->glEnable(GL_DEPTH_TEST));
}

/******************************************************************************
* Renders the frame on top of the scene that indicates the visible rendering area.
******************************************************************************/
void ViewportWindow::renderRenderFrame()
{
	// Create a rendering buffer that is responsible for rendering the frame.
	if(!_renderFrameOverlay || !_renderFrameOverlay->isValid(_viewportRenderer)) {
		_renderFrameOverlay = _viewportRenderer->createImagePrimitive();
		QImage image(1, 1, QImage::Format_ARGB32_Premultiplied);
		image.fill(0xA0FFFFFF);
		_renderFrameOverlay->setImage(image);
	}

	Box2 rect = viewport()->renderFrameRect();

	// Render rectangle borders
	_renderFrameOverlay->renderViewport(_viewportRenderer, Point2(-1,-1), Vector2(1.0 + rect.minc.x(), 2));
	_renderFrameOverlay->renderViewport(_viewportRenderer, Point2(rect.maxc.x(),-1), Vector2(1.0 - rect.maxc.x(), 2));
	_renderFrameOverlay->renderViewport(_viewportRenderer, Point2(rect.minc.x(),-1), Vector2(rect.width(), 1.0 + rect.minc.y()));
	_renderFrameOverlay->renderViewport(_viewportRenderer, Point2(rect.minc.x(),rect.maxc.y()), Vector2(rect.width(), 1.0 - rect.maxc.y()));
}

/******************************************************************************
* Determines the object that is visible under the given mouse cursor position.
******************************************************************************/
ViewportPickResult ViewportWindow::pick(const QPointF& pos)
{
	// Cannot perform picking while viewport is not visible or currently rendering.
	if(!isExposed() || viewport()->isRendering()) {
		ViewportPickResult result;
		result.valid = false;
		return result;
	}

	try {
		if(_pickingRenderer->isRefreshRequired()) {
			// Let the viewport do the actual rendering work.
			viewport()->renderInteractive(_pickingRenderer);
		}

		// Query which object is located at the given window position.
		ViewportPickResult result;
		const PickingSceneRenderer::ObjectRecord* objInfo;
		std::tie(objInfo, result.subobjectId) = _pickingRenderer->objectAtLocation((pos * devicePixelRatio()).toPoint());
		result.valid = (objInfo != nullptr);
		if(objInfo) {
			result.objectNode = objInfo->objectNode;
			result.pickInfo = objInfo->pickInfo;
			result.worldPosition = _pickingRenderer->worldPositionFromLocation((pos * devicePixelRatio()).toPoint());
		}
		return result;
	}
	catch(const Exception& ex) {
		ex.showError();
		ViewportPickResult result;
		result.valid = false;
		return result;
	}
}

#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)

/******************************************************************************
* This internal method receives events to the viewport window.
******************************************************************************/
bool ViewportWindow::event(QEvent* event)
{
	// Handle update request events posted by renderLater().
	if(event->type() == QEvent::UpdateLater) {
		_updatePending = false;
		processViewportUpdate();
		return true;
	}
	return QWindow::event(event);
}

/******************************************************************************
* Handles the expose events.
******************************************************************************/
void ViewportWindow::exposeEvent(QExposeEvent*)
{
	if(isExposed()) {
		renderNow();
	}
}

/******************************************************************************
* Handles the resize events.
******************************************************************************/
void ViewportWindow::resizeEvent(QResizeEvent*)
{
	if(isExposed()) {
		renderNow();
	}
}

#else
	
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

#endif

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
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
		startTimer(0);
#endif
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

#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)

/******************************************************************************
* Is called in periodic intervals.
******************************************************************************/
void ViewportWindow::timerEvent(QTimerEvent* event)
{
	if(_contextMenuArea.contains(mapFromGlobal(QCursor::pos())))
		return;

	if(_cursorInContextMenuArea) {
		_cursorInContextMenuArea = false;
		viewport()->updateViewport();
	}
	killTimer(event->timerId());
}

#else

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

#endif

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
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
	if(!isExposed())
		return;
#endif

	_updateRequested = false;

	// Do not re-enter rendering function of the same viewport.
	if(!viewport() || viewport()->isRendering())
		return;

#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
	// Before making our GL context current, remember the old context that
	// is currently active so we can restore it when we are done.
	// This is necessary, because multiple viewport repaint requests can be
	// processed simultaneously.
	QPointer<QOpenGLContext> oldContext = QOpenGLContext::currentContext();
	QSurface* oldSurface = oldContext ? oldContext->surface() : nullptr;

	if(!_context->makeCurrent(this)) {
		qWarning() << "ViewportWindow::renderNow(): Failed to make OpenGL context current.";
		return;
	}
#endif
	OVITO_REPORT_OPENGL_ERRORS();

	QSurfaceFormat format = context()->format();
	// OpenGL in a VirtualBox machine Windows guest reports "2.1 Chromium 1.9" as version string, which is
	// not correctly parsed by Qt. We have to workaround this.
	if(qstrncmp((const char*)context()->functions()->glGetString(GL_VERSION), "2.1 ", 4) == 0) {
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
					.arg(QString((const char*)context()->functions()->glGetString(GL_VENDOR)))
					.arg(QString((const char*)context()->functions()->glGetString(GL_RENDERER)))
					.arg(format.majorVersion())
					.arg(format.minorVersion())
					.arg(QString((const char*)context()->functions()->glGetString(GL_VERSION)))
					.arg(OVITO_OPENGL_MINIMUM_VERSION_MAJOR)
					.arg(OVITO_OPENGL_MINIMUM_VERSION_MINOR)
				);
			ex.showError();
			QCoreApplication::removePostedEvents(nullptr, 0);
			QCoreApplication::instance()->quit();
		}
		return;
	}

	OVITO_REPORT_OPENGL_ERRORS();
	if(!viewport()->dataset()->viewportConfig()->isSuspended()) {

		// Invalidate picking buffer every time the visible contents of the viewport change.
		_pickingRenderer->reset();

		QSize vpSize = viewportWindowSize();
		if(!vpSize.isEmpty()) {

			// Set up OpenGL viewport.
			context()->functions()->glViewport(0, 0, vpSize.width(), vpSize.height());

			try {
				// Let the Viewport class do the actual rendering work.
				viewport()->renderInteractive(_viewportRenderer);
			}
			catch(Exception& ex) {
				if(ex.context() == nullptr) ex.setContext(viewport()->dataset());
				ex.prependGeneralMessage(tr("An unexpected error occurred while rendering the viewport contents. The program will quit."));

				QString openGLReport;
				QTextStream stream(&openGLReport, QIODevice::WriteOnly | QIODevice::Text);
				stream << "OpenGL version: " << context()->format().majorVersion() << QStringLiteral(".") << context()->format().minorVersion() << endl;
				stream << "OpenGL profile: " << (context()->format().profile() == QSurfaceFormat::CoreProfile ? "core" : (context()->format().profile() == QSurfaceFormat::CompatibilityProfile ? "compatibility" : "none")) << endl;
				stream << "OpenGL vendor: " << QString((const char*)context()->functions()->glGetString(GL_VENDOR)) << endl;
				stream << "OpenGL renderer: " << QString((const char*)context()->functions()->glGetString(GL_RENDERER)) << endl;
				stream << "OpenGL version string: " << QString((const char*)context()->functions()->glGetString(GL_VERSION)) << endl;
				stream << "OpenGL shading language: " << QString((const char*)context()->functions()->glGetString(GL_SHADING_LANGUAGE_VERSION)) << endl;
				stream << "OpenGL shader programs: " << QOpenGLShaderProgram::hasOpenGLShaderPrograms() << endl;
				stream << "OpenGL geometry shaders: " << QOpenGLShader::hasOpenGLShaders(QOpenGLShader::Geometry, context()) << endl;
				stream << "Using point sprites: " << OpenGLSceneRenderer::pointSpritesEnabled() << endl;
				stream << "Using geometry shaders: " << OpenGLSceneRenderer::geometryShadersEnabled() << endl;
				stream << "Context sharing: " << OpenGLSceneRenderer::contextSharingEnabled() << endl;
				ex.appendDetailMessage(openGLReport);

				viewport()->dataset()->viewportConfig()->suspendViewportUpdates();
				QCoreApplication::removePostedEvents(nullptr, 0);
				ex.showError();
				QCoreApplication::instance()->quit();
			}
		}
	}
	else {
		Color backgroundColor = Viewport::viewportColor(ViewportSettings::COLOR_VIEWPORT_BKG);
		context()->functions()->glClearColor(backgroundColor.r(), backgroundColor.g(), backgroundColor.b(), 1);
		context()->functions()->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		_viewport->dataset()->viewportConfig()->updateViewports();
	}
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
	context()->swapBuffers(this);
#endif

	OVITO_REPORT_OPENGL_ERRORS();

#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
	// Restore old GL context.
	if(oldSurface && oldContext) {
		if(!oldContext->makeCurrent(oldSurface))
			qWarning() << "ViewportWindow::renderNow(): Failed to restore old OpenGL context.";
	}
	else {
		_context->doneCurrent();
	}
#endif
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
