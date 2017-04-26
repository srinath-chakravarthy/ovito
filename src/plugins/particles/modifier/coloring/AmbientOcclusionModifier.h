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

#pragma once


#include <plugins/particles/Particles.h>
#include <plugins/particles/modifier/AsynchronousParticleModifier.h>

#include <QOffscreenSurface>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Coloring)

/**
 * \brief Calculates ambient occlusion lighting for particles.
 */
class OVITO_PARTICLES_EXPORT AmbientOcclusionModifier : public AsynchronousParticleModifier
{
public:

	enum { MAX_AO_RENDER_BUFFER_RESOLUTION = 4 };

	/// Computes the modifier's results.
	class AmbientOcclusionEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		AmbientOcclusionEngine(const TimeInterval& validityInterval, int resolution, int samplingCount, ParticleProperty* positions, const Box3& boundingBox, std::vector<FloatType>&& particleRadii, DataSet* dataset);

		/// Computes the modifier's results and stores them in this object for later retrieval.
		virtual void perform() override;

		/// Returns the property storage that contains the input particle positions.
		ParticleProperty* positions() const { return _positions.data(); }

		/// Returns the property storage that contains the computed per-particle brightness values.
		ParticleProperty* brightness() const { return _brightness.data(); }

	private:

		DataSet* _dataset;
		int _resolution;
		int _samplingCount;
		QExplicitlySharedDataPointer<ParticleProperty> _positions;
		QExplicitlySharedDataPointer<ParticleProperty> _brightness;
		Box3 _boundingBox;
		std::vector<FloatType> _particleRadii;
		QOffscreenSurface _offscreenSurface;
	};

public:

	/// Constructor.
	Q_INVOKABLE AmbientOcclusionModifier(DataSet* dataset);

protected:

	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// Creates and initializes a computation engine that will compute the modifier's results.
	virtual std::shared_ptr<ComputeEngine> createEngine(TimePoint time, TimeInterval validityInterval) override;

	/// Unpacks the results of the computation engine and stores them in the modifier.
	virtual void transferComputationResults(ComputeEngine* engine) override;

	/// Lets the modifier insert the cached computation results into the modification pipeline.
	virtual PipelineStatus applyComputationResults(TimePoint time, TimeInterval& validityInterval) override;

private:

	/// This stores the cached results of the modifier, i.e. the brightness value of each particle.
	QExplicitlySharedDataPointer<ParticleProperty> _brightnessValues;

	/// This controls the intensity of the shading effect.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, intensity, setIntensity);

	/// Controls the quality of the lighting computation.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, samplingCount, setSamplingCount);

	/// Controls the resolution of the offscreen rendering buffer.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, bufferResolution, setBufferResolution);

private:

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Ambient occlusion");
	Q_CLASSINFO("ModifierCategory", "Coloring");
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


