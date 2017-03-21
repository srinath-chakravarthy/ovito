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
#include <plugins/particles/objects/ParticlePropertyObject.h>
#include <plugins/particles/objects/BondPropertyObject.h>
#include <core/animation/controller/Controller.h>
#include <core/animation/AnimationSettings.h>
#include <core/rendering/ImagePrimitive.h>
#include <core/rendering/TextPrimitive.h>
#include <core/viewport/Viewport.h>
#include "../ParticleModifier.h"
#include "ColormapsData.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Coloring)

/**
 * \brief Abstract base class for color gradients that can be used with a ColorCodingModifier.
 *
 * Implementations of this class convert a scalar value in the range [0,1] to a color value.
 */
class OVITO_PARTICLES_EXPORT ColorCodingGradient : public RefTarget
{
protected:

	/// Constructor.
	ColorCodingGradient(DataSet* dataset) : RefTarget(dataset) {}

public:

	/// \brief Converts a scalar value to a color value.
	/// \param t A value between 0 and 1.
	/// \return The color that visualizes the given scalar value.
	virtual Color valueToColor(FloatType t) = 0;

private:

	Q_OBJECT
	OVITO_OBJECT
};

/**
 * \brief Converts a scalar value to a color using the HSV color system.
 */
class OVITO_PARTICLES_EXPORT ColorCodingHSVGradient : public ColorCodingGradient
{
public:

	/// Constructor.
	Q_INVOKABLE ColorCodingHSVGradient(DataSet* dataset) : ColorCodingGradient(dataset) {}

	/// \brief Converts a scalar value to a color value.
	/// \param t A value between 0 and 1.
	/// \return The color that visualizes the given scalar value.
	virtual Color valueToColor(FloatType t) override { return Color::fromHSV((FloatType(1) - t) * FloatType(0.7), 1, 1); }

private:

	Q_OBJECT
	OVITO_OBJECT
	Q_CLASSINFO("DisplayName", "Rainbow");
};

/**
 * \brief Converts a scalar value to a color using a gray-scale ramp.
 */
class OVITO_PARTICLES_EXPORT ColorCodingGrayscaleGradient : public ColorCodingGradient
{
public:

	/// Constructor.
	Q_INVOKABLE ColorCodingGrayscaleGradient(DataSet* dataset) : ColorCodingGradient(dataset) {}

	/// \brief Converts a scalar value to a color value.
	/// \param t A value between 0 and 1.
	/// \return The color that visualizes the given scalar value.
	virtual Color valueToColor(FloatType t) override { return Color(t, t, t); }

private:

	Q_OBJECT
	OVITO_OBJECT
	Q_CLASSINFO("DisplayName", "Grayscale");
};

/**
 * \brief Converts a scalar value to a color.
 */
class OVITO_PARTICLES_EXPORT ColorCodingHotGradient : public ColorCodingGradient
{
public:

	/// Constructor.
	Q_INVOKABLE ColorCodingHotGradient(DataSet* dataset) : ColorCodingGradient(dataset) {}

	/// \brief Converts a scalar value to a color value.
	/// \param t A value between 0 and 1.
	/// \return The color that visualizes the given scalar value.
	virtual Color valueToColor(FloatType t) override {
		// Interpolation black->red->yellow->white.
		OVITO_ASSERT(t >= 0 && t <= 1);
		return Color(std::min(t / FloatType(0.375), FloatType(1)), std::max(FloatType(0), std::min((t-FloatType(0.375))/FloatType(0.375), FloatType(1))), std::max(FloatType(0), t*4 - FloatType(3)));
	}

private:

	Q_OBJECT
	OVITO_OBJECT
	Q_CLASSINFO("DisplayName", "Hot");
};

/**
 * \brief Converts a scalar value to a color.
 */
class OVITO_PARTICLES_EXPORT ColorCodingJetGradient : public ColorCodingGradient
{
public:

	/// Constructor.
	Q_INVOKABLE ColorCodingJetGradient(DataSet* dataset) : ColorCodingGradient(dataset) {}

