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
#include <core/animation/AnimationSettings.h>
#include "HistogramModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(Particles, HistogramModifier, ParticleModifier);
DEFINE_FLAGS_PROPERTY_FIELD(HistogramModifier, _numberOfBins, "NumberOfBins", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(HistogramModifier, _selectInRange, "SelectInRange");
DEFINE_FLAGS_PROPERTY_FIELD(HistogramModifier, _selectionRangeStart, "SelectionRangeStart", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(HistogramModifier, _selectionRangeEnd, "SelectionRangeEnd", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(HistogramModifier, _fixXAxisRange, "FixXAxisRange");
DEFINE_FLAGS_PROPERTY_FIELD(HistogramModifier, _xAxisRangeStart, "XAxisRangeStart", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(HistogramModifier, _xAxisRangeEnd, "XAxisRangeEnd", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(HistogramModifier, _fixYAxisRange, "FixYAxisRange");
DEFINE_FLAGS_PROPERTY_FIELD(HistogramModifier, _yAxisRangeStart, "YAxisRangeStart", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(HistogramModifier, _yAxisRangeEnd, "YAxisRangeEnd", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(HistogramModifier, _sourceParticleProperty, "SourceProperty");
DEFINE_PROPERTY_FIELD(HistogramModifier, _sourceBondProperty, "SourceBondProperty");
DEFINE_PROPERTY_FIELD(HistogramModifier, _onlySelected, "OnlySelected");
DEFINE_PROPERTY_FIELD(HistogramModifier, _dataSourceType, "DataSourceType");
SET_PROPERTY_FIELD_LABEL(HistogramModifier, _numberOfBins, "Number of histogram bins");
SET_PROPERTY_FIELD_LABEL(HistogramModifier, _selectInRange, "Select value range");
SET_PROPERTY_FIELD_LABEL(HistogramModifier, _selectionRangeStart, "Selection range start");
SET_PROPERTY_FIELD_LABEL(HistogramModifier, _selectionRangeEnd, "Selection range end");
SET_PROPERTY_FIELD_LABEL(HistogramModifier, _fixXAxisRange, "Fix x-range");
SET_PROPERTY_FIELD_LABEL(HistogramModifier, _xAxisRangeStart, "X-range start");
SET_PROPERTY_FIELD_LABEL(HistogramModifier, _xAxisRangeEnd, "X-range end");
SET_PROPERTY_FIELD_LABEL(HistogramModifier, _fixYAxisRange, "Fix y-range");
SET_PROPERTY_FIELD_LABEL(HistogramModifier, _yAxisRangeStart, "Y-range start");
SET_PROPERTY_FIELD_LABEL(HistogramModifier, _yAxisRangeEnd, "Y-range end");
SET_PROPERTY_FIELD_LABEL(HistogramModifier, _sourceParticleProperty, "Source property");
SET_PROPERTY_FIELD_LABEL(HistogramModifier, _sourceBondProperty, "Source property");
SET_PROPERTY_FIELD_LABEL(HistogramModifier, _onlySelected, "Use only selected particles/bonds");
SET_PROPERTY_FIELD_LABEL(HistogramModifier, _dataSourceType, "Source type");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(HistogramModifier, _numberOfBins, IntegerParameterUnit, 1, 100000);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
HistogramModifier::HistogramModifier(DataSet* dataset) : ParticleModifier(dataset),
	_numberOfBins(200), _selectInRange(false),
	_selectionRangeStart(0), _selectionRangeEnd(1),
	_fixXAxisRange(false), _xAxisRangeStart(0), _xAxisRangeEnd(0),
	_fixYAxisRange(false), _yAxisRangeStart(0), _yAxisRangeEnd(0),
	_onlySelected(false), _dataSourceType(Particles)
{
	INIT_PROPERTY_FIELD(HistogramModifier::_numberOfBins);
	INIT_PROPERTY_FIELD(HistogramModifier::_selectInRange);
	INIT_PROPERTY_FIELD(HistogramModifier::_selectionRangeStart);
	INIT_PROPERTY_FIELD(HistogramModifier::_selectionRangeEnd);
	INIT_PROPERTY_FIELD(HistogramModifier::_fixXAxisRange);
	INIT_PROPERTY_FIELD(HistogramModifier::_xAxisRangeStart);
	INIT_PROPERTY_FIELD(HistogramModifier::_xAxisRangeEnd);
	INIT_PROPERTY_FIELD(HistogramModifier::_fixYAxisRange);
	INIT_PROPERTY_FIELD(HistogramModifier::_yAxisRangeStart);
	INIT_PROPERTY_FIELD(HistogramModifier::_yAxisRangeEnd);
	INIT_PROPERTY_FIELD(HistogramModifier::_sourceParticleProperty);
	INIT_PROPERTY_FIELD(HistogramModifier::_sourceBondProperty);
	INIT_PROPERTY_FIELD(HistogramModifier::_onlySelected);
	INIT_PROPERTY_FIELD(HistogramModifier::_dataSourceType);
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a pipeline.
******************************************************************************/
void HistogramModifier::initializeModifier(PipelineObject* pipeline, ModifierApplication* modApp)
{
	ParticleModifier::initializeModifier(pipeline, modApp);

	// Use the first available particle property from the input state as data source when the modifier is newly created.
	if(sourceParticleProperty().isNull()) {
		PipelineFlowState input = getModifierInput(modApp);
		ParticlePropertyReference bestProperty;
		for(DataObject* o : input.objects()) {
			ParticlePropertyObject* property = dynamic_object_cast<ParticlePropertyObject>(o);
			if(property && (property->dataType() == qMetaTypeId<int>() || property->dataType() == qMetaTypeId<FloatType>())) {
				bestProperty = ParticlePropertyReference(property, (property->componentCount() > 1) ? 0 : -1);
			}
		}
		if(!bestProperty.isNull()) {
			setSourceParticleProperty(bestProperty);
		}
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
}

/******************************************************************************
* This modifies the input object.
******************************************************************************/
PipelineStatus HistogramModifier::modifyParticles(TimePoint time, TimeInterval& validityInterval)
{
	_histogramData.resize(std::max(1, numberOfBins()));
	std::fill(_histogramData.begin(), _histogramData.end(), 0);

	// Get the source property.
	PropertyBase* property;
	PropertyBase* inputSelection = nullptr;
	PropertyBase* outputSelection = nullptr;
	ParticlePropertyObject* outputParticleSelectionObj = nullptr;
	BondPropertyObject* outputBondSelectionObj = nullptr;
	size_t vecComponent;
	if(dataSourceType() == Particles) {
		if(sourceParticleProperty().isNull())
			throwException(tr("Select a particle property first."));
		ParticlePropertyObject* propertyObj = sourceParticleProperty().findInState(input());
		if(!propertyObj)
			throwException(tr("The selected particle property with the name '%1' does not exist.").arg(sourceParticleProperty().name()));
		if(sourceParticleProperty().vectorComponent() >= (int)propertyObj->componentCount())
			throwException(tr("The selected vector component is out of range. The particle property '%1' contains only %2 values per particle.").arg(sourceParticleProperty().name()).arg(propertyObj->componentCount()));
		property = propertyObj->storage();
		vecComponent = std::max(0, sourceParticleProperty().vectorComponent());

		// Get the particle selection property if enabled by the user.
		if(onlySelected()) {
			ParticlePropertyObject* selPropertyObj = expectStandardProperty(ParticleProperty::SelectionProperty);
			OVITO_ASSERT(selPropertyObj->size() == property->size());
			inputSelection = selPropertyObj->storage();
		}

		// Create selection property for output.
		if(selectInRange()) {
			outputParticleSelectionObj = outputStandardProperty(ParticleProperty::SelectionProperty, true);
			outputSelection = outputParticleSelectionObj->modifiableStorage();
		}
	}
	else {
		if(sourceBondProperty().isNull())
			throwException(tr("Select a bond property first."));
		BondPropertyObject* propertyObj = sourceBondProperty().findInState(input());
		if(!propertyObj)
			throwException(tr("The selected bond property with the name '%1' does not exist.").arg(sourceBondProperty().name()));
		if(sourceBondProperty().vectorComponent() >= (int)propertyObj->componentCount())
			throwException(tr("The selected vector component is out of range. The bond property '%1' contains only %2 values per bond.").arg(sourceBondProperty().name()).arg(propertyObj->componentCount()));
		property = propertyObj->storage();
		vecComponent = std::max(0, sourceBondProperty().vectorComponent());

		// Get the bond selection property if enabled by the user.
		if(onlySelected()) {
			BondPropertyObject* selPropertyObj = expectStandardBondProperty(BondProperty::SelectionProperty);
			OVITO_ASSERT(selPropertyObj->size() == property->size());
			inputSelection = selPropertyObj->storage();
		}

		// Create selection property for output.
		if(selectInRange()) {
			outputBondSelectionObj = outputStandardBondProperty(BondProperty::SelectionProperty, true);
			outputSelection = outputBondSelectionObj->modifiableStorage();
		}		
	}
	size_t vecComponentCount = property->componentCount();

	// Create selection property for output.
	FloatType selectionRangeStart = _selectionRangeStart;
	FloatType selectionRangeEnd = _selectionRangeEnd;
	if(selectionRangeStart > selectionRangeEnd) std::swap(selectionRangeStart, selectionRangeEnd);
	size_t numSelected = 0;

	double intervalStart = _xAxisRangeStart;
	double intervalEnd = _xAxisRangeEnd;

	if(property->size() > 0) {
		if(property->dataType() == qMetaTypeId<FloatType>()) {
			const FloatType* v_begin = property->constDataFloat() + vecComponent;
			const FloatType* v_end = v_begin + (property->size() * vecComponentCount);
			// Determine value range.
			if(!fixXAxisRange()) {
				intervalStart = std::numeric_limits<double>::max();
				intervalEnd = std::numeric_limits<double>::lowest();
				const int* sel = inputSelection ? inputSelection->constDataInt() : nullptr;
				for(auto v = v_begin; v != v_end; v += vecComponentCount) {
					if(sel && !*sel++) continue;
					if(*v < intervalStart) intervalStart = *v;
					if(*v > intervalEnd) intervalEnd = *v;
				}
			}
			// Perform binning.
			if(intervalEnd > intervalStart) {
				FloatType binSize = (intervalEnd - intervalStart) / _histogramData.size();
				const int* sel = inputSelection ? inputSelection->constDataInt() : nullptr;
				for(auto v = v_begin; v != v_end; v += vecComponentCount) {
					if(sel && !*sel++) continue;
					if(*v < intervalStart || *v > intervalEnd) continue;
					int binIndex = (*v - intervalStart) / binSize;
					_histogramData[std::max(0, std::min(binIndex, _histogramData.size() - 1))]++;
				}
			}
			else {
				if(!inputSelection)
					_histogramData[0] = property->size();
				else
					_histogramData[0] = property->size() - std::count(inputSelection->constDataInt(), inputSelection->constDataInt() + inputSelection->size(), 0);
			}
			if(outputSelection) {
				OVITO_ASSERT(outputSelection->size() == property->size());
				int* s = outputSelection->dataInt();
				int* s_end = s + outputSelection->size();
				const int* sel = inputSelection ? inputSelection->constDataInt() : nullptr;
				for(auto v = v_begin; v != v_end; v += vecComponentCount, ++s) {
					if((!sel || *sel++) && *v >= selectionRangeStart && *v <= selectionRangeEnd) {
						*s = 1;
						numSelected++;
					}
					else *s = 0;
				}
			}
		}
		else if(property->dataType() == qMetaTypeId<int>()) {
			const int* v_begin = property->constDataInt() + vecComponent;
			const int* v_end = v_begin + (property->size() * vecComponentCount);
			// Determine value range.
			if(!fixXAxisRange()) {
				intervalStart = std::numeric_limits<double>::max();
				intervalEnd = std::numeric_limits<double>::lowest();
				const int* sel = inputSelection ? inputSelection->constDataInt() : nullptr;
				for(auto v = v_begin; v != v_end; v += vecComponentCount) {
					if(sel && !*sel++) continue;
					if(*v < intervalStart) intervalStart = *v;
					if(*v > intervalEnd) intervalEnd = *v;
				}
			}
			// Perform binning.
			if(intervalEnd > intervalStart) {
				FloatType binSize = (intervalEnd - intervalStart) / _histogramData.size();
				const int* sel = inputSelection ? inputSelection->constDataInt() : nullptr;
				for(auto v = v_begin; v != v_end; v += vecComponentCount) {
					if(sel && !*sel++) continue;
					if(*v < intervalStart || *v > intervalEnd) continue;
					int binIndex = ((FloatType)*v - intervalStart) / binSize;
					_histogramData[std::max(0, std::min(binIndex, _histogramData.size() - 1))]++;
				}
			}
			else {
				if(!inputSelection)
					_histogramData[0] = property->size();
				else
					_histogramData[0] = property->size() - std::count(inputSelection->constDataInt(), inputSelection->constDataInt() + inputSelection->size(), 0);
			}
			if(outputSelection) {
				OVITO_ASSERT(outputSelection->size() == property->size());
				int* s = outputSelection->dataInt();
				int* s_end = s + outputSelection->size();
				const int* sel = inputSelection ? inputSelection->constDataInt() : nullptr;
				for(auto v = v_begin; v != v_end; v += vecComponentCount, ++s) {
					if((!sel || *sel++) && *v >= selectionRangeStart && *v <= selectionRangeEnd) {
						*s = 1;
						numSelected++;
					}
					else *s = 0;
				}
			}
		}
	}
	else {
		intervalStart = intervalEnd = 0;
	}

	QString statusMessage;
	if(outputParticleSelectionObj) {
		outputParticleSelectionObj->changed();
		statusMessage += tr("%1 particles selected (%2%)").arg(numSelected).arg((FloatType)numSelected * 100 / std::max(1,(int)outputParticleSelectionObj->size()), 0, 'f', 1);
	}
	else if(outputBondSelectionObj) {
		outputBondSelectionObj->changed();
		statusMessage += tr("%1 bonds selected (%2%)").arg(numSelected).arg((FloatType)numSelected * 100 / std::max(1,(int)outputBondSelectionObj->size()), 0, 'f', 1);
	}

	_xAxisRangeStart = intervalStart;
	_xAxisRangeEnd = intervalEnd;

	if(!fixYAxisRange()) {
		_yAxisRangeStart = 0.0;
		_yAxisRangeEnd = *std::max_element(_histogramData.begin(), _histogramData.end());
	}

	// Inform the editor component that the stored histogram data has changed
	// and it should update the display.
	notifyDependents(ReferenceEvent::ObjectStatusChanged);

	return PipelineStatus(PipelineStatus::Success, statusMessage);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
