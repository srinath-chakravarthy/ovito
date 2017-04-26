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
#include <core/rendering/FrameBuffer.h>
#include <core/rendering/RenderSettings.h>
#include <core/reference/CloneHelper.h>
#include <core/scene/ObjectNode.h>
#include <core/utilities/concurrent/Task.h>
#include "TachyonRenderer.h"

extern "C" {

#include <tachyon/render.h>
#include <tachyon/camera.h>
#include <tachyon/threads.h>
#include <tachyon/trace.h>

};

#if TACHYON_MAJOR_VERSION <= 0 && TACHYON_MINOR_VERSION < 99
	#error "The OVITO Tachyon plugin requires version 0.99 or newer of the Tachyon library."
#endif

namespace Ovito { namespace Tachyon {

// Helper function that converts an OVITO vector to a Tachyon vector.
template<typename T>
inline apivector tvec(const Vector_3<T>& v) {
	return rt_vector(v.x(), v.y(), -v.z());
}

// Helper function that converts an OVITO point to a Tachyon vector.
template<typename T>
inline apivector tvec(const Point_3<T>& p) {
	return rt_vector(p.x(), p.y(), -p.z());
}

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(TachyonRenderer, NonInteractiveSceneRenderer);
DEFINE_FLAGS_PROPERTY_FIELD(TachyonRenderer, antialiasingEnabled, "EnableAntialiasing", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(TachyonRenderer, directLightSourceEnabled, "EnableDirectLightSource", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(TachyonRenderer, shadowsEnabled, "EnableShadows", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(TachyonRenderer, antialiasingSamples, "AntialiasingSamples", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(TachyonRenderer, defaultLightSourceIntensity, "DefaultLightSourceIntensity", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(TachyonRenderer, ambientOcclusionEnabled, "EnableAmbientOcclusion", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(TachyonRenderer, ambientOcclusionSamples, "AmbientOcclusionSamples", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(TachyonRenderer, ambientOcclusionBrightness, "AmbientOcclusionBrightness", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(TachyonRenderer, depthOfFieldEnabled, "DepthOfFieldEnabled", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(TachyonRenderer, dofFocalLength, "DOFFocalLength", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(TachyonRenderer, dofAperture, "DOFAperture", PROPERTY_FIELD_MEMORIZE);
SET_PROPERTY_FIELD_LABEL(TachyonRenderer, antialiasingEnabled, "Enable anti-aliasing");
SET_PROPERTY_FIELD_LABEL(TachyonRenderer, antialiasingSamples, "Anti-aliasing samples");
SET_PROPERTY_FIELD_LABEL(TachyonRenderer, directLightSourceEnabled, "Direct light");
SET_PROPERTY_FIELD_LABEL(TachyonRenderer, shadowsEnabled, "Shadows");
SET_PROPERTY_FIELD_LABEL(TachyonRenderer, defaultLightSourceIntensity, "Direct light intensity");
SET_PROPERTY_FIELD_LABEL(TachyonRenderer, ambientOcclusionEnabled, "Ambient occlusion");
SET_PROPERTY_FIELD_LABEL(TachyonRenderer, ambientOcclusionSamples, "Ambient occlusion samples");
SET_PROPERTY_FIELD_LABEL(TachyonRenderer, ambientOcclusionBrightness, "Ambient occlusion brightness");
SET_PROPERTY_FIELD_LABEL(TachyonRenderer, depthOfFieldEnabled, "Depth of field");
SET_PROPERTY_FIELD_LABEL(TachyonRenderer, dofFocalLength, "Focal length");
SET_PROPERTY_FIELD_LABEL(TachyonRenderer, dofAperture, "Aperture");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(TachyonRenderer, antialiasingSamples, IntegerParameterUnit, 1, 500);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(TachyonRenderer, defaultLightSourceIntensity, FloatParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(TachyonRenderer, ambientOcclusionBrightness, FloatParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(TachyonRenderer, dofFocalLength, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(TachyonRenderer, dofAperture, FloatParameterUnit, 0, 1);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(TachyonRenderer, ambientOcclusionSamples, IntegerParameterUnit, 1, 100);

/******************************************************************************
* Default constructor.
******************************************************************************/
TachyonRenderer::TachyonRenderer(DataSet* dataset) : NonInteractiveSceneRenderer(dataset),
		_antialiasingEnabled(true), _directLightSourceEnabled(true), _shadowsEnabled(true),
	  _antialiasingSamples(12), _ambientOcclusionEnabled(true), _ambientOcclusionSamples(12),
	  _defaultLightSourceIntensity(0.90f), _ambientOcclusionBrightness(0.80f), _depthOfFieldEnabled(false),
	  _dofFocalLength(40), _dofAperture(1e-2f)
{
	INIT_PROPERTY_FIELD(antialiasingEnabled);
	INIT_PROPERTY_FIELD(antialiasingSamples);
	INIT_PROPERTY_FIELD(directLightSourceEnabled);
	INIT_PROPERTY_FIELD(shadowsEnabled);
	INIT_PROPERTY_FIELD(defaultLightSourceIntensity);
	INIT_PROPERTY_FIELD(ambientOcclusionEnabled);
	INIT_PROPERTY_FIELD(ambientOcclusionSamples);
	INIT_PROPERTY_FIELD(ambientOcclusionBrightness);
	INIT_PROPERTY_FIELD(depthOfFieldEnabled);
	INIT_PROPERTY_FIELD(dofFocalLength);
	INIT_PROPERTY_FIELD(dofAperture);
}

/******************************************************************************
* Prepares the renderer for rendering of the given scene.
******************************************************************************/
bool TachyonRenderer::startRender(DataSet* dataset, RenderSettings* settings)
{
	if(!NonInteractiveSceneRenderer::startRender(dataset, settings))
		return false;

	rt_initialize(0, NULL);

	return true;
}

/******************************************************************************
* Renders a single animation frame into the given frame buffer.
******************************************************************************/
bool TachyonRenderer::renderFrame(FrameBuffer* frameBuffer, StereoRenderingTask stereoTask, TaskManager& taskManager)
{
	SynchronousTask renderTask(taskManager);
	renderTask.setProgressText(tr("Handing scene data to Tachyon renderer"));

	// Create new scene and set up parameters.
	_rtscene = rt_newscene();
	rt_resolution(_rtscene, renderSettings()->outputImageWidth(), renderSettings()->outputImageHeight());
	if(antialiasingEnabled())
		rt_aa_maxsamples(_rtscene, antialiasingSamples());

	// Create Tachyon frame buffer.
	QImage img(renderSettings()->outputImageWidth(), renderSettings()->outputImageHeight(), QImage::Format_RGBA8888);
	rt_rawimage_rgba32(_rtscene, img.bits());

	// Set background color.
	TimeInterval iv;
	Color backgroundColor;
	renderSettings()->backgroundColorController()->getColorValue(time(), backgroundColor, iv);
	colora bgcolor = { (float)backgroundColor.r(), (float)backgroundColor.g(), (float)backgroundColor.b(), 1.0f };
	if(renderSettings()->generateAlphaChannel())
		bgcolor.a = 0;
	rt_background(_rtscene, bgcolor);

	// Set equation used for rendering specular highlights.
	rt_phong_shader(_rtscene, RT_SHADER_NULL_PHONG);

	// Set up camera.
	if(projParams().isPerspective) {
		double zoomScale = 1;
		if(!depthOfFieldEnabled() || dofFocalLength() <= 0 || dofAperture() <= 0) {
			rt_camera_projection(_rtscene, RT_PROJECTION_PERSPECTIVE);
		}
		else {
			rt_camera_projection(_rtscene, RT_PROJECTION_PERSPECTIVE_DOF);
			rt_camera_dof(_rtscene, dofFocalLength(), dofAperture());
			zoomScale = dofFocalLength();
		}

		// Calculate projection point and directions in camera space.
		Point3 p0 = projParams().inverseProjectionMatrix * Point3(0,0,0);
		Vector3 direction = projParams().inverseProjectionMatrix * Point3(0,0,0) - Point3::Origin();
		Vector3 up = projParams().inverseProjectionMatrix * Point3(0,1,0) - p0;
		// Transform to world space.
		p0 = Point3::Origin() + projParams().inverseViewMatrix.translation();
		direction = (projParams().inverseViewMatrix * direction).normalized();
		up = (projParams().inverseViewMatrix * up).normalized();
		rt_camera_position(_rtscene, tvec(p0), tvec(direction), tvec(up));
		rt_camera_zoom(_rtscene, 0.5 / tan(projParams().fieldOfView * 0.5) / zoomScale);
	}
	else {
		rt_camera_projection(_rtscene, RT_PROJECTION_ORTHOGRAPHIC);

		// Calculate projection point and directions in camera space.
		Point3 p0 = projParams().inverseProjectionMatrix * Point3(0,0,-1);
		Vector3 direction = projParams().inverseProjectionMatrix * Point3(0,0,1) - p0;
		Vector3 up = projParams().inverseProjectionMatrix * Point3(0,1,-1) - p0;
		// Transform to world space.
		p0 = projParams().inverseViewMatrix * p0;
		direction = (projParams().inverseViewMatrix * direction).normalized();
		up = (projParams().inverseViewMatrix * up).normalized();
		p0 += direction * (projParams().znear - FloatType(1e-9));

		rt_camera_position(_rtscene, tvec(p0), tvec(direction), tvec(up));
		rt_camera_zoom(_rtscene, 0.5 / projParams().fieldOfView);
	}

	// Set up light.
	if(directLightSourceEnabled()) {
		apitexture lightTex;
		memset(&lightTex, 0, sizeof(lightTex));
		lightTex.col.r = lightTex.col.g = lightTex.col.b = defaultLightSourceIntensity();
		lightTex.ambient = 1.0;
		lightTex.opacity = 1.0;
		lightTex.diffuse = 1.0;
		void* lightTexPtr = rt_texture(_rtscene, &lightTex);
		Vector3 lightDir = projParams().inverseViewMatrix * Vector3(0.2f,-0.2f,-1.0f);
		rt_directional_light(_rtscene, lightTexPtr, tvec(lightDir));
	}

	if(ambientOcclusionEnabled() || (directLightSourceEnabled() && shadowsEnabled())) {
		// Full shading mode required.
		rt_shadermode(_rtscene, RT_SHADER_FULL);
	}
	else {
		// This will turn off shadows.
		rt_shadermode(_rtscene, RT_SHADER_MEDIUM);
	}

	if(ambientOcclusionEnabled()) {
		apicolor skycol;
		skycol.r = skycol.g = skycol.b = ambientOcclusionBrightness();
		rt_rescale_lights(_rtscene, 0.2);
		rt_ambient_occlusion(_rtscene, ambientOcclusionSamples(), skycol);
	}

	rt_trans_mode(_rtscene, RT_TRANS_VMD);

	// Rays can pass through this maximum number of semi-transparent objects:
	rt_camera_raydepth(_rtscene, 1000);

	// Export Ovito data objects to Tachyon scene.
	renderScene();

	// Render visual 3D representation of the modifiers.
	renderModifiers(false);

	// Render visual 2D representation of the modifiers.
	renderModifiers(true);

	// Render scene.
	renderTask.setProgressMaximum(renderSettings()->outputImageWidth() * renderSettings()->outputImageHeight());
	renderTask.setProgressText(tr("Rendering image"));

	scenedef * scene = (scenedef *)_rtscene;

	/* if certain key aspects of the scene parameters have been changed */
	/* since the last frame rendered, or when rendering the scene the   */
	/* first time, various setup, initialization and memory allocation  */
	/* routines need to be run in order to prepare for rendering.       */
	if(scene->scenecheck)
		rendercheck(scene);

	camera_init(scene);      /* Initialize all aspects of camera system  */

	// Make sure the target frame buffer has the right memory format.
	if(frameBuffer->image().format() != QImage::Format_ARGB32)
		frameBuffer->image() = frameBuffer->image().convertToFormat(QImage::Format_ARGB32);

	int tileSize = scene->numthreads * 2;
	for(int ystart = 0; ystart < scene->vres; ystart += tileSize) {
		for(int xstart = 0; xstart < scene->hres; xstart += tileSize) {
			int xstop = std::min(scene->hres, xstart + tileSize);
			int ystop = std::min(scene->vres, ystart + tileSize);
			for(int thr = 0; thr < scene->numthreads; thr++) {
				thr_parms* parms = &((thr_parms *) scene->threadparms)[thr];
				parms->startx = 1 + xstart;
				parms->stopx  = xstop;
				parms->xinc   = 1;
				parms->starty = thr + 1 + ystart;
				parms->stopy  = ystop;
				parms->yinc   = scene->numthreads;
			}

			/* if using threads, wake up the child threads...  */
			rt_thread_barrier(((thr_parms *) scene->threadparms)[0].runbar, 1);

			/* Actually Ray Trace The Image */
			thread_trace(&((thr_parms *) scene->threadparms)[0]);

			// Copy rendered image back into Ovito's frame buffer.
			// Flip image since Tachyon fills the buffer upside down.
			OVITO_ASSERT(frameBuffer->image().format() == QImage::Format_ARGB32);
			int bperline = renderSettings()->outputImageWidth() * 4;
			for(int y = ystart; y < ystop; y++) {
				uchar* dst = frameBuffer->image().scanLine(frameBuffer->image().height() - 1 - y) + xstart * 4;
				uchar* src = img.bits() + y*bperline + xstart * 4;
				for(int x = xstart; x < xstop; x++, dst += 4, src += 4) {
					dst[0] = src[2];
					dst[1] = src[1];
					dst[2] = src[0];
					dst[3] = src[3];
				}
			}
			frameBuffer->update(QRect(xstart, frameBuffer->image().height() - ystop, xstop - xstart, ystop - ystart));

			renderTask.setProgressValue(renderTask.progressValue() + (xstop - xstart) * (ystop - ystart));
			if(renderTask.isCanceled())
				break;
		}

		if(renderTask.isCanceled())
			break;
	}

	// Execute recorded overlay draw calls.
	QPainter painter(&frameBuffer->image());
	for(const auto& imageCall : _imageDrawCalls) {
		QRectF rect(std::get<1>(imageCall).x(), std::get<1>(imageCall).y(), std::get<2>(imageCall).x(), std::get<2>(imageCall).y());
		painter.drawImage(rect, std::get<0>(imageCall));
		frameBuffer->update(rect.toAlignedRect());
	}
	for(const auto& textCall : _textDrawCalls) {
		QRectF pos(std::get<3>(textCall).x(), std::get<3>(textCall).y(), 0, 0);
		painter.setPen(std::get<1>(textCall));
		painter.setFont(std::get<2>(textCall));
		QRectF boundingRect;
		painter.drawText(pos, std::get<4>(textCall) | Qt::TextSingleLine | Qt::TextDontClip, std::get<0>(textCall), &boundingRect);
		frameBuffer->update(boundingRect.toAlignedRect());
	}

	// Clean up.
	rt_deletescene(_rtscene);

	return !renderTask.isCanceled();
}

/******************************************************************************
* Finishes the rendering pass. This is called after all animation frames have been rendered
* or when the rendering operation has been aborted.
******************************************************************************/
void TachyonRenderer::endRender()
{
	// Shut down Tachyon library.
	rt_finalize();

	// Release draw call buffers.
	_imageDrawCalls.clear();
	_textDrawCalls.clear();

	NonInteractiveSceneRenderer::endRender();
}

/******************************************************************************
* Renders the line geometry stored in the given buffer.
******************************************************************************/
void TachyonRenderer::renderLines(const DefaultLinePrimitive& lineBuffer)
{
	// Lines are not supported by this renderer.
}

/******************************************************************************
* Renders the particles stored in the given buffer.
******************************************************************************/
void TachyonRenderer::renderParticles(const DefaultParticlePrimitive& particleBuffer)
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
				void* tex = getTachyonTexture(c->r(), c->g(), c->b(), c->a());
				Point3 tp = tm * (*p);
				rt_sphere(_rtscene, tex, tvec(tp), *r);
			}
		}
	}
	else if(particleBuffer.particleShape() == ParticlePrimitive::SquareShape) {
		// Rendering cubic particles.
		for(; p != p_end; ++p, ++c, ++r) {
			if(c->a() > 0) {
				void* tex = getTachyonTexture(c->r(), c->g(), c->b(), c->a());
				Point3 tp = tm * (*p);
				rt_box(_rtscene, tex, rt_vector(tp.x() - *r, tp.y() - *r, -tp.z() - *r), rt_vector(tp.x() + *r, tp.y() + *r, -tp.z() + *r));
			}
		}
	}
	else if(particleBuffer.particleShape() == ParticlePrimitive::BoxShape) {
		// Rendering noncubic/rotated box particles.
		auto shape = particleBuffer.shapes().begin();
		auto shape_end = particleBuffer.shapes().end();
		auto orientation = particleBuffer.orientations().cbegin();
		auto orientation_end = particleBuffer.orientations().cend();
		for(; p != p_end; ++p, ++c, ++r) {
			if(c->a() <= 0) continue;
			void* tex = getTachyonTexture(c->r(), c->g(), c->b(), c->a());
			Point3 tp = tm * (*p);
			Quaternion quat(0,0,0,1);
			if(orientation != orientation_end) {
				quat = *orientation++;
				// Normalize quaternion.
				FloatType c = sqrt(quat.dot(quat));
				if(c <= FLOATTYPE_EPSILON)
					quat.setIdentity();
				else
					quat /= c;
			}
			Vector3 s(*r);
			if(shape != shape_end) {
				s = *shape++;
				if(s == Vector3::Zero())
					s = Vector3(*r);
			}
			if(quat == Quaternion(0,0,0,1)) {
				rt_box(_rtscene, tex, rt_vector(tp.x() - s.x(), tp.y() - s.y(), -tp.z()-s.z()), rt_vector(tp.x() + s.x(), tp.y() + s.y(), -tp.z()+s.z()));
			}
			else {
				apivector corners[8] = {
						tvec(tp + quat * Vector3(-s.x(), -s.y(), -s.z())),
						tvec(tp + quat * Vector3( s.x(), -s.y(), -s.z())),
						tvec(tp + quat * Vector3( s.x(),  s.y(), -s.z())),
						tvec(tp + quat * Vector3(-s.x(),  s.y(), -s.z())),
						tvec(tp + quat * Vector3(-s.x(), -s.y(),  s.z())),
						tvec(tp + quat * Vector3( s.x(), -s.y(),  s.z())),
						tvec(tp + quat * Vector3( s.x(),  s.y(),  s.z())),
						tvec(tp + quat * Vector3(-s.x(),  s.y(),  s.z()))
				};
				rt_tri(_rtscene, tex, corners[0], corners[1], corners[2]);
				rt_tri(_rtscene, tex, corners[0], corners[2], corners[3]);
				rt_tri(_rtscene, tex, corners[4], corners[6], corners[5]);
				rt_tri(_rtscene, tex, corners[4], corners[7], corners[6]);
				rt_tri(_rtscene, tex, corners[0], corners[4], corners[5]);
				rt_tri(_rtscene, tex, corners[0], corners[5], corners[1]);
				rt_tri(_rtscene, tex, corners[1], corners[5], corners[6]);
				rt_tri(_rtscene, tex, corners[1], corners[6], corners[2]);
				rt_tri(_rtscene, tex, corners[2], corners[6], corners[7]);
				rt_tri(_rtscene, tex, corners[2], corners[7], corners[3]);
				rt_tri(_rtscene, tex, corners[3], corners[7], corners[4]);
				rt_tri(_rtscene, tex, corners[3], corners[4], corners[0]);
			}
		}
	}
	else if(particleBuffer.particleShape() == ParticlePrimitive::EllipsoidShape) {
		const Matrix3 linear_tm = tm.linear();
		// Rendering ellipsoid particles.
		auto shape = particleBuffer.shapes().cbegin();
		auto shape_end = particleBuffer.shapes().cend();
		auto orientation = particleBuffer.orientations().cbegin();
		auto orientation_end = particleBuffer.orientations().cend();
		for(; p != p_end && shape != shape_end; ++p, ++c, ++shape, ++r) {
			if(c->a() <= 0) continue;
			void* tex = getTachyonTexture(c->r(), c->g(), c->b(), c->a());
			Point3 tp = tm * (*p);
			Quaternion quat(0,0,0,1);
			if(orientation != orientation_end) {
				quat = *orientation++;
				// Normalize quaternion.
				FloatType c = sqrt(quat.dot(quat));
				if(c == 0)
					quat.setIdentity();
				else
					quat /= c;
			}
			if(shape->x() != 0 && shape->y() != 0 && shape->z() != 0) {
				Matrix3 qmat(FloatType(1)/(shape->x()*shape->x()), 0, 0,
						     0, FloatType(1)/(shape->y()*shape->y()), 0,
						     0, 0, FloatType(1)/(shape->z()*shape->z()));
				Matrix3 rot = Matrix3(1,0,0, 0,1,0, 0,0,-1) * linear_tm * Matrix3::rotation(quat);
				Matrix3 quadric = rot * qmat * rot.transposed();
				rt_quadric(_rtscene, tex, tvec(tp),
						quadric(0,0), quadric(0,1), quadric(0,2), 0.0,
						quadric(1,1), quadric(1,2), 0.0,
						quadric(2,2), 0.0, -1.0, std::max(shape->x(), std::max(shape->y(), shape->z())));
			}
			else {
				rt_sphere(_rtscene, tex, tvec(tp), *r);
			}
		}
	}
}

/******************************************************************************
* Renders the arrow elements stored in the given buffer.
******************************************************************************/
void TachyonRenderer::renderArrows(const DefaultArrowPrimitive& arrowBuffer)
{
	const AffineTransformation tm = modelTM();
	if(arrowBuffer.shape() == ArrowPrimitive::CylinderShape) {
		for(const DefaultArrowPrimitive::ArrowElement& element : arrowBuffer.elements()) {
			void* tex = getTachyonTexture(element.color.r(), element.color.g(), element.color.b(), element.color.a());
			Point3 tp = tm * element.pos;
			Vector3 ta = tm * element.dir;
			rt_fcylinder(_rtscene, tex, tvec(tp), tvec(ta), element.width);
			rt_ring(_rtscene, tex, tvec(tp+ta), tvec(ta), 0, element.width);
			rt_ring(_rtscene, tex, tvec(tp), tvec(-ta), 0, element.width);
		}
	}

	else if(arrowBuffer.shape() == ArrowPrimitive::ArrowShape) {
		for(const DefaultArrowPrimitive::ArrowElement& element : arrowBuffer.elements()) {
			void* tex = getTachyonTexture(element.color.r(), element.color.g(), element.color.b(), element.color.a());
			FloatType arrowHeadRadius = element.width * FloatType(2.5);
			FloatType arrowHeadLength = arrowHeadRadius * FloatType(1.8);
			FloatType length = element.dir.length();
			if(length == 0)
				continue;

			if(length > arrowHeadLength) {
				Point3 tp = tm * element.pos;
				Vector3 ta = tm * (element.dir * ((length - arrowHeadLength) / length));
				Vector3 tb = tm * (element.dir * (arrowHeadLength / length));

				rt_fcylinder(_rtscene, tex, tvec(tp), tvec(ta), element.width);
				rt_ring(_rtscene, tex, tvec(tp), tvec(-ta), 0, element.width);
				rt_ring(_rtscene, tex, tvec(tp+ta), tvec(-ta), element.width, arrowHeadRadius);
				rt_cone(_rtscene, tex, tvec(tp+ta+tb), tvec(-tb), arrowHeadRadius);
			}
			else {
				FloatType r = arrowHeadRadius * length / arrowHeadLength;

				Point3 tp = tm * element.pos;
				Vector3 ta = tm * element.dir;

				rt_ring(_rtscene, tex, tvec(tp), tvec(-ta), 0, r);
				rt_cone(_rtscene, tex, tvec(tp+ta), tvec(-ta), r);
			}
		}
	}
}

/******************************************************************************
* Renders the text stored in the given buffer.
******************************************************************************/
void TachyonRenderer::renderText(const DefaultTextPrimitive& textBuffer, const Point2& pos, int alignment)
{
	_textDrawCalls.push_back(std::make_tuple(textBuffer.text(), textBuffer.color(), textBuffer.font(), pos, alignment));
}

/******************************************************************************
* Renders the image stored in the given buffer.
******************************************************************************/
void TachyonRenderer::renderImage(const DefaultImagePrimitive& imageBuffer, const Point2& pos, const Vector2& size)
{
	_imageDrawCalls.push_back(std::make_tuple(imageBuffer.image(), pos, size));
}

/******************************************************************************
* Renders the triangle mesh stored in the given buffer.
******************************************************************************/
void TachyonRenderer::renderMesh(const DefaultMeshPrimitive& meshBuffer)
{
	// Stores data of a single vertex passed to Tachyon.
	struct ColoredVertexWithNormal {
		ColorAT<float> color;
		Vector_3<float> normal;
		Point_3<float> pos;
	};

	const TriMesh& mesh = meshBuffer.mesh();

	// Allocate render vertex buffer.
	int renderVertexCount = mesh.faceCount() * 3;
	if(renderVertexCount == 0)
		return;
	std::vector<ColoredVertexWithNormal> renderVertices(renderVertexCount);

	const AffineTransformationT<float> tm = (AffineTransformationT<float>)modelTM();
	const Matrix_3<float> normalTM = tm.linear().inverse().transposed();
	quint32 allMask = 0;

	// Compute face normals.
	std::vector<Vector_3<float>> faceNormals(mesh.faceCount());
	auto faceNormal = faceNormals.begin();
	for(auto face = mesh.faces().constBegin(); face != mesh.faces().constEnd(); ++face, ++faceNormal) {
		const Point3& p0 = mesh.vertex(face->vertex(0));
		Vector3 d1 = mesh.vertex(face->vertex(1)) - p0;
		Vector3 d2 = mesh.vertex(face->vertex(2)) - p0;
		*faceNormal = normalTM * (Vector_3<float>)d2.cross(d1);
		if(*faceNormal != Vector_3<float>::Zero()) {
			//faceNormal->normalize();
			allMask |= face->smoothingGroups();
		}
	}

	// Initialize render vertices.
	std::vector<ColoredVertexWithNormal>::iterator rv = renderVertices.begin();
	faceNormal = faceNormals.begin();
	ColorAT<float> defaultVertexColor = ColorAT<float>(meshBuffer.meshColor());
	for(auto face = mesh.faces().constBegin(); face != mesh.faces().constEnd(); ++face, ++faceNormal) {

		// Initialize render vertices for this face.
		for(size_t v = 0; v < 3; v++, rv++) {
			if(face->smoothingGroups())
				rv->normal = Vector_3<float>::Zero();
			else
				rv->normal = *faceNormal;
			rv->pos = tm * (Point_3<float>)mesh.vertex(face->vertex(v));

			if(mesh.hasVertexColors())
				rv->color = ColorAT<float>(mesh.vertexColor(face->vertex(v)));
			else if(mesh.hasFaceColors())
				rv->color = ColorAT<float>(mesh.faceColor(face - mesh.faces().constBegin()));
			else if(face->materialIndex() < meshBuffer.materialColors().size() && face->materialIndex() >= 0)
				rv->color = ColorAT<float>(meshBuffer.materialColors()[face->materialIndex()]);
			else
				rv->color = defaultVertexColor;
		}
	}

	if(allMask) {
		std::vector<Vector_3<float>> groupVertexNormals(mesh.vertexCount());
		for(int group = 0; group < OVITO_MAX_NUM_SMOOTHING_GROUPS; group++) {
			quint32 groupMask = quint32(1) << group;
            if((allMask & groupMask) == 0) continue;

			// Reset work arrays.
            std::fill(groupVertexNormals.begin(), groupVertexNormals.end(), Vector_3<float>::Zero());

			// Compute vertex normals at original vertices for current smoothing group.
            faceNormal = faceNormals.begin();
			for(auto face = mesh.faces().constBegin(); face != mesh.faces().constEnd(); ++face, ++faceNormal) {
				// Skip faces which do not belong to the current smoothing group.
				if((face->smoothingGroups() & groupMask) == 0) continue;

				// Add face's normal to vertex normals.
				for(size_t fv = 0; fv < 3; fv++)
					groupVertexNormals[face->vertex(fv)] += *faceNormal;
			}

			// Transfer vertex normals from original vertices to render vertices.
			rv = renderVertices.begin();
			for(const auto& face : mesh.faces()) {
				if(face.smoothingGroups() & groupMask) {
					for(size_t fv = 0; fv < 3; fv++, ++rv)
						rv->normal += groupVertexNormals[face.vertex(fv)];
				}
				else rv += 3;
			}
		}
	}

	// Precompute some camera-related information that is needed for face culling.
	Point_3<float> cameraPos = Point_3<float>::Origin() + (Vector_3<float>)projParams().inverseViewMatrix.translation();
	Vector3 projectionSpaceDirection = projParams().inverseProjectionMatrix * Point3(0,0,1) - projParams().inverseProjectionMatrix * Point3(0,0,-1);
	Vector_3<float> cameraDirection = (Vector_3<float>)(projParams().inverseViewMatrix * projectionSpaceDirection);

	// Pass transformed triangles to Tachyon renderer.
	faceNormal = faceNormals.begin();
	void* tex = getTachyonTexture(1.0f, 1.0f, 1.0f, defaultVertexColor.a());
	for(auto rv = renderVertices.begin(); rv != renderVertices.end(); ++faceNormal) {
		auto rv0 = rv++;
		auto rv1 = rv++;
		auto rv2 = rv++;

		// Perform culling of triangles not facing the viewer.
		if(meshBuffer.cullFaces()) {
			if(projParams().isPerspective) {
				if(faceNormal->dot(rv0->pos - cameraPos) >= 0) continue;
			}
			else {
				if(faceNormal->dot(cameraDirection) >= 0) continue;
			}
		}

		if(mesh.hasVertexColors() || mesh.hasFaceColors() || meshBuffer.materialColors().empty() == false)
			tex = getTachyonTexture(1.0f, 1.0f, 1.0f, defaultVertexColor.a());

		rt_vcstri(_rtscene, tex,
				tvec(rv0->pos),
				tvec(rv1->pos),
				tvec(rv2->pos),
				tvec(rv0->normal),
				tvec(rv1->normal),
				tvec(rv2->normal),
				rt_color(rv0->color.r(), rv0->color.g(), rv0->color.b()),
				rt_color(rv1->color.r(), rv1->color.g(), rv1->color.b()),
				rt_color(rv2->color.r(), rv2->color.g(), rv2->color.b()));
	}
}

/******************************************************************************
* Creates a texture with the given color.
******************************************************************************/
void* TachyonRenderer::getTachyonTexture(FloatType r, FloatType g, FloatType b, FloatType alpha)
{
	apitexture tex;
	memset(&tex, 0, sizeof(tex));
	tex.ambient  = flt(0.3);
	tex.diffuse  = flt(0.8);
	tex.specular = flt(0.0);
	tex.opacity  = alpha;
	tex.col.r = r;
	tex.col.g = g;
	tex.col.b = b;
	tex.texturefunc = RT_TEXTURE_CONSTANT;

	return rt_texture(_rtscene, &tex);
}

}	// End of namespace
}	// End of namespace

