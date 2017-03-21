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

#include <plugins/particles/Particles.h>
#include <core/scene/pipeline/PipelineObject.h>
#include <core/animation/controller/Controller.h>
#include <core/reference/CloneHelper.h>
#include <core/plugins/PluginManager.h>
#include <core/rendering/SceneRenderer.h>
#include <core/viewport/Viewport.h>
#include <core/viewport/ViewportConfiguration.h>
#include <core/utilities/concurrent/Task.h>
#include <core/utilities/concurrent/TaskManager.h>
#include "ColorCodingModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Coloring)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(ColorCodingModifier, ParticleModifier);
DEFINE_REFERENCE_FIELD(ColorCodingModifier, startValueController, "StartValue", Controller);
DEFINE_REFERENCE_FIELD(ColorCodingModifier, endValueController, "EndValue", Controller);
DEFINE_REFERENCE_FIELD(ColorCodingModifier, colorGradient, "ColorGradient", ColorCodingGradient);
DEFINE_PROPERTY_FIELD(ColorCodingModifier, colorOnlySelected, "SelectedOnly");
DEFINE_PROPERTY_FIELD(ColorCodingModifier, keepSelection, "KeepSelection");
DEFINE_PROPERTY_FIELD(ColorCodingModifier, sourceParticleProperty, "SourceProperty");
DEFINE_PROPERTY_FIELD(ColorCodingModifier, sourceBondProperty, "SourceBondProperty");
DEFINE_PROPERTY_FIELD(ColorCodingModifier, colorApplicationMode, "ColorApplicationMode");
SET_PROPERTY_FIELD_LABEL(ColorCodingModifier, startValueController, "Start value");
SET_PROPERTY_FIELD_LABEL(ColorCodingModifier, endValueController, "End value");
SET_PROPERTY_FIELD_LABEL(ColorCodingModifier, colorGradient, "Color gradient");
SET_PROPERTY_FIELD_LABEL(ColorCodingModifier, colorOnlySelected, "Color only selected particles/bonds");
SET_PROPERTY_FIELD_LABEL(ColorCodingModifier, keepSelection, "Keep selection");
SET_PROPERTY_FIELD_LABEL(ColorCodingModifier, sourceParticleProperty, "Source property");
SET_PROPERTY_FIELD_LABEL(ColorCodingModifier, sourceBondProperty, "Source property");
SET_PROPERTY_FIELD_LABEL(ColorCodingModifier, colorApplicationMode, "Target");

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(ColorCodingGradient, RefTarget);
IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(ColorCodingHSVGradient, ColorCodingGradient);
IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(ColorCodingGrayscaleGradient, ColorCodingGradient);
IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(ColorCodingHotGradient, ColorCodingGradient);
IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(ColorCodingJetGradient, ColorCodingGradient);
IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(ColorCodingBlueWhiteRedGradient, ColorCodingGradient);
IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(ColorCodingViridisGradient, ColorCodingGradient);
IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(ColorCodingMagmaGradient, ColorCodingGradient);
IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(ColorCodingImageGradient, ColorCodingGradient);
DEFINE_PROPERTY_FIELD(ColorCodingImageGradient, image, "Image");

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
ColorCodingModifier::ColorCodingModifier(DataSet* dataset) : ParticleModifier(dataset),
	_colorOnlySelected(false), _keepSelection(false), _colorApplicationMode(Particles)
{
	INIT_PROPERTY_FIELD(startValueController);
	INIT_PROPERTY_FIELD(endValueController);
	INIT_PROPERTY_FIELD(colorGradient);
	INIT_PROPERTY_FIELD(colorOnlySelected);
	INIT_PROPERTY_FIELD(keepSelection);
	INIT_PROPERTY_FIELD(sourceParticleProperty);
	INIT_PROPERTY_FIELD(sourceBondProperty);
	INIT_PROPERTY_FIELD(colorApplicationMode);

	setColorGradient(new ColorCodingHSVGradient(dataset));
	setStartValueController(ControllerManager::createFloatController(dataset));
	setEndValueController(ControllerManager::createFloatController(dataset));
}

