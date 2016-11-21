///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
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
#include <core/rendering/FrameBuffer.h>
#include <core/rendering/RenderSettings.h>
#include <core/scene/ObjectNode.h>
#include <core/utilities/concurrent/ProgressDisplay.h>
#include "POVRayRenderer.h"

namespace Ovito { namespace POVRay {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(POVRay, POVRayRenderer, NonInteractiveSceneRenderer);

/******************************************************************************
* Constructor.
******************************************************************************/
POVRayRenderer::POVRayRenderer(DataSet* dataset) : NonInteractiveSceneRenderer(dataset)
{
}

/******************************************************************************
* Prepares the renderer for rendering of the given scene.
******************************************************************************/
bool POVRayRenderer::startRender(DataSet* dataset, RenderSettings* settings)
{
	if(!NonInteractiveSceneRenderer::startRender(dataset, settings))
		return false;

	return true;
}

/******************************************************************************
* This method is called just before renderFrame() is called.
* Sets the view projection parameters, the animation frame to render,
* and the viewport who is being rendered.
******************************************************************************/
void POVRayRenderer::beginFrame(TimePoint time, const ViewProjectionParameters& params, Viewport* vp)
{
	NonInteractiveSceneRenderer::beginFrame(time, params, vp);

	_outputStream << "#version 3.5;\n";
	_outputStream << "#include \"transforms.inc\"\n";

#if 0
	_outputStream << "global_settings {\n";
	_outputStream << "radiosity {\n";
	_outputStream << "count " << writer.renderer()->_radiosityRayCount->getValueAtTime(writer.time()) << "\n";
	_outputStream << "recursion_limit " << writer.renderer()->_radiosityRecursionLimit->getValueAtTime(writer.time()) << "\n";
	_outputStream << "error_bound " << writer.renderer()->_radiosityErrorBound->getValueAtTime(writer.time()) << "\n";
	_outputStream << "}\n";
	_outputStream << "}\n";
#endif

	// Write background.
	TimeInterval iv;
	Color backgroundColor;
	renderSettings()->backgroundColorController()->getColorValue(time, backgroundColor, iv);
	_outputStream << "background { color "; write(backgroundColor); _outputStream << "}\n";

	// Write camera.
	_outputStream << "camera {\n";
	// Write projection transformation
	if(projParams().isPerspective) {
		_outputStream << "  perspective\n";

		Point3 p0 = projParams().inverseProjectionMatrix * Point3(0,0,0);
		Point3 px = projParams().inverseProjectionMatrix * Point3(1,0,0);
		Point3 py = projParams().inverseProjectionMatrix * Point3(0,1,0);
		Point3 lookat = projParams().inverseProjectionMatrix * Point3(0,0,0);
		Vector3 direction = lookat - Point3::Origin();
		Vector3 right = px - p0;
		Vector3 up = right.cross(direction).normalized();
		right = direction.cross(up).normalized() * (up.length() / projParams().aspectRatio);

		_outputStream << "  location <0, 0, 0>\n";
		_outputStream << "  direction "; write(direction.normalized()); _outputStream << "\n";
		_outputStream << "  right "; write(right); _outputStream << "\n";
		_outputStream << "  up "; write(up); _outputStream << "\n";
		_outputStream << "  angle " << (atan(tan(projParams().fieldOfView * 0.5) / projParams().aspectRatio) * 2.0 * 180.0 / FLOATTYPE_PI) << "\n";
	}
	else {
		_outputStream << "  orthographic\n";

		Point3 px = projParams().inverseProjectionMatrix * Point3(1,0,0);
		Point3 py = projParams().inverseProjectionMatrix * Point3(0,1,0);
		Vector3 direction = projParams().inverseProjectionMatrix * Point3(0,0,1) - Point3::Origin();
		Vector3 up = (py - Point3::Origin()) * 2;
		Vector3 right = px - Point3::Origin();

		right = direction.cross(up).normalized() * (up.length() / projParams().aspectRatio);

		_outputStream << "  location "; write(-(direction*2)); _outputStream << "\n";
		_outputStream << "  direction "; write(direction); _outputStream << "\n";
		_outputStream << "  right "; write(right); _outputStream << "\n";
		_outputStream << "  up "; write(up); _outputStream << "\n";
		_outputStream << "  sky "; write(up); _outputStream << "\n";
		_outputStream << "  look_at "; write(-direction); _outputStream << "\n";
	}
	// Write camera transformation.
	Rotation rot(projParams().viewMatrix);
	_outputStream << "  Axis_Rotate_Trans("; write(rot.axis()); _outputStream << ", " << (rot.angle() * 180.0f / FLOATTYPE_PI) << ")\n";
	_outputStream << "  translate "; write(projParams().inverseViewMatrix.translation()); _outputStream << "\n";
	_outputStream << "}\n";

	// Write a light source.
	_outputStream << "light_source {\n";
	_outputStream << "  <0, 0, 0>\n";
	_outputStream << "  color <1.5, 1.5, 1.5>\n";
	_outputStream << "  parallel\n";
	_outputStream << "  shadowless\n";
	_outputStream << "  point_at "; write(projParams().inverseViewMatrix * Vector3(0,0,-1)); _outputStream << "\n";
	_outputStream << "}\n";

	// Define macro for particle primitives in the POV-Ray file to reduce file size.
	_outputStream << "#macro SPRTCLE(pos, particleRadius, particleColor)\n";
	_outputStream << "sphere { pos, particleRadius\n";
	_outputStream << "         texture { pigment { color particleColor } }\n";
	_outputStream << "}\n";
	_outputStream << "#end\n";
	_outputStream << "#macro BPRTCLE(pos, particleRadius, particleColor)\n";
	_outputStream << "box { pos - <particleRadius,particleRadius,particleRadius>, pos + <particleRadius,particleRadius,particleRadius>\n";
	_outputStream << "         texture { pigment { color particleColor } }\n";
	_outputStream << "}\n";
	_outputStream << "#end\n";
	
}

/******************************************************************************
* Renders a single animation frame into the given frame buffer.
******************************************************************************/
bool POVRayRenderer::renderFrame(FrameBuffer* frameBuffer, StereoRenderingTask stereoTask, AbstractProgressDisplay* progress)
{
	if(progress) progress->setStatusText(tr("Preparing scene"));

	return (!progress || progress->wasCanceled() == false);
}

/******************************************************************************
* This method is called after renderFrame() has been called.
******************************************************************************/
void POVRayRenderer::endFrame()
{
	NonInteractiveSceneRenderer::endFrame();
}

/******************************************************************************
* Finishes the rendering pass. This is called after all animation frames have been rendered
* or when the rendering operation has been aborted.
******************************************************************************/
void POVRayRenderer::endRender()
{
	// Release 2d draw call buffers.
	_imageDrawCalls.clear();
	_textDrawCalls.clear();

	NonInteractiveSceneRenderer::endRender();
}

/******************************************************************************
* Renders the line geometry stored in the given buffer.
******************************************************************************/
void POVRayRenderer::renderLines(const DefaultLinePrimitive& lineBuffer)
{
	// Lines are not supported by this renderer.
}

/******************************************************************************
* Renders the particles stored in the given buffer.
******************************************************************************/
void POVRayRenderer::renderParticles(const DefaultParticlePrimitive& particleBuffer)
{
	auto p = particleBuffer.positions().cbegin();
	auto p_end = particleBuffer.positions().cend();
	auto c = particleBuffer.colors().cbegin();
	auto r = particleBuffer.radii().cbegin();

	const AffineTransformation tm = modelTM();

	if(particleBuffer.particleShape() == ParticlePrimitive::SphericalShape) {
		// Rendering spherical particles.
		for(; p != p_end; ++p, ++c, ++r) {
			if(c->a() > 0) {
				_outputStream << "SPRTCLE("; write(tm * (*p)); 
				_outputStream << ", " << (*r) << ", "; write(*c);
				_outputStream << ")\n";
			}
		}
	}
	else if(particleBuffer.particleShape() == ParticlePrimitive::SquareShape) {
		// Rendering cubic particles.
		for(; p != p_end; ++p, ++c, ++r) {
			if(c->a() > 0) {
				_outputStream << "BPRTCLE("; write(tm * (*p)); 
				_outputStream << ", " << (*r) << ", "; write(*c);
				_outputStream << ")\n";
			}
		}
	}
	else throwException(tr("Particle shape not supported by POV-Ray renderer: %1").arg(particleBuffer.particleShape()));
}

/******************************************************************************
* Renders the arrow elements stored in the given buffer.
******************************************************************************/
void POVRayRenderer::renderArrows(const DefaultArrowPrimitive& arrowBuffer)
{
}

/******************************************************************************
* Renders the text stored in the given buffer.
******************************************************************************/
void POVRayRenderer::renderText(const DefaultTextPrimitive& textBuffer, const Point2& pos, int alignment)
{
	_textDrawCalls.push_back(std::make_tuple(textBuffer.text(), textBuffer.color(), textBuffer.font(), pos, alignment));
}

/******************************************************************************
* Renders the image stored in the given buffer.
******************************************************************************/
void POVRayRenderer::renderImage(const DefaultImagePrimitive& imageBuffer, const Point2& pos, const Vector2& size)
{
	_imageDrawCalls.push_back(std::make_tuple(imageBuffer.image(), pos, size));
}

/******************************************************************************
* Renders the triangle mesh stored in the given buffer.
******************************************************************************/
void POVRayRenderer::renderMesh(const DefaultMeshPrimitive& meshBuffer)
{
}

}	// End of namespace
}	// End of namespace

