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
#include "ViewportImageGeometryBuffer.h"
#include "ViewportSceneRenderer.h"

#include <QGLWidget>

namespace Ovito {

IMPLEMENT_OVITO_OBJECT(Core, ViewportImageGeometryBuffer, ImageGeometryBuffer);

/******************************************************************************
* Constructor.
******************************************************************************/
ViewportImageGeometryBuffer::ViewportImageGeometryBuffer(ViewportSceneRenderer* renderer) :
	_contextGroup(QOpenGLContextGroup::currentContextGroup()),
	_texture(0),
	_needTextureUpdate(true)
{
	OVITO_ASSERT(renderer->glcontext()->shareGroup() == _contextGroup);

	// Initialize OpenGL shader.
	_shader = renderer->loadShaderProgram("image", ":/core/glsl/image.vertex.glsl", ":/core/glsl/image.fragment.glsl");

	// Create vertex buffer
	if(!_vertexBuffer.create())
		throw Exception(tr("Failed to create OpenGL vertex buffer."));
	_vertexBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
	if(!_vertexBuffer.bind())
			throw Exception(tr("Failed to bind OpenGL vertex buffer."));
	_vertexBuffer.allocate(4 * sizeof(Point2));
	_vertexBuffer.release();

	// Create OpenGL texture.
	glGenTextures(1, &_texture);

	// Make sure texture gets deleted again when this object is destroyed.
	attachOpenGLResources();
}

/******************************************************************************
* Destructor.
******************************************************************************/
ViewportImageGeometryBuffer::~ViewportImageGeometryBuffer()
{
	destroyOpenGLResources();
}

/******************************************************************************
* This method that takes care of freeing the shared OpenGL resources owned
* by this class.
******************************************************************************/
void ViewportImageGeometryBuffer::freeOpenGLResources()
{
	OVITO_CHECK_OPENGL(glDeleteTextures(1, &_texture));
	_texture = 0;
}

/******************************************************************************
* Returns true if the buffer is filled and can be rendered with the given renderer.
******************************************************************************/
bool ViewportImageGeometryBuffer::isValid(SceneRenderer* renderer)
{
	ViewportSceneRenderer* vpRenderer = qobject_cast<ViewportSceneRenderer*>(renderer);
	if(!vpRenderer) return false;
	return (_contextGroup == vpRenderer->glcontext()->shareGroup()) && (_texture != 0) && _vertexBuffer.isCreated();
}

/******************************************************************************
* Renders the image in a rectangle given in window coordinates.
******************************************************************************/
void ViewportImageGeometryBuffer::renderWindow(SceneRenderer* renderer, const Point2& pos, const Vector2& size)
{
	GLint vc[4];
	glGetIntegerv(GL_VIEWPORT, vc);

	// Transform rectangle to normalized device coordinates.
	renderViewport(renderer, Point2(pos.x() / vc[2] * 2 - 1, 1 - (pos.y() + size.y()) / vc[3] * 2),
		Vector2(size.x() / vc[2] * 2, size.y() / vc[3] * 2));
}

/******************************************************************************
* Renders the image in a rectangle given in viewport coordinates.
******************************************************************************/
void ViewportImageGeometryBuffer::renderViewport(SceneRenderer* renderer, const Point2& pos, const Vector2& size)
{
	OVITO_ASSERT(_contextGroup == QOpenGLContextGroup::currentContextGroup());
	OVITO_ASSERT(_texture != 0);
	OVITO_STATIC_ASSERT(sizeof(FloatType) == sizeof(float) && sizeof(Point2) == sizeof(float)*2);
	ViewportSceneRenderer* vpRenderer = dynamic_object_cast<ViewportSceneRenderer>(renderer);

	if(image().isNull() || !vpRenderer)
		return;

	// Prepare texture.
	OVITO_CHECK_OPENGL(glBindTexture(GL_TEXTURE_2D, _texture));
	vpRenderer->glfuncs()->glActiveTexture(GL_TEXTURE0);

	if(_needTextureUpdate) {
		_needTextureUpdate = false;

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

		// Upload texture data.
		QImage textureImage = QGLWidget::convertToGLFormat(image());
		OVITO_CHECK_OPENGL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureImage.width(), textureImage.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, textureImage.constBits()));
	}

	// Transform rectangle to normalized device coordinates.
	Point2 corners[4] = {
			Point2(pos.x(), pos.y()),
			Point2(pos.x() + size.x(), pos.y()),
			Point2(pos.x(), pos.y() + size.y()),
			Point2(pos.x() + size.x(), pos.y() + size.y())
	};

	bool wasDepthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
	bool wasBlendEnabled = glIsEnabled(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if(!_shader->bind())
		throw Exception(tr("Failed to bind OpenGL shader."));

	if(!_vertexBuffer.bind())
			throw Exception(tr("Failed to bind OpenGL vertex buffer."));

	_vertexBuffer.write(0, corners, 4 * sizeof(Point2));
	_shader->setAttributeBuffer("vertex_pos", GL_FLOAT, 0, 2);
	_shader->enableAttributeArray("vertex_pos");

	OVITO_CHECK_OPENGL(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

	_vertexBuffer.release();
	_shader->release();

	// Restore old state.
	if(wasDepthTestEnabled) glEnable(GL_DEPTH_TEST);
	if(!wasBlendEnabled) glDisable(GL_BLEND);
}

};