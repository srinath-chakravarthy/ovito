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

#include <QTemporaryFile>
#include <QProcess>

namespace Ovito { namespace POVRay {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(POVRay, POVRayRenderer, NonInteractiveSceneRenderer);
DEFINE_FLAGS_PROPERTY_FIELD(POVRayRenderer, _qualityLevel, "QualityLevel", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(POVRayRenderer, _enableAntialiasing, "EnableAntialiasing", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(POVRayRenderer, _samplingMethod, "SamplingMethod", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(POVRayRenderer, _AAThreshold, "AAThreshold", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(POVRayRenderer, _antialiasDepth, "AntialiasDepth", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(POVRayRenderer, _enableJitter, "EnableJitter", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(POVRayRenderer, _povrayDisplayEnabled, "ShowPOVRayDisplay", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(POVRayRenderer, _enableRadiosity, "EnableRadiosity", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(POVRayRenderer, _radiosityRayCount, "RadiosityRayCount", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(POVRayRenderer, _radiosityRecursionLimit, "RadiosityRecursionLimit", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(POVRayRenderer, _radiosityErrorBound, "RadiosityErrorBound", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(POVRayRenderer, _povrayExecutable, "ExecutablePath", PROPERTY_FIELD_MEMORIZE);
SET_PROPERTY_FIELD_LABEL(POVRayRenderer, _qualityLevel, "Quality level");
SET_PROPERTY_FIELD_LABEL(POVRayRenderer, _enableAntialiasing, "Enable anti-aliasing");
SET_PROPERTY_FIELD_LABEL(POVRayRenderer, _samplingMethod, "Sampling method");
SET_PROPERTY_FIELD_LABEL(POVRayRenderer, _AAThreshold, "Anti-aliasing threshold");
SET_PROPERTY_FIELD_LABEL(POVRayRenderer, _antialiasDepth, "Anti-aliasing depth");
SET_PROPERTY_FIELD_LABEL(POVRayRenderer, _enableJitter, "Enable jitter");
SET_PROPERTY_FIELD_LABEL(POVRayRenderer, _povrayDisplayEnabled, "Show POV-Ray window");
SET_PROPERTY_FIELD_LABEL(POVRayRenderer, _enableRadiosity, "Enable radiosity");
SET_PROPERTY_FIELD_LABEL(POVRayRenderer, _radiosityRayCount, "Ray count");
SET_PROPERTY_FIELD_LABEL(POVRayRenderer, _radiosityRecursionLimit, "Recursion limit");
SET_PROPERTY_FIELD_LABEL(POVRayRenderer, _radiosityErrorBound, "Error bound");
SET_PROPERTY_FIELD_LABEL(POVRayRenderer, _povrayExecutable, "POV-Ray executable path");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(POVRayRenderer, _qualityLevel, IntegerParameterUnit, 0, 11);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(POVRayRenderer, _samplingMethod, IntegerParameterUnit, 1, 2);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(POVRayRenderer, _AAThreshold, FloatParameterUnit, 0, 1);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(POVRayRenderer, _antialiasDepth, IntegerParameterUnit, 1, 9);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(POVRayRenderer, _radiosityRayCount, IntegerParameterUnit, 1, 1600);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(POVRayRenderer, _radiosityRecursionLimit, IntegerParameterUnit, 1, 20);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(POVRayRenderer, _radiosityErrorBound, FloatParameterUnit, 1e-5, 100);

/******************************************************************************
* Constructor.
******************************************************************************/
POVRayRenderer::POVRayRenderer(DataSet* dataset) : NonInteractiveSceneRenderer(dataset),
	_qualityLevel(9), _enableAntialiasing(true), _samplingMethod(1), _AAThreshold(0.3f),
	_antialiasDepth(3), _enableJitter(true), _povrayDisplayEnabled(true), _enableRadiosity(false),
	_radiosityRayCount(35), _radiosityRecursionLimit(3), _radiosityErrorBound(1.8f)
{
	INIT_PROPERTY_FIELD(POVRayRenderer::_qualityLevel);
	INIT_PROPERTY_FIELD(POVRayRenderer::_enableAntialiasing);
	INIT_PROPERTY_FIELD(POVRayRenderer::_samplingMethod);
	INIT_PROPERTY_FIELD(POVRayRenderer::_AAThreshold);
	INIT_PROPERTY_FIELD(POVRayRenderer::_antialiasDepth);
	INIT_PROPERTY_FIELD(POVRayRenderer::_enableJitter);
	INIT_PROPERTY_FIELD(POVRayRenderer::_povrayDisplayEnabled);
	INIT_PROPERTY_FIELD(POVRayRenderer::_enableRadiosity);
	INIT_PROPERTY_FIELD(POVRayRenderer::_radiosityRayCount);
	INIT_PROPERTY_FIELD(POVRayRenderer::_radiosityRecursionLimit);
	INIT_PROPERTY_FIELD(POVRayRenderer::_radiosityErrorBound);
	INIT_PROPERTY_FIELD(POVRayRenderer::_povrayExecutable);
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

	if(_outputStream.device() == nullptr) {
		// Write scene to a temporary file.
		_sceneFile.reset(new QTemporaryFile(QDir::tempPath() + QStringLiteral("/scene.XXXXXX.pov")));
		if(!_sceneFile->open())
			throwException(tr("Failed to open temporary POV-Ray scene file for writing."));
		_sceneFile->setTextModeEnabled(true);
		_outputStream.setDevice(_sceneFile.get());

		// Let POV-Ray write the image to a temporary file.
		_imageFile.reset(new QTemporaryFile(QDir::tempPath() + QStringLiteral("/povray.XXXXXX.png")));
		if(!_imageFile->open())
			throwException(tr("Failed to open temporary POV-Ray image file."));
	}
	
	_outputStream << "#version 3.5;\n";
	_outputStream << "#include \"transforms.inc\"\n";

	if(_enableRadiosity) {
		_outputStream << "global_settings {\n";
		_outputStream << "radiosity {\n";
		_outputStream << "count " << _radiosityRayCount.value() << "\n";
		_outputStream << "always_sample on\n";		
		_outputStream << "recursion_limit " << _radiosityRecursionLimit.value() << "\n";
		_outputStream << "error_bound " << _radiosityErrorBound.value() << "\n";
		_outputStream << "}\n";
		_outputStream << "}\n";
	}

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
		Vector3 direction = (lookat - Point3::Origin()).normalized();
		Vector3 right = px - p0;
		Vector3 up = right.cross(direction).normalized();
		right = direction.cross(up).normalized() * (up.length() / projParams().aspectRatio);

		_outputStream << "  location <0, 0, 0>\n";
		_outputStream << "  direction "; write(direction); _outputStream << "\n";
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
	Vector3 viewingDirection = projParams().inverseViewMatrix.column(2);
	Vector3 screen_x = projParams().inverseViewMatrix.column(0).normalized();
	Vector3 screen_y = projParams().inverseViewMatrix.column(1).normalized();

	// Write a light source.
	_outputStream << "light_source {\n";
	_outputStream << "  <0, 0, 0>\n";
	if(!_enableRadiosity)
		_outputStream << "  color <1.5, 1.5, 1.5>\n";
	else
		_outputStream << "  color <0.5, 0.5, 0.5>\n";
	_outputStream << "  parallel\n";
	_outputStream << "  shadowless\n";
	_outputStream << "  point_at "; write(projParams().inverseViewMatrix * Vector3(0,0,-1)); _outputStream << "\n";
	_outputStream << "}\n";

	// Define macro for particle primitives in the POV-Ray file to reduce file size.
	_outputStream << "#macro SPRTCLE(pos, particleRadius, particleColor) // Macro for spherical particles\n";
	_outputStream << "sphere { pos, particleRadius\n";
	_outputStream << "         texture { pigment { color particleColor } }\n";
	_outputStream << "}\n";
	_outputStream << "#end\n";
	_outputStream << "#macro DPRTCLE(pos, particleRadius, particleColor) // Macro for flat disc particles facing the camera\n";
	_outputStream << "disc { pos, "; write(viewingDirection); _outputStream << ", particleRadius\n";
	_outputStream << "         texture { pigment { color particleColor } }\n";
	_outputStream << "}\n";
	_outputStream << "#end\n";
	_outputStream << "#macro CPRTCLE(pos, particleRadius, particleColor) // Macro for cubic particles\n";
	_outputStream << "box { pos - <particleRadius,particleRadius,particleRadius>, pos + <particleRadius,particleRadius,particleRadius>\n";
	_outputStream << "         texture { pigment { color particleColor } }\n";
	_outputStream << "}\n";
	_outputStream << "#end\n";
	_outputStream << "#macro SQPRTCLE(pos, particleRadius, particleColor) // Macro for flat square particles facing the camera\n";
	_outputStream << "triangle { pos+"; write(screen_x+screen_y); _outputStream << "*particleRadius, ";
	_outputStream << "pos+"; write(screen_x-screen_y); _outputStream << "*particleRadius, ";
	_outputStream << "pos+"; write(-screen_x-screen_y); _outputStream << "*particleRadius\n";
	_outputStream << "         texture { pigment { color particleColor } }\n";
	_outputStream << "}\n";
	_outputStream << "triangle { pos+"; write(screen_x+screen_y); _outputStream << "*particleRadius, ";
	_outputStream << "pos+"; write(-screen_x-screen_y); _outputStream << "*particleRadius, ";
	_outputStream << "pos+"; write(-screen_x+screen_y); _outputStream << "*particleRadius\n";
	_outputStream << "         texture { pigment { color particleColor } }\n";
	_outputStream << "}\n";
	_outputStream << "#end\n";
}

/******************************************************************************
* Renders a single animation frame into the given frame buffer.
******************************************************************************/
bool POVRayRenderer::renderFrame(FrameBuffer* frameBuffer, StereoRenderingTask stereoTask, AbstractProgressDisplay* progress)
{
	if(progress) progress->setStatusText(tr("Writing scene to temporary POV-Ray file."));

	// Export Ovito data objects to POV-Ray scene.
	renderScene();

	// Render visual 3D representation of the modifiers.
	renderModifiers(false);

	// Render visual 2D representation of the modifiers.
	renderModifiers(true);
	
	if(_sceneFile && _imageFile) {
		_outputStream.flush();
		_sceneFile->close();
		_imageFile->close();

		// Start POV-Ray sub-process.
		if(progress) {
			progress->setStatusText(tr("Starting external POV-Ray program."));
			if(progress->wasCanceled()) return false;
		}

		// Specify POV-Ray options:
		QStringList parameters;
		parameters << QString("+W%1").arg(renderSettings()->outputImageWidth());	// Set image width
		parameters << QString("+H%1").arg(renderSettings()->outputImageHeight()); 	// Set image height
		parameters << "Pause_When_Done=off";	// No pause after rendering
		parameters << "Output_to_File=on";		// Output to file
		parameters << "-V";		// Verbose output
		parameters << "Output_File_Type=N";		// Output PNG image
		parameters << QString("Output_File_Name=%1").arg(QDir::toNativeSeparators(_imageFile->fileName()));		// Output image to temporary file.
		parameters << QString("Input_File_Name=%1").arg(QDir::toNativeSeparators(_sceneFile->fileName()));		// Read scene from temporary file.

		if(renderSettings()->generateAlphaChannel())
			parameters << "Output_Alpha=on";		// Output alpha channel
		else
			parameters << "Output_Alpha=off";		// No alpha channel

		if(_povrayDisplayEnabled)
			parameters << "Display=on";
		else
			parameters << "Display=off";

		// Pass quality settings to POV-Ray.
		if(_qualityLevel)
			parameters << QString("+Q%1").arg(_qualityLevel);
		if(_enableAntialiasing) {
			if(_enableAntialiasing) {
				if(_AAThreshold)
					parameters << QString("+A%1").arg(_AAThreshold);
				else
					parameters << "+A";
			}
			else {
				parameters << "-A";
			}
		}
		if(_samplingMethod)
			parameters << QString("+AM%1").arg(_samplingMethod);
		if(_antialiasDepth)
			parameters << QString("+R%1").arg(_antialiasDepth);
		if(_enableJitter)
			parameters << (_enableJitter ? "+J" : "-J");		

		QProcess povrayProcess;
		QString executablePath = povrayExecutable().isEmpty() ? QString("povray") : povrayExecutable();
		qDebug() << "Starting POV-Ray sub-process (path =" << executablePath << ")";
		qDebug() << "POV-Ray parameters:" << parameters;
		povrayProcess.setReadChannel(QProcess::StandardOutput);
		povrayProcess.start(executablePath, parameters);
		if(!povrayProcess.waitForStarted()) {
			QString errorString = povrayProcess.errorString();
			if(povrayProcess.error() == QProcess::FailedToStart)
				errorString = tr("The process failed to start. Either the invoked program is missing, or you may have insufficient permissions to invoke the program.");
			throwException(tr("Could not run the POV-Ray executable: %1 (error code %2)\nPlease check your POV-Ray installation.\nExecutable path: %3").arg(errorString).arg(povrayProcess.error()).arg(executablePath));
		}

		// Wait until POV-Ray has finished rendering.
		if(progress) {
			progress->setStatusText(tr("Waiting for external POV-Ray program..."));
			if(progress->wasCanceled()) return false;
		}

		while(!povrayProcess.waitForFinished(100)) {
			if(progress) {
				QCoreApplication::processEvents();
				if(progress->wasCanceled())
					return false;
			}
		}

		OVITO_ASSERT(povrayProcess.exitStatus() == QProcess::NormalExit);
		qDebug() << "POV-Ray console output:";
		std::cout << povrayProcess.readAllStandardError().constData(); 
		qDebug() << "POV-Ray program returned with exit code" << povrayProcess.exitCode();
		if(povrayProcess.exitCode() != 0)
			throwException(tr("POV-Ray program returned with error code %1.").arg(povrayProcess.exitCode()));

		// Get rendered image from POV-Ray process.
		if(progress) {
			progress->setStatusText(tr("Getting rendered image from POV-Ray."));
			if(progress->wasCanceled()) return false;
		}

		QImage povrayImage;
		if(!povrayImage.load(_imageFile->fileName(), "PNG")) {
			_imageFile->setAutoRemove(false);
			qDebug() << "Temporary image filename was" << QDir::toNativeSeparators(_imageFile->fileName());
			qDebug() << "Temporary file will not be deleted.";
			throwException(tr("Failed to parse image data obtained from external POV-Ray program."));
		}

		// Fill frame buffer with background color.
		QPainter painter(&frameBuffer->image());

		if(!renderSettings()->generateAlphaChannel()) {
			TimeInterval iv;
			Color backgroundColor;
			renderSettings()->backgroundColorController()->getColorValue(time(), backgroundColor, iv);			
			painter.fillRect(frameBuffer->image().rect(), backgroundColor);
		}

		// Copy POV-Ray image to the internal frame buffer.
		painter.drawImage(0, 0, povrayImage);
		frameBuffer->update();

		// Execute recorded overlay draw calls.
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
	}

	return (!progress || progress->wasCanceled() == false);
}

/******************************************************************************
* This method is called after renderFrame() has been called.
******************************************************************************/
void POVRayRenderer::endFrame()
{
	_sceneFile.reset();
	_imageFile.reset();
	_outputStream.setDevice(nullptr);
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
		if(particleBuffer.shadingMode() == ParticlePrimitive::NormalShading) {
			// Rendering spherical particles.
			for(; p != p_end; ++p, ++c, ++r) {
				if(c->a() > 0) {
					_outputStream << "SPRTCLE("; write(tm * (*p)); 
					_outputStream << ", " << (*r) << ", "; write(*c);
					_outputStream << ")\n";
				}
			}
		}
		else {
			// Rendering disc particles.
			for(; p != p_end; ++p, ++c, ++r) {
				if(c->a() > 0) {
					_outputStream << "DPRTCLE("; write(tm * (*p)); 
					_outputStream << ", " << (*r) << ", "; write(*c);
					_outputStream << ")\n";
				}
			}
		}
	}
	else if(particleBuffer.particleShape() == ParticlePrimitive::SquareShape) {
		if(particleBuffer.shadingMode() == ParticlePrimitive::NormalShading) {
			// Rendering cubic particles.
			for(; p != p_end; ++p, ++c, ++r) {
				if(c->a() > 0) {
					_outputStream << "CPRTCLE("; write(tm * (*p)); 
					_outputStream << ", " << (*r) << ", "; write(*c);
					_outputStream << ")\n";
				}
			}
		}
		else {
			// Rendering square particles.
			for(; p != p_end; ++p, ++c, ++r) {
				if(c->a() > 0) {
					_outputStream << "SQPRTCLE("; write(tm * (*p)); 
					_outputStream << ", " << (*r) << ", "; write(*c);
					_outputStream << ")\n";
				}
			}
		}
	}
	else if(particleBuffer.particleShape() == ParticlePrimitive::BoxShape || particleBuffer.particleShape() == ParticlePrimitive::EllipsoidShape) {
		// Rendering box/ellipsoid particles.
		auto shape = particleBuffer.shapes().begin();
		auto shape_end = particleBuffer.shapes().end();
		auto orientation = particleBuffer.orientations().cbegin();
		auto orientation_end = particleBuffer.orientations().cend();		
		for(; p != p_end; ++p, ++c, ++r) {
			if(c->a() <= 0) continue;
			AffineTransformation pmatrix = AffineTransformation::Identity();
			pmatrix.translation() = (tm * (*p) - Point3::Origin());
			if(orientation != orientation_end) {
				Quaternion quat = *orientation++;
				// Normalize quaternion.
				FloatType c = sqrt(quat.dot(quat));
				if(c >= FLOATTYPE_EPSILON) {
					quat /= c;
					Matrix3 rot = Matrix3::rotation(quat);
					pmatrix.column(0) = rot.column(0);
					pmatrix.column(1) = rot.column(1);
					pmatrix.column(2) = rot.column(2);
				}
			}
			Vector3 s(*r);
			if(shape != shape_end) {
				s = *shape++;
				if(s == Vector3::Zero())
					s = Vector3(*r);
			}
			if(particleBuffer.particleShape() == ParticlePrimitive::BoxShape) {
				_outputStream << "box { "; write(-s); _outputStream << ", "; write(s); _outputStream << "\n";
				_outputStream << "      texture { pigment { color "; write(*c); _outputStream << " } }\n";
				_outputStream << "      matrix "; write(pmatrix); _outputStream << "\n";
				_outputStream << "}\n";
			}
			else {
				_outputStream << "sphere { <0,0,0>, 1\n";
				_outputStream << "      texture { pigment { color "; write(*c); _outputStream << " } }\n";
				_outputStream << "      matrix "; write(pmatrix * AffineTransformation(
						 s.x(),0,0,0,
						 0,s.y(),0,0,
						 0,0,s.z(),0)); _outputStream << "\n";
				_outputStream << "}\n";
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
	const AffineTransformation tm = modelTM();
	if(arrowBuffer.shape() == ArrowPrimitive::CylinderShape) {
		for(const DefaultArrowPrimitive::ArrowElement& element : arrowBuffer.elements()) {
			_outputStream << "cylinder { ";
			write(tm * element.pos);	// base point
			_outputStream << ", ";
			write(tm * (element.pos + element.dir));	// cap point
			_outputStream << ", " << element.width << "\n";
			_outputStream << "         texture { pigment { color ";
			write(element.color);
			_outputStream << " } }\n";
			_outputStream << "}\n";
		}
	}
	else if(arrowBuffer.shape() == ArrowPrimitive::ArrowShape) {
		for(const DefaultArrowPrimitive::ArrowElement& element : arrowBuffer.elements()) {
			FloatType arrowHeadRadius = element.width * FloatType(2.5);
			FloatType arrowHeadLength = arrowHeadRadius * FloatType(1.8);
			FloatType length = element.dir.length();
			if(length == 0)
				continue;

			if(length > arrowHeadLength) {
				Point3 tp = tm * element.pos;
				Vector3 ta = tm * (element.dir * ((length - arrowHeadLength) / length));
				Vector3 tb = tm * (element.dir * (arrowHeadLength / length));

				_outputStream << "cylinder { ";
				write(tp);	// base point
				_outputStream << ", ";
				write(tp+ta);	// cap point
				_outputStream << ", " << element.width << "\n";
				_outputStream << "         texture { pigment { color ";
				write(element.color);
				_outputStream << " } }\n";
				_outputStream << "}\n";

				_outputStream << "cone { ";
				write(tp+ta);	// base point
				_outputStream << ", " << arrowHeadRadius << ", ";
				write(tp+ta+tb);	// cap point
				_outputStream << ", 0\n";
				_outputStream << "         texture { pigment { color ";
				write(element.color);
				_outputStream << " } }\n";
				_outputStream << "}\n";				
			}
			else {
				FloatType r = arrowHeadRadius * length / arrowHeadLength;

				Point3 tp = tm * element.pos;
				Vector3 ta = tm * element.dir;

				_outputStream << "cone { ";
				write(tp);	// base point
				_outputStream << ", " << r << ", ";
				write(tp+ta);	// cap point
				_outputStream << ", 0\n";
				_outputStream << "         texture { pigment { color ";
				write(element.color);
				_outputStream << " } }\n";
				_outputStream << "}\n";
			}
		}
	}
	else throwException(tr("Arrow shape not supported by POV-Ray renderer: %1").arg(arrowBuffer.shape()));
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
	// Stores data of a single vertex passed to POV-Ray.
	struct VertexWithNormal {
		Vector3 normal;
		Point3 pos;
	};

	const TriMesh& mesh = meshBuffer.mesh();

	// Allocate render vertex buffer.
	int renderVertexCount = mesh.faceCount() * 3;
	if(renderVertexCount == 0)
		return;
	std::vector<VertexWithNormal> renderVertices(renderVertexCount);
	quint32 allMask = 0;

	// Compute face normals.
	std::vector<Vector3> faceNormals(mesh.faceCount());
	auto faceNormal = faceNormals.begin();
	for(auto face = mesh.faces().constBegin(); face != mesh.faces().constEnd(); ++face, ++faceNormal) {
		const Point3& p0 = mesh.vertex(face->vertex(0));
		Vector3 d1 = mesh.vertex(face->vertex(1)) - p0;
		Vector3 d2 = mesh.vertex(face->vertex(2)) - p0;
		*faceNormal = d2.cross(d1);
		if(*faceNormal != Vector3::Zero()) {
			faceNormal->normalize();
			allMask |= face->smoothingGroups();
		}
	}

	// Initialize render vertices.
	std::vector<VertexWithNormal>::iterator rv = renderVertices.begin();
	faceNormal = faceNormals.begin();
	for(auto face = mesh.faces().constBegin(); face != mesh.faces().constEnd(); ++face, ++faceNormal) {

		// Initialize render vertices for this face.
		for(size_t v = 0; v < 3; v++, rv++) {
			if(face->smoothingGroups())
				rv->normal = Vector3::Zero();
			else
				rv->normal = *faceNormal;
			rv->pos = mesh.vertex(face->vertex(v));
		}
	}

	if(allMask) {
		std::vector<Vector3> groupVertexNormals(mesh.vertexCount());
		for(int group = 0; group < OVITO_MAX_NUM_SMOOTHING_GROUPS; group++) {
			quint32 groupMask = quint32(1) << group;
            if((allMask & groupMask) == 0) continue;

			// Reset work arrays.
            std::fill(groupVertexNormals.begin(), groupVertexNormals.end(), Vector3::Zero());

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

	_outputStream << "mesh {\n";

	for(auto rv = renderVertices.begin(); rv != renderVertices.end(); ) {
		_outputStream << "smooth_triangle {\n";
		write(rv->pos); _outputStream << ", "; write(rv->normal); _outputStream << ",\n";
		++rv;
		write(rv->pos); _outputStream << ", "; write(rv->normal); _outputStream << ",\n";
		++rv;
		write(rv->pos); _outputStream << ", "; write(rv->normal); _outputStream << " }\n";
		++rv;
	}

	// Write material
	_outputStream << "material {\n";
	_outputStream << "  texture { pigment { color "; write(meshBuffer.meshColor()); _outputStream << " } }\n";
	_outputStream << "}\n";

	// Write transformation.
	_outputStream << "matrix "; write(modelTM()); _outputStream << "\n";

	_outputStream << "}\n";
}

}	// End of namespace
}	// End of namespace

