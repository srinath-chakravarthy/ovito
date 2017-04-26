///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2014) Alexander Stukowski
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
#include <plugins/particles/objects/ParticleDisplay.h>
#include <core/app/Application.h>
#include <opengl_renderer/OpenGLSceneRenderer.h>
#include "AmbientOcclusionModifier.h"
#include "AmbientOcclusionRenderer.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Coloring)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(AmbientOcclusionModifier, AsynchronousParticleModifier);
DEFINE_PROPERTY_FIELD(AmbientOcclusionModifier, intensity, "Intensity");
DEFINE_PROPERTY_FIELD(AmbientOcclusionModifier, samplingCount, "SamplingCount");
DEFINE_PROPERTY_FIELD(AmbientOcclusionModifier, bufferResolution, "BufferResolution");
SET_PROPERTY_FIELD_LABEL(AmbientOcclusionModifier, intensity, "Shading intensity");
SET_PROPERTY_FIELD_LABEL(AmbientOcclusionModifier, samplingCount, "Number of exposure samples");
SET_PROPERTY_FIELD_LABEL(AmbientOcclusionModifier, bufferResolution, "Render buffer resolution");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(AmbientOcclusionModifier, intensity, PercentParameterUnit, 0, 1);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(AmbientOcclusionModifier, samplingCount, IntegerParameterUnit, 3, 2000);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(AmbientOcclusionModifier, bufferResolution, IntegerParameterUnit, 1, AmbientOcclusionModifier::MAX_AO_RENDER_BUFFER_RESOLUTION);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
AmbientOcclusionModifier::AmbientOcclusionModifier(DataSet* dataset) : AsynchronousParticleModifier(dataset),
	_intensity(0.7f), _samplingCount(40), _bufferResolution(3)
{
	INIT_PROPERTY_FIELD(intensity);
	INIT_PROPERTY_FIELD(samplingCount);
	INIT_PROPERTY_FIELD(bufferResolution);
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
std::shared_ptr<AsynchronousParticleModifier::ComputeEngine> AmbientOcclusionModifier::createEngine(TimePoint time, TimeInterval validityInterval)
{
	if(Application::instance()->headlessMode())
		throwException(tr("Ambient occlusion modifier requires OpenGL support and cannot be used when program is running in headless mode. "
						  "Please run program on a machine where access to graphics hardware is possible."));

	// Get modifier input.
	ParticlePropertyObject* posProperty = expectStandardProperty(ParticleProperty::PositionProperty);
	ParticleTypeProperty* typeProperty = dynamic_object_cast<ParticleTypeProperty>(inputStandardProperty(ParticleProperty::PositionProperty));
	ParticlePropertyObject* radiusProperty = inputStandardProperty(ParticleProperty::RadiusProperty);
	ParticlePropertyObject* shapeProperty = inputStandardProperty(ParticleProperty::AsphericalShapeProperty);

	// Compute bounding box of input particles.
	Box3 boundingBox;
	for(DisplayObject* displayObj : posProperty->displayObjects()) {
		if(ParticleDisplay* particleDisplay = dynamic_object_cast<ParticleDisplay>(displayObj)) {
			boundingBox.addBox(particleDisplay->particleBoundingBox(posProperty, typeProperty, radiusProperty, shapeProperty));
		}
	}

	// The render buffer resolution.
	int res = std::min(std::max(bufferResolution(), 0), (int)MAX_AO_RENDER_BUFFER_RESOLUTION);
	int resolution = (128 << res);

	TimeInterval interval;
	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<AmbientOcclusionEngine>(validityInterval, resolution, samplingCount(), posProperty->storage(), boundingBox, inputParticleRadii(time, interval), dataset());
}

/******************************************************************************
* Compute engine constructor.
******************************************************************************/
AmbientOcclusionModifier::AmbientOcclusionEngine::AmbientOcclusionEngine(const TimeInterval& validityInterval, int resolution, int samplingCount, ParticleProperty* positions, const Box3& boundingBox, std::vector<FloatType>&& particleRadii, DataSet* dataset) :
	ComputeEngine(validityInterval),
	_resolution(resolution),
	_samplingCount(samplingCount),
	_positions(positions),
	_boundingBox(boundingBox),
	_brightness(new ParticleProperty(positions->size(), qMetaTypeId<FloatType>(), 1, 0, tr("Brightness"), true)),
	_particleRadii(particleRadii),
	_dataset(dataset)
{
	_offscreenSurface.setFormat(OpenGLSceneRenderer::getDefaultSurfaceFormat());
	_offscreenSurface.create();
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void AmbientOcclusionModifier::AmbientOcclusionEngine::perform()
{
	if(_boundingBox.isEmpty() || positions()->size() == 0)
		throw Exception(tr("Modifier input is degenerate or contains no particles."));

	setProgressText(tr("Computing ambient occlusion"));

	// Create the AmbientOcclusionRenderer instance.
	OORef<AmbientOcclusionRenderer> renderer(new AmbientOcclusionRenderer(_dataset, QSize(_resolution, _resolution), _offscreenSurface));

	renderer->startRender(nullptr, nullptr);
	try {
		// The buffered particle geometry used to render the particles.
		std::shared_ptr<ParticlePrimitive> particleBuffer;

		setProgressMaximum(_samplingCount);
		for(int sample = 0; sample < _samplingCount; sample++) {
			if(!setProgressValue(sample))
				break;

			// Generate lighting direction on unit sphere.
			FloatType y = (FloatType)sample * 2 / _samplingCount - FloatType(1) + FloatType(1) / _samplingCount;
			FloatType r = sqrt(FloatType(1) - y * y);
			FloatType phi = (FloatType)sample * FLOATTYPE_PI * (FloatType(3) - sqrt(FloatType(5)));
			Vector3 dir(cos(phi), y, sin(phi));

			// Set up view projection.
			ViewProjectionParameters projParams;
			projParams.viewMatrix = AffineTransformation::lookAlong(_boundingBox.center(), dir, Vector3(0,0,1));

			// Transform bounding box to camera space.
			Box3 bb = _boundingBox.transformed(projParams.viewMatrix).centerScale(FloatType(1.01));

			// Complete projection parameters.
			projParams.aspectRatio = 1;
			projParams.isPerspective = false;
			projParams.inverseViewMatrix = projParams.viewMatrix.inverse();
			projParams.fieldOfView = 0.5f * _boundingBox.size().length();
			projParams.znear = -bb.maxc.z();
			projParams.zfar  = std::max(-bb.minc.z(), projParams.znear + 1.0f);
			projParams.projectionMatrix = Matrix4::ortho(-projParams.fieldOfView, projParams.fieldOfView,
								-projParams.fieldOfView, projParams.fieldOfView,
								projParams.znear, projParams.zfar);
			projParams.inverseProjectionMatrix = projParams.projectionMatrix.inverse();
			projParams.validityInterval = TimeInterval::infinite();

			renderer->beginFrame(0, projParams, nullptr);
			renderer->setWorldTransform(AffineTransformation::Identity());
			try {
				// Create particle buffer.
				if(!particleBuffer || !particleBuffer->isValid(renderer)) {
					particleBuffer = renderer->createParticlePrimitive(ParticlePrimitive::FlatShading, ParticlePrimitive::LowQuality, ParticlePrimitive::SphericalShape, false);
					particleBuffer->setSize(positions()->size());
					particleBuffer->setParticlePositions(positions()->constDataPoint3());
					particleBuffer->setParticleRadii(_particleRadii.data());
				}
				particleBuffer->render(renderer);
			}
			catch(...) {
				renderer->endFrame(false);
				throw;
			}
			renderer->endFrame(true);

			// Extract brightness values from rendered image.
			const QImage image = renderer->image();
			FloatType* brightnessValues = brightness()->dataFloat();
			for(int y = 0; y < _resolution; y++) {
				const QRgb* pixel = reinterpret_cast<const QRgb*>(image.scanLine(y));
				for(int x = 0; x < _resolution; x++, ++pixel) {
					quint32 red = qRed(*pixel);
					quint32 green = qGreen(*pixel);
					quint32 blue = qBlue(*pixel);
					quint32 alpha = qAlpha(*pixel);
					quint32 id = red + (green << 8) + (blue << 16) + (alpha << 24);
					if(id == 0)
						continue;
					quint32 particleIndex = id - 1;
					OVITO_ASSERT(particleIndex < positions()->size());
					brightnessValues[particleIndex] += 1;
				}
			}
		}
	}
	catch(...) {
		renderer->endRender();
		throw;
	}
	renderer->endRender();

	if(!isCanceled()) {
		setProgressValue(_samplingCount);
		// Normalize brightness values.
		FloatType maxBrightness = *std::max_element(brightness()->constDataFloat(), brightness()->constDataFloat() + brightness()->size());
		if(maxBrightness != 0) {
			for(FloatType& b : brightness()->floatRange()) {
				b /= maxBrightness;
			}
		}
	}
}

/******************************************************************************
* Unpacks the results of the computation engine and stores them in the modifier.
******************************************************************************/
void AmbientOcclusionModifier::transferComputationResults(ComputeEngine* engine)
{
	_brightnessValues = static_cast<AmbientOcclusionEngine*>(engine)->brightness();
}

/******************************************************************************
* Lets the modifier insert the cached computation results into the
* modification pipeline.
******************************************************************************/
PipelineStatus AmbientOcclusionModifier::applyComputationResults(TimePoint time, TimeInterval& validityInterval)
{
	if(!_brightnessValues)
		throwException(tr("No computation results available."));

	if(inputParticleCount() != _brightnessValues->size())
		throwException(tr("The number of input particles has changed. The stored results have become invalid."));

	// Get effect intensity.
	FloatType intens = std::min(std::max(intensity(), FloatType(0)), FloatType(1));

	// Get output property object.
	ParticlePropertyObject* colorProperty = outputStandardProperty(ParticleProperty::ColorProperty);
	OVITO_ASSERT(colorProperty->size() == _brightnessValues->size());

	std::vector<Color> existingColors = inputParticleColors(time, validityInterval);
	OVITO_ASSERT(colorProperty->size() == existingColors.size());
	const FloatType* b = _brightnessValues->constDataFloat();
	Color* c = colorProperty->dataColor();
	Color* c_end = c + colorProperty->size();
	auto c_in = existingColors.cbegin();
	for(; c != c_end; ++b, ++c, ++c_in) {
		FloatType factor = FloatType(1) - intens + (*b);
		if(factor < FloatType(1))
			*c = factor * (*c_in);
		else
			*c = *c_in;
	}
	colorProperty->changed();

	return PipelineStatus::Success;
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void AmbientOcclusionModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	AsynchronousParticleModifier::propertyChanged(field);

	// Recompute brightness values when the AO parameters have been changed.
	if(field == PROPERTY_FIELD(samplingCount) ||
		field == PROPERTY_FIELD(bufferResolution))
		invalidateCachedResults();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
