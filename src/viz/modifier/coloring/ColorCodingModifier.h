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

#ifndef __OVITO_COLOR_CODING_MODIFIER_H
#define __OVITO_COLOR_CODING_MODIFIER_H

#include <core/Core.h>
#include <core/animation/controller/Controller.h>
#include <viz/util/ParticlePropertyComboBox.h>
#include "../ParticleModifier.h"

namespace Viz {

using namespace Ovito;

/*
 * Abstract base class for color gradients that can be used with the ColorCodingModifier.
 * It converts a scalar value in the range [0,1] to a color value.
 */
class ColorCodingGradient : public RefTarget
{
protected:

	/// Default constructor.
	ColorCodingGradient() {}

public:

	/// \brief Converts a scalar value to a color value.
	/// \param t A value between 0 and 1.
	/// \return The color that visualizes the given scalar value.
	virtual Color valueToColor(FloatType t) = 0;

private:

	Q_OBJECT
	OVITO_OBJECT
};

/*
 * Converts a scalar value to a color using the HSV color system.
 */
class ColorCodingHSVGradient : public ColorCodingGradient
{
public:

	/// Default constructor.
	Q_INVOKABLE ColorCodingHSVGradient() {}

	/// \brief Converts a scalar value to a color value.
	/// \param t A value between 0 and 1.
	/// \return The color that visualizes the given scalar value.
	virtual Color valueToColor(FloatType t) override { return Color::fromHSV((FloatType(1) - t) * FloatType(0.7), 1, 1); }

private:

	Q_OBJECT
	OVITO_OBJECT
	Q_CLASSINFO("DisplayName", "Rainbow");
};

/*
 * Converts a scalar value to a color using a gray-scale ramp.
 */
class ColorCodingGrayscaleGradient : public ColorCodingGradient
{
public:

	/// Default constructor.
	Q_INVOKABLE ColorCodingGrayscaleGradient() {}

	/// \brief Converts a scalar value to a color value.
	/// \param t A value between 0 and 1.
	/// \return The color that visualizes the given scalar value.
	virtual Color valueToColor(FloatType t) override { return Color(t, t, t); }

private:

	Q_OBJECT
	OVITO_OBJECT
	Q_CLASSINFO("DisplayName", "Grayscale");
};

/*
 * Converts a scalar value to a color.
 */
class ColorCodingHotGradient : public ColorCodingGradient
{
public:

	/// Default constructor.
	Q_INVOKABLE ColorCodingHotGradient() {}

	/// \brief Converts a scalar value to a color value.
	/// \param t A value between 0 and 1.
	/// \return The color that visualizes the given scalar value.
	virtual Color valueToColor(FloatType t) override {
		// Interpolation black->red->yellow->white.
		OVITO_ASSERT(t >= 0.0f && t <= 1.0f);
		return Color(std::min(t / 0.375f, FloatType(1)), std::max(FloatType(0), std::min((t-0.375f)/0.375f, FloatType(1))), std::max(FloatType(0), t*4.0f - 3.0f));
	}

private:

	Q_OBJECT
	OVITO_OBJECT
	Q_CLASSINFO("DisplayName", "Hot");
};

/*
 * Converts a scalar value to a color.
 */
class ColorCodingJetGradient : public ColorCodingGradient
{
public:

	/// Default constructor.
	Q_INVOKABLE ColorCodingJetGradient() {}

	/// \brief Converts a scalar value to a color value.
	/// \param t A value between 0 and 1.
	/// \return The color that visualizes the given scalar value.
	virtual Color valueToColor(FloatType t) override {
	    if(t < 0.125f) return Color(0, 0, 0.5f + 0.5f * t / 0.125f);
	    else if(t < 0.125f + 0.25f) return Color(0, (t - 0.125f) / 0.25f, 1);
	    else if(t < 0.125f + 0.25f + 0.25f) return Color((t - 0.375f) / 0.25f, 1, 1.0f - (t - 0.375f) / 0.25f);
	    else if(t < 0.125f + 0.25f + 0.25f + 0.25f) return Color(1, 1.0f - (t - 0.625f) / 0.25f, 0);
	    else return Color(1.0f - 0.5f * (t - 0.875f) / 0.125f, 0, 0);
	}

private:

	Q_OBJECT
	OVITO_OBJECT
	Q_CLASSINFO("DisplayName", "Jet");
};


/*
 * This modifier assigns a colors to the particles based on the value of a
 * selected particle property.
 */
class ColorCodingModifier : public ParticleModifier
{
public:

	/// Default constructor.
	Q_INVOKABLE ColorCodingModifier();

	/// Asks the modifier for its validity interval at the given time.
	virtual TimeInterval modifierValidity(TimePoint time) override;

	/// This virtual method is called by the system when the modifier has been inserted into a PipelineObject.
	virtual void initializeModifier(PipelineObject* pipelineObject, ModifierApplication* modApp) override;