/******************************************************************************
* Loads the user-defined default values of this object's parameter fields from the
* application's settings store.
******************************************************************************/
void ColorCodingModifier::loadUserDefaults()
{
	ParticleModifier::loadUserDefaults();

	// Load the default gradient type set by the user.
	QSettings settings;
	settings.beginGroup(ColorCodingModifier::OOType.plugin()->pluginId());
	settings.beginGroup(ColorCodingModifier::OOType.name());
	QString typeString = settings.value(PROPERTY_FIELD(colorGradient).identifier()).toString();
	if(!typeString.isEmpty()) {
		try {
			OvitoObjectType* gradientType = OvitoObjectType::decodeFromString(typeString);
			if(!colorGradient() || colorGradient()->getOOType() != *gradientType) {
				OORef<ColorCodingGradient> gradient = dynamic_object_cast<ColorCodingGradient>(gradientType->createInstance(dataset()));
				if(gradient) setColorGradient(gradient);
			}
		}
		catch(...) {}
	}
}

/******************************************************************************
* Asks the modifier for its validity interval at the given time.
******************************************************************************/
TimeInterval ColorCodingModifier::modifierValidity(TimePoint time)
{
	TimeInterval interval = ParticleModifier::modifierValidity(time);
	if(startValueController()) interval.intersect(startValueController()->validityInterval(time));
	if(endValueController()) interval.intersect(endValueController()->validityInterval(time));
	return interval;
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a pipeline.
******************************************************************************/
void ColorCodingModifier::initializeModifier(PipelineObject* pipeline, ModifierApplication* modApp)
{
	ParticleModifier::initializeModifier(pipeline, modApp);

	// Select the first available particle property from the input by default.
	if(sourceParticleProperty().isNull()) {
		PipelineFlowState input = getModifierInput(modApp);
		ParticlePropertyReference bestProperty;
		for(DataObject* o : input.objects()) {
			ParticlePropertyObject* property = dynamic_object_cast<ParticlePropertyObject>(o);
			if(property && (property->dataType() == qMetaTypeId<int>() || property->dataType() == qMetaTypeId<FloatType>())) {
				bestProperty = ParticlePropertyReference(property, (property->componentCount() > 1) ? 0 : -1);
			}
		}
		if(!bestProperty.isNull())
			setSourceParticleProperty(bestProperty);
	}

	// Select the first available bond property from the input by default.
	if(sourceBondProperty().isNull()) {
		PipelineFlowState input = getModifierInput(modApp);
		BondPropertyReference bestProperty;
		for(DataObject* o : input.objects()) {
			BondPropertyObject* property = dynamic_object_cast<BondPropertyObject>(o);
			if(property && (property->dataType() == qMetaTypeId<int>() || property->dataType() == qMetaTypeId<FloatType>())) {
				bestProperty = BondPropertyReference(property, (property->componentCount() > 1) ? 0 : -1);
			}
		}
		if(!bestProperty.isNull())
			setSourceBondProperty(bestProperty);
	}

	// Automatically adjust value range.
	if(startValue() == 0 && endValue() == 0)
		adjustRange();
}

/******************************************************************************
* This modifies the input object.
******************************************************************************/
PipelineStatus ColorCodingModifier::modifyParticles(TimePoint time, TimeInterval& validityInterval)
{
	// Get the source property.
	PropertyBase* property;
	PropertyBase* selProperty = nullptr;
	PropertyBase* colorProperty;
	DataObject* selectionPropertyObj = nullptr;
	int vecComponent;
	if(colorApplicationMode() != ColorCodingModifier::Bonds) {
		if(sourceParticleProperty().isNull())
			throwException(tr("Select a particle property first."));
		ParticlePropertyObject* propertyObj = sourceParticleProperty().findInState(input());
		if(!propertyObj)
			throwException(tr("The particle property with the name '%1' does not exist.").arg(sourceParticleProperty().name()));
		property = propertyObj->storage();
		if(sourceParticleProperty().vectorComponent() >= (int)property->componentCount())
			throwException(tr("The vector component is out of range. The particle property '%1' contains only %2 values per particle.").arg(sourceParticleProperty().name()).arg(property->componentCount()));
		vecComponent = std::max(0, sourceParticleProperty().vectorComponent());
		
		// Get the particle selection property if enabled by the user.
		if(colorOnlySelected()) {
			if(ParticlePropertyObject* selPropertyObj = inputStandardProperty(ParticleProperty::SelectionProperty)) {
				selProperty = selPropertyObj->storage();
				selectionPropertyObj = selPropertyObj;
			}
		}

		// Create the color output property.
		ParticlePropertyObject* colorPropertyObj;
		if(colorApplicationMode() == Particles)
			colorPropertyObj = outputStandardProperty(ParticleProperty::ColorProperty);
		else
			colorPropertyObj = outputStandardProperty(ParticleProperty::VectorColorProperty);
		colorProperty = colorPropertyObj->modifiableStorage();
	}
	else {
		if(sourceBondProperty().isNull())
			throwException(tr("Select a bond property first."));
		BondPropertyObject* propertyObj = sourceBondProperty().findInState(input());
		if(!propertyObj)
			throwException(tr("The bond property with the name '%1' does not exist.").arg(sourceBondProperty().name()));
		property = propertyObj->storage();
		if(sourceBondProperty().vectorComponent() >= (int)property->componentCount())
			throwException(tr("The vector component is out of range. The bond property '%1' contains only %2 values per bond.").arg(sourceBondProperty().name()).arg(property->componentCount()));
		vecComponent = std::max(0, sourceBondProperty().vectorComponent());

		// Get the bond selection property if enabled by the user.
		if(colorOnlySelected()) {
			if(BondPropertyObject* selPropertyObj = inputStandardBondProperty(BondProperty::SelectionProperty)) {
				selProperty = selPropertyObj->storage();
				selectionPropertyObj = selPropertyObj;
			}
		}

		// Create the color output property.
		BondPropertyObject* colorPropertyObj = outputStandardBondProperty(BondProperty::ColorProperty);
		colorProperty = colorPropertyObj->modifiableStorage();
	}

	int stride = property->stride() / property->dataTypeSize();

	if(!_colorGradient)
		throwException(tr("No color gradient has been selected."));

	// Get modifier's parameter values.
	FloatType startValue = 0, endValue = 0;
	if(startValueController()) startValue = startValueController()->getFloatValue(time, validityInterval);
	if(endValueController()) endValue = endValueController()->getFloatValue(time, validityInterval);

	// Clamp to finite range.
	if(!std::isfinite(startValue)) startValue = std::numeric_limits<FloatType>::min();
	if(!std::isfinite(endValue)) endValue = std::numeric_limits<FloatType>::max();

	// Get the particle selection property if enabled by the user.
	const int* sel = nullptr;
	std::vector<Color> existingColors;
	if(selProperty) {
		sel = selProperty->constDataInt();
		if(colorApplicationMode() != ColorCodingModifier::Bonds)
			existingColors = inputParticleColors(time, validityInterval);
		else
			existingColors = inputBondColors(time, validityInterval);
		OVITO_ASSERT(existingColors.size() == property->size());
	}

	OVITO_ASSERT(colorProperty->size() == property->size());
	Color* c_begin = colorProperty->dataColor();
	Color* c_end = c_begin + colorProperty->size();
	Color* c = c_begin;

	if(property->dataType() == qMetaTypeId<FloatType>()) {
		const FloatType* v = property->constDataFloat() + vecComponent;
		for(; c != c_end; ++c, v += stride) {

			// If the "only selected" option is enabled, and the particle is not selected, use the existing particle color.
			if(sel && !(*sel++)) {
				*c = existingColors[c - c_begin];
				continue;
			}

			// Compute linear interpolation.
			FloatType t;
			if(startValue == endValue) {
				if((*v) == startValue) t = 0.5;
				else if((*v) > startValue) t = 1;
				else t = 0;
			}
			else t = ((*v) - startValue) / (endValue - startValue);

			// Clamp values.
			if(std::isnan(t)) t = 0;
			else if(t == std::numeric_limits<FloatType>::infinity()) t = 1;
			else if(t == -std::numeric_limits<FloatType>::infinity()) t = 0;
			else if(t < 0) t = 0;
			else if(t > 1) t = 1;

			*c = _colorGradient->valueToColor(t);
		}
	}
	else if(property->dataType() == qMetaTypeId<int>()) {
		const int* v = property->constDataInt() + vecComponent;
		for(; c != c_end; ++c, v += stride) {

			// If the "only selected" option is enabled, and the particle is not selected, use the existing particle color.
			if(sel && !(*sel++)) {
				*c = existingColors[c - c_begin];
				continue;
			}

			// Compute linear interpolation.
			FloatType t;
			if(startValue == endValue) {
				if((*v) == startValue) t = 0.5;
				else if((*v) > startValue) t = 1;
				else t = 0;
			}
			else t = ((*v) - startValue) / (endValue - startValue);

			// Clamp values.
			if(t < 0) t = 0;
			else if(t > 1) t = 1;

			*c = _colorGradient->valueToColor(t);
		}
	}
	else
		throwException(tr("The property '%1' has an invalid or non-numeric data type.").arg(property->name()));

	// Clear selection if requested.
	if(selectionPropertyObj && !keepSelection()) {
		output().removeObject(selectionPropertyObj);
	}

	if(colorApplicationMode() == ColorCodingModifier::Particles)
		outputStandardProperty(ParticleProperty::ColorProperty)->changed();
	else if(colorApplicationMode() == ColorCodingModifier::Vectors)
		outputStandardProperty(ParticleProperty::VectorColorProperty)->changed();
	else if(colorApplicationMode() == ColorCodingModifier::Bonds)
		outputStandardBondProperty(BondProperty::ColorProperty)->changed();

	return PipelineStatus::Success;
}

/******************************************************************************
* Determines the range of values in the input data for the selected property.
******************************************************************************/
bool ColorCodingModifier::determinePropertyValueRange(const PipelineFlowState& state, FloatType& min, FloatType& max)
{
	PropertyBase* property;
	int vecComponent;
	if(colorApplicationMode() != ColorCodingModifier::Bonds) {
		ParticlePropertyObject* propertyObj = sourceParticleProperty().findInState(state);
		if(!propertyObj)
			return false;
		property = propertyObj->storage();
		if(sourceParticleProperty().vectorComponent() >= (int)property->componentCount())
			return false;
		vecComponent = std::max(0, sourceParticleProperty().vectorComponent());
	}
	else {
		BondPropertyObject* propertyObj = sourceBondProperty().findInState(state);
		if(!propertyObj)
			return false;
		property = propertyObj->storage();
		if(sourceBondProperty().vectorComponent() >= (int)property->componentCount())
			return false;
		vecComponent = std::max(0, sourceBondProperty().vectorComponent());
	}

	int stride = property->stride() / property->dataTypeSize();

	// Iterate over all particles/bonds.
	FloatType maxValue = FLOATTYPE_MIN;
	FloatType minValue = FLOATTYPE_MAX;
	if(property->dataType() == qMetaTypeId<FloatType>()) {
		const FloatType* v = property->constDataFloat() + vecComponent;
		const FloatType* vend = v + (property->size() * stride);
		for(; v != vend; v += stride) {
			if(*v > maxValue) maxValue = *v;
			if(*v < minValue) minValue = *v;
		}
	}
	else if(property->dataType() == qMetaTypeId<int>()) {
		const int* v = property->constDataInt() + vecComponent;
		const int* vend = v + (property->size() * stride);
		for(; v != vend; v += stride) {
			if(*v > maxValue) maxValue = *v;
			if(*v < minValue) minValue = *v;
		}
	}
	if(minValue == FLOATTYPE_MAX)
		return false;

	// Clamp to finite range.
	if(!std::isfinite(minValue)) minValue = std::numeric_limits<FloatType>::min();
	if(!std::isfinite(maxValue)) maxValue = std::numeric_limits<FloatType>::max();

	if(minValue < min) min = minValue;
	if(maxValue > min) max = maxValue;

	return true;
}

/******************************************************************************
* Sets the start and end value to the minimum and maximum value
* in the selected particle or bond property.
* Returns true if successful.
******************************************************************************/
bool ColorCodingModifier::adjustRange()
{
	// Get the input data.
	PipelineFlowState inputState = getModifierInput();

	// Determine the minimum and maximum values of the selected property.
	FloatType minValue = std::numeric_limits<FloatType>::max();
	FloatType maxValue = std::numeric_limits<FloatType>::min();
	if(!determinePropertyValueRange(inputState, minValue, maxValue))
		return false;

	// Adjust range of color coding.
	if(startValueController())
		startValueController()->setCurrentFloatValue(minValue);
	if(endValueController())
		endValueController()->setCurrentFloatValue(maxValue);

	return true;
}

/******************************************************************************
* Sets the start and end value to the minimum and maximum value of the selected 
* particle or bond property determined over the entire animation sequence.
******************************************************************************/
bool ColorCodingModifier::adjustRangeGlobal(TaskManager& taskManager)
{
	ViewportSuspender noVPUpdates(this);
	SynchronousTask task(taskManager);

	TimeInterval interval = dataset()->animationSettings()->animationInterval();
	task.setProgressMaximum(interval.duration() / dataset()->animationSettings()->ticksPerFrame() + 1);

	FloatType minValue = std::numeric_limits<FloatType>::max();
	FloatType maxValue = std::numeric_limits<FloatType>::min();

	TimePoint oldAnimTime = dataset()->animationSettings()->time();
	for(TimePoint time = interval.start(); time <= interval.end(); time += dataset()->animationSettings()->ticksPerFrame()) {
		if(task.isCanceled()) break;
		task.setProgressText(tr("Analyzing frame %1").arg(dataset()->animationSettings()->timeToFrame(time)));
		dataset()->animationSettings()->setTime(time);

		for(ModifierApplication* modApp : modifierApplications()) {

			PipelineObject* pipelineObj = modApp->pipelineObject();
			if(!pipelineObj) continue;
			
			Future<PipelineFlowState> stateFuture = pipelineObj->evaluateAsync(PipelineEvalRequest(time, false, modApp, false));
			if(!taskManager.waitForTask(stateFuture))
				break;

			determinePropertyValueRange(stateFuture.result(), minValue, maxValue);
		}
		task.setProgressValue(task.progressValue() + 1);
	}
	dataset()->animationSettings()->setTime(oldAnimTime);

	if(!task.isCanceled()) {
		// Adjust range of color coding.
		if(startValueController() && minValue != std::numeric_limits<FloatType>::max())
			startValueController()->setCurrentFloatValue(minValue);
		if(endValueController() && maxValue != std::numeric_limits<FloatType>::min())
			endValueController()->setCurrentFloatValue(maxValue);

		return true;
	}
	return false;
}

/******************************************************************************
* Saves the class' contents to the given stream.
******************************************************************************/
void ColorCodingModifier::saveToStream(ObjectSaveStream& stream)
{
	ParticleModifier::saveToStream(stream);

	stream.beginChunk(0x02);
	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void ColorCodingModifier::loadFromStream(ObjectLoadStream& stream)
{
	ParticleModifier::loadFromStream(stream);

	int version = stream.expectChunkRange(0, 0x02);
	// This is for backward compatibility with old OVITO versions.
	if(version == 0x01) {
		ParticlePropertyReference pref;
		stream >> pref;
		setSourceParticleProperty(pref);
	}
	stream.closeChunk();
}

/******************************************************************************
* Parses the serialized contents of a property field in a custom way.
******************************************************************************/
bool ColorCodingModifier::loadPropertyFieldFromStream(ObjectLoadStream& stream, const ObjectLoadStream::SerializedPropertyField& serializedField)
{
	// This is for backward compatibility with OVITO 2.7.1.
	if(serializedField.identifier == "OperateOnBonds" && serializedField.definingClass == &ColorCodingModifier::OOType) {
		bool operateOnBonds;
		stream >> operateOnBonds;
		if(operateOnBonds)
			setColorApplicationMode(Bonds);
		return true;
	}

	return ParticleModifier::loadPropertyFieldFromStream(stream, serializedField);
}

/******************************************************************************
* Loads the given image file from disk.
******************************************************************************/
void ColorCodingImageGradient::loadImage(const QString& filename)
{
	QImage image(filename);
	if(image.isNull())
		throwException(tr("Could not load image file '%1'.").arg(filename));
	setImage(image);
}

/******************************************************************************
* Converts a scalar value to a color value.
******************************************************************************/
Color ColorCodingImageGradient::valueToColor(FloatType t)
{
	if(image().isNull()) return Color(0,0,0);
	QPoint p;
	if(image().width() > image().height())
		p = QPoint(std::min((int)(t * image().width()), image().width()-1), 0);
	else
		p = QPoint(0, std::min((int)(t * image().height()), image().height()-1));
	return Color(image().pixel(p));
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