	/// \brief Converts a scalar value to a color value.
	/// \param t A value between 0 and 1.
	/// \return The color that visualizes the given scalar value.
	virtual Color valueToColor(FloatType t) override {
	    if(t < FloatType(0.125)) return Color(0, 0, FloatType(0.5) + FloatType(0.5) * t / FloatType(0.125));
	    else if(t < FloatType(0.125) + FloatType(0.25)) return Color(0, (t - FloatType(0.125)) / FloatType(0.25), 1);
	    else if(t < FloatType(0.125) + FloatType(0.25) + FloatType(0.25)) return Color((t - FloatType(0.375)) / FloatType(0.25), 1, FloatType(1) - (t - FloatType(0.375)) / FloatType(0.25));
	    else if(t < FloatType(0.125) + FloatType(0.25) + FloatType(0.25) + FloatType(0.25)) return Color(1, FloatType(1) - (t - FloatType(0.625)) / FloatType(0.25), 0);
	    else return Color(FloatType(1) - FloatType(0.5) * (t - FloatType(0.875)) / FloatType(0.125), 0, 0);
	}

private:

	Q_OBJECT
	OVITO_OBJECT
	Q_CLASSINFO("DisplayName", "Jet");
};

/**
 * \brief Converts a scalar value to a color.
 */
class OVITO_PARTICLES_EXPORT ColorCodingBlueWhiteRedGradient : public ColorCodingGradient
{
public:

	/// Constructor.
	Q_INVOKABLE ColorCodingBlueWhiteRedGradient(DataSet* dataset) : ColorCodingGradient(dataset) {}

	/// \brief Converts a scalar value to a color value.
	/// \param t A value between 0 and 1.
	/// \return The color that visualizes the given scalar value.
	virtual Color valueToColor(FloatType t) override {
		if(t <= FloatType(0.5))
			return Color(t * 2, t * 2, 1);
		else
			return Color(1, (FloatType(1)-t) * 2, (FloatType(1)-t) * 2);
	}

private:

	Q_OBJECT
	OVITO_OBJECT
	Q_CLASSINFO("DisplayName", "Blue-White-Red");
};

/**
 * \brief Converts a scalar value to a color.
 */
class OVITO_PARTICLES_EXPORT ColorCodingViridisGradient : public ColorCodingGradient
{
public:

	/// Constructor.
	Q_INVOKABLE ColorCodingViridisGradient(DataSet* dataset) : ColorCodingGradient(dataset) {}

	/// \brief Converts a scalar value to a color value.
	/// \param t A value between 0 and 1.
	/// \return The color that visualizes the given scalar value.
	virtual Color valueToColor(FloatType t) override {
		int index = t * (sizeof(colormap_viridis_data)/sizeof(colormap_viridis_data[0]) - 1);
		return Color(colormap_viridis_data[index][0], colormap_viridis_data[index][1], colormap_viridis_data[index][2]);
	}

private:

	Q_OBJECT
	OVITO_OBJECT
	Q_CLASSINFO("DisplayName", "Viridis");
};

/**
 * \brief Converts a scalar value to a color.
 */
class OVITO_PARTICLES_EXPORT ColorCodingMagmaGradient : public ColorCodingGradient
{
public:

	/// Constructor.
	Q_INVOKABLE ColorCodingMagmaGradient(DataSet* dataset) : ColorCodingGradient(dataset) {}

	/// \brief Converts a scalar value to a color value.
	/// \param t A value between 0 and 1.
	/// \return The color that visualizes the given scalar value.
	virtual Color valueToColor(FloatType t) override {
		int index = t * (sizeof(colormap_magma_data)/sizeof(colormap_magma_data[0]) - 1);
		return Color(colormap_magma_data[index][0], colormap_magma_data[index][1], colormap_magma_data[index][2]);
	}

private:

	Q_OBJECT
	OVITO_OBJECT
	Q_CLASSINFO("DisplayName", "Magma");
};

/**
 * \brief Converts a scalar value to a color based on a user-defined image.
 */
class OVITO_PARTICLES_EXPORT ColorCodingImageGradient : public ColorCodingGradient
{
public:

	/// Constructor.
	Q_INVOKABLE ColorCodingImageGradient(DataSet* dataset) : ColorCodingGradient(dataset) {
		INIT_PROPERTY_FIELD(image);
	}