	/// Sets the source particle property that is used for coloring of particles.
	void setSourceProperty(const ParticlePropertyReference& prop);

	/// Returns the source particle property that is used for coloring of particles.
	const ParticlePropertyReference& sourceProperty() const { return _sourcePropertyRef; }

	/// Returns the range start value.
	FloatType startValue() const { return _startValueCtrl ? _startValueCtrl->currentValue() : 0; }

	/// Sets the range start value.
	void setStartValue(FloatType value) { if(_startValueCtrl) _startValueCtrl->setCurrentValue(value); }

	/// Returns the controller for the range start value.
	FloatController* startValueController() const { return _startValueCtrl; }

	/// Sets the controller for the range start value.
	void setStartValueController(const OORef<FloatController>& ctrl) { _startValueCtrl = ctrl; }

	/// Returns the range end value.
	FloatType endValue() const { return _endValueCtrl ? _endValueCtrl->currentValue() : 0; }

	/// Sets the range end value.
	void setEndValue(FloatType value) { if(_endValueCtrl) _endValueCtrl->setCurrentValue(value); }

	/// Returns the controller for the range end value.
	FloatController* endValueController() const { return _endValueCtrl; }

	/// Sets the controller for the range end value.
	void setEndValueController(const OORef<FloatController>& ctrl) { _endValueCtrl = ctrl; }

	/// Returns the color gradient used by the modifier to convert scalar atom properties to colors.
	ColorCodingGradient* colorGradient() const { return _colorGradient; }

	/// Sets the color gradient for the modifier to convert scalar atom properties to colors.
	void setColorGradient(const OORef<ColorCodingGradient>& gradient) { _colorGradient = gradient; }

	/// Sets the start and end value to the minimum and maximum value in the selected data channel.
	bool adjustRange();

	/// Retrieves the selected input particle property from the given modifier input state.
	ParticlePropertyObject* lookupInputProperty(const PipelineFlowState& inputState) const;

public:

	Q_PROPERTY(FloatType startValue READ startValue WRITE setStartValue)
	Q_PROPERTY(FloatType endValue READ endValue WRITE setEndValue)
	Q_PROPERTY(Viz::ParticlePropertyReference sourceProperty READ sourceProperty WRITE setSourceProperty)

protected:

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream) override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

	/// Creates a copy of this object.
	virtual OORef<RefTarget> clone(bool deepCopy, CloneHelper& cloneHelper) override;

	/// Modifies the particle object.
	virtual ObjectStatus modifyParticles(TimePoint time, TimeInterval& validityInterval) override;

	/// This controller stores the start value of the color scale.
	ReferenceField<FloatController> _startValueCtrl;

	/// This controller stores the end value of the color scale.
	ReferenceField<FloatController> _endValueCtrl;

	/// This object converts scalar atom properties to colors.
	ReferenceField<ColorCodingGradient> _colorGradient;

	/// The particle type property that is used as source for the coloring.
	ParticlePropertyReference _sourcePropertyRef;

	friend class ColorCodingModifierEditor;

private:

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Color Coding");
	Q_CLASSINFO("ModifierCategory", "Coloring");

	DECLARE_REFERENCE_FIELD(_startValueCtrl);
	DECLARE_REFERENCE_FIELD(_endValueCtrl);
	DECLARE_REFERENCE_FIELD(_colorGradient);
};

/*
 * A properties editor for the ColorCodingModifier class.
 */
class ColorCodingModifierEditor : public ParticleModifierEditor
{
public:

	/// Default constructor.
	Q_INVOKABLE ColorCodingModifierEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

private:

	/// The list of particle type properties.
	ParticlePropertyComboBox* propertyListBox;

	/// The list of available color gradients.
	QComboBox* colorGradientList;

	/// Label that displays the color gradient picture.
	QLabel* colorLegendLabel;

protected Q_SLOTS:

	/// Updates the contents of the property list combo box.
	void updatePropertyList();

	/// Updates the display for the color gradient.
	void updateColorGradient();

	/// This is called when the user has selected another item in the particle property list.
	void onPropertySelected(int index);

	/// Is called when the user selects a color gradient in the list box.
	void onColorGradientSelected(int index);

	/// Is called when the user presses the "Adjust Range" button.
	void onAdjustRange();

	/// Is called when the user presses the "Reverse Range" button.
	void onReverseRange();

	/// Is called when the user presses the "Export color scale" button.
	void onExportColorScale();

protected:

	/// This method is called when a reference target changes.
	virtual bool referenceEvent(RefTarget* source, ReferenceEvent* event) override;

private:

	Q_OBJECT
	OVITO_OBJECT
};

};	// End of namespace

#endif // __OVITO_COLOR_CODING_MODIFIER_H