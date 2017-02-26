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
#include <core/viewport/Viewport.h>
#include <core/viewport/ViewportConfiguration.h>
#include <core/rendering/RenderSettings.h>
#include <core/app/Application.h>
#include "StandardSceneRenderer.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Rendering)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(StandardSceneRenderer, OpenGLSceneRenderer);
DEFINE_PROPERTY_FIELD(StandardSceneRenderer, antialiasingLevel, "AntialiasingLevel");
SET_PROPERTY_FIELD_LABEL(StandardSceneRenderer, antialiasingLevel, "Antialiasing level");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(StandardSceneRenderer, antialiasingLevel, IntegerParameterUnit, 1, 6);

/******************************************************************************
* Prepares the renderer for rendering and sets the data set that is being rendered.
******************************************************************************/
bool StandardSceneRenderer::startRender(DataSet* dataset, RenderSettings* settings)
{
	if(Application::instance()->headlessMode())
		throwException(tr("Cannot use OpenGL renderer when running in headless mode. "
				"Please use a different rendering engine or run program on a machine where access to "
				"graphics hardware is possible."));

	if(!OpenGLSceneRenderer::startRender(dataset, settings))
		return false;

	int sampling = std::max(1, antialiasingLevel());

	if(Application::instance()->guiMode()) {
		// Create a temporary OpenGL context for rendering to an offscreen buffer.
		_offscreenContext.reset(new QOpenGLContext());
		_offscreenContext->setFormat(OpenGLSceneRenderer::getDefaultSurfaceFormat());
		// It should share its resources with the viewport renderer.
		const QVector<Viewport*>& viewports = renderDataset()->viewportConfig()->viewports();
		if(!viewports.empty() && viewports.front()->window())
			_offscreenContext->setShareContext(viewports.front()->window()->glcontext());
		if(!_offscreenContext->create())
			throwException(tr("Failed to create OpenGL context for rendering."));
	}
	else {
		// Create new OpenGL context for rendering in console mode.
		OVITO_ASSERT(QOpenGLContext::currentContext() == nullptr);
		_offscreenContext.reset(new QOpenGLContext());
		_offscreenContext->setFormat(OpenGLSceneRenderer::getDefaultSurfaceFormat());
		if(!_offscreenContext->create())
			throwException(tr("Failed to create OpenGL context for rendering."));
	}

	// Create offscreen buffer.
	if(_offscreenSurface.isNull())
		_offscreenSurface.reset(new QOffscreenSurface());
	_offscreenSurface->setFormat(_offscreenContext->format());
	_offscreenSurface->create();
	if(!_offscreenSurface->isValid())
		throwException(tr("Failed to create offscreen rendering surface."));

	// Make the context current.
	if(!_offscreenContext->makeCurrent(_offscreenSurface.data()))
		throwException(tr("Failed to make OpenGL context current."));

	// Create OpenGL framebuffer.
	_framebufferSize = QSize(settings->outputImageWidth() * sampling, settings->outputImageHeight() * sampling);
	QOpenGLFramebufferObjectFormat framebufferFormat;
	framebufferFormat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
	_framebufferObject.reset(new QOpenGLFramebufferObject(_framebufferSize.width(), _framebufferSize.height(), framebufferFormat));
	if(!_framebufferObject->isValid())
		throwException(tr("Failed to create OpenGL framebuffer object for offscreen rendering."));

	// Bind OpenGL buffer.
	if(!_framebufferObject->bind())
		throwException(tr("Failed to bind OpenGL framebuffer object for offscreen rendering."));

	return true;
}

/******************************************************************************
* This method is called just before renderFrame() is called.
******************************************************************************/
void StandardSceneRenderer::beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp)
{
	// Make GL context current.
	if(!_offscreenContext || !_offscreenContext->makeCurrent(_offscreenSurface.data()))
		throwException(tr("Failed to make OpenGL context current."));

	OpenGLSceneRenderer::beginFrame(time, params, vp);

	// Setup GL viewport.
	setRenderingViewport(0, 0, _framebufferSize.width(), _framebufferSize.height());

	// Set rendering background color.
	if(!renderSettings()->generateAlphaChannel()) {
		Color backgroundColor = renderSettings()->backgroundColor();
		setClearColor(ColorA(backgroundColor));
	}
	else setClearColor(ColorA(0, 0, 0, 0));
}

/******************************************************************************
* Renders the current animation frame.
******************************************************************************/
bool StandardSceneRenderer::renderFrame(FrameBuffer* frameBuffer, StereoRenderingTask stereoTask, TaskManager& taskManager)
{
	// Let the base class do the main rendering work.
	if(!OpenGLSceneRenderer::renderFrame(frameBuffer, stereoTask, taskManager))
		return false;

	// Flush the contents to the FBO before extracting image.
	glcontext()->swapBuffers(_offscreenSurface.data());

	// Fetch rendered image from OpenGL framebuffer.
    QImage bufferImage = _framebufferObject->toImage();

	// Scale it down to the output size.
	QImage image = bufferImage.scaled(frameBuffer->image().width(), frameBuffer->image().height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

	// Copy OpenGL image to the output frame buffer.
	frameBuffer->image() = image;
	frameBuffer->update();

	return true;
}

/******************************************************************************
* Is called after rendering has finished.
******************************************************************************/
void StandardSceneRenderer::endRender()
{
	QOpenGLFramebufferObject::bindDefault();
	QOpenGLContext* ctxt = QOpenGLContext::currentContext();
	if(ctxt) ctxt->doneCurrent();
	_framebufferObject.reset();
	_offscreenContext.reset();
	_offscreenSurface.reset();
	OpenGLSceneRenderer::endRender();
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