	/// \brief Converts a scalar value to a color value.
	/// \param t A value between 0 and 1.
	/// \return The color that visualizes the given scalar value.
	virtual Color valueToColor(FloatType t) override;

	/// Loads the given image file from disk.
	void loadImage(const QString& filename);

private:

	/// The user-defined color map image.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QImage, image, setImage);

	Q_OBJECT
	OVITO_OBJECT
	Q_CLASSINFO("DisplayName", "User image");
};


/**
 * \brief This modifier assigns a colors to the particles or bonds based on the value of a property.
 */
class OVITO_PARTICLES_EXPORT ColorCodingModifier : public ParticleModifier
{
public:

	/// The modes supported by the color coding modifier.
	enum ColorApplicationMode {
		Particles,	//< Colors are assigned to particles
		Bonds,		//< Colors are assigned to bonds
		Vectors		//< Colors are assigned to vector arrows
	};
	Q_ENUMS(ColorApplicationMode);

public:

	/// Constructor.
	Q_INVOKABLE ColorCodingModifier(DataSet* dataset);

	/// Loads the user-defined default values of this object's parameter fields from the
	/// application's settings store.
	virtual void loadUserDefaults() override;

	/// Asks the modifier for its validity interval at the given time.
	virtual TimeInterval modifierValidity(TimePoint time) override;

	/// Returns the range start value.
	FloatType startValue() const { return startValueController() ? startValueController()->currentFloatValue() : 0; }

	/// Sets the range start value.
	void setStartValue(FloatType value) { if(startValueController()) startValueController()->setCurrentFloatValue(value); }

	/// Returns the range end value.
	FloatType endValue() const { return endValueController() ? endValueController()->currentFloatValue() : 0; }

	/// Sets the range end value.
	void setEndValue(FloatType value) { if(endValueController()) endValueController()->setCurrentFloatValue(value); }

	/// Sets the start and end value to the minimum and maximum value of the selected particle or bond property
	/// determined over the entire animation sequence.
	bool adjustRangeGlobal(TaskManager& taskManager);

public Q_SLOTS:

	/// Sets the start and end value to the minimum and maximum value of the selected particle or bond property.
	/// Returns true if successful.
	bool adjustRange();

protected:

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream) override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

	/// Parses the serialized contents of a property field in a custom way.
	virtual bool loadPropertyFieldFromStream(ObjectLoadStream& stream, const ObjectLoadStream::SerializedPropertyField& serializedField) override;

	/// This virtual method is called by the system when the modifier has been inserted into a PipelineObject.
	virtual void initializeModifier(PipelineObject* pipelineObject, ModifierApplication* modApp) override;

	/// Modifies the particles.
	virtual PipelineStatus modifyParticles(TimePoint time, TimeInterval& validityInterval) override;

	/// Determines the range of values in the input data for the selected property.
	bool determinePropertyValueRange(const PipelineFlowState& state, FloatType& min, FloatType& max);

private:

	/// This controller stores the start value of the color scale.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(Controller, startValueController, setStartValueController);

	/// This controller stores the end value of the color scale.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(Controller, endValueController, setEndValueController);

	/// This object converts scalar atom properties to colors.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(ColorCodingGradient, colorGradient, setColorGradient);

	/// The particle type property that is used as source for the coloring.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(ParticlePropertyReference, sourceParticleProperty, setSourceParticleProperty);

	/// The bond type property that is used as source for the coloring.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(BondPropertyReference, sourceBondProperty, setSourceBondProperty);

	/// Controls whether the modifier assigns a color only to selected particles.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, colorOnlySelected, setColorOnlySelected);

	/// Controls whether the input particle selection is preserved.
	/// If false, the selection is cleared by the modifier.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, keepSelection, setKeepSelection);

	/// Controls what is being assigned colors by the modifier.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(ColorApplicationMode, colorApplicationMode, setColorApplicationMode);

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Color coding");
	Q_CLASSINFO("ModifierCategory", "Coloring");
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::Particles::ColorCodingModifier::ColorApplicationMode);
Q_DECLARE_TYPEINFO(Ovito::Particles::ColorCodingModifier::ColorApplicationMode, Q_PRIMITIVE_TYPE);


