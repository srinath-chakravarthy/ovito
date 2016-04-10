///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
//  Copyright (2014) Lars Pastewka
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
#include <plugins/particles/objects/ParticleTypeProperty.h>
#include <core/scene/pipeline/PipelineObject.h>
#include <core/animation/AnimationSettings.h>
#include "ScatterPlotModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(Particles, ScatterPlotModifier, ParticleModifier);
DEFINE_PROPERTY_FIELD(ScatterPlotModifier, _selectXAxisInRange, "SelectXAxisInRange");
DEFINE_FLAGS_PROPERTY_FIELD(ScatterPlotModifier, _selectionXAxisRangeStart, "SelectionXAxisRangeStart", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(ScatterPlotModifier, _selectionXAxisRangeEnd, "SelectionXAxisRangeEnd", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(ScatterPlotModifier, _selectYAxisInRange, "SelectYAxisInRange");
DEFINE_FLAGS_PROPERTY_FIELD(ScatterPlotModifier, _selectionYAxisRangeStart, "SelectionYAxisRangeStart", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(ScatterPlotModifier, _selectionYAxisRangeEnd, "SelectionYAxisRangeEnd", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(ScatterPlotModifier, _fixXAxisRange, "FixXAxisRange");
DEFINE_FLAGS_PROPERTY_FIELD(ScatterPlotModifier, _xAxisRangeStart, "XAxisRangeStart", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(ScatterPlotModifier, _xAxisRangeEnd, "XAxisRangeEnd", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(ScatterPlotModifier, _fixYAxisRange, "FixYAxisRange");
DEFINE_FLAGS_PROPERTY_FIELD(ScatterPlotModifier, _yAxisRangeStart, "YAxisRangeStart", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(ScatterPlotModifier, _yAxisRangeEnd, "YAxisRangeEnd", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(ScatterPlotModifier, _xAxisProperty, "XAxisProperty");
DEFINE_PROPERTY_FIELD(ScatterPlotModifier, _yAxisProperty, "YAxisProperty");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, _selectXAxisInRange, "Select particles in x-range");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, _selectionXAxisRangeStart, "Selection x-range start");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, _selectionXAxisRangeEnd, "Selection x-range end");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, _selectYAxisInRange, "Select particles in y-range");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, _selectionYAxisRangeStart, "Selection y-range start");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, _selectionYAxisRangeEnd, "Selection y-range end");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, _fixXAxisRange, "Fix x-axis range");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, _xAxisRangeStart, "X-axis range start");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, _xAxisRangeEnd, "X-axis range end");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, _fixYAxisRange, "Fix y-axis range");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, _yAxisRangeStart, "Y-axis range start");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, _yAxisRangeEnd, "Y-axis range end");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, _xAxisProperty, "X-axis property");
SET_PROPERTY_FIELD_LABEL(ScatterPlotModifier, _yAxisProperty, "Y-axis property");

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
ScatterPlotModifier::ScatterPlotModifier(DataSet* dataset) : ParticleModifier(dataset),
	_selectXAxisInRange(false),	_selectionXAxisRangeStart(0), _selectionXAxisRangeEnd(1),
	_selectYAxisInRange(false),	_selectionYAxisRangeStart(0), _selectionYAxisRangeEnd(1),
	_fixXAxisRange(false), _xAxisRangeStart(0),	_xAxisRangeEnd(0), _fixYAxisRange(false),
	_yAxisRangeStart(0), _yAxisRangeEnd(0)
{
	INIT_PROPERTY_FIELD(ScatterPlotModifier::_selectXAxisInRange);
	INIT_PROPERTY_FIELD(ScatterPlotModifier::_selectionXAxisRangeStart);
	INIT_PROPERTY_FIELD(ScatterPlotModifier::_selectionXAxisRangeEnd);
	INIT_PROPERTY_FIELD(ScatterPlotModifier::_selectYAxisInRange);
	INIT_PROPERTY_FIELD(ScatterPlotModifier::_selectionYAxisRangeStart);
	INIT_PROPERTY_FIELD(ScatterPlotModifier::_selectionYAxisRangeEnd);
	INIT_PROPERTY_FIELD(ScatterPlotModifier::_fixXAxisRange);
	INIT_PROPERTY_FIELD(ScatterPlotModifier::_xAxisRangeStart);
	INIT_PROPERTY_FIELD(ScatterPlotModifier::_xAxisRangeEnd);
	INIT_PROPERTY_FIELD(ScatterPlotModifier::_fixYAxisRange);
	INIT_PROPERTY_FIELD(ScatterPlotModifier::_yAxisRangeStart);
	INIT_PROPERTY_FIELD(ScatterPlotModifier::_yAxisRangeEnd);
	INIT_PROPERTY_FIELD(ScatterPlotModifier::_xAxisProperty);
	INIT_PROPERTY_FIELD(ScatterPlotModifier::_yAxisProperty);
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a pipeline.
******************************************************************************/
void ScatterPlotModifier::initializeModifier(PipelineObject* pipeline, ModifierApplication* modApp)
{
	ParticleModifier::initializeModifier(pipeline, modApp);

	ParticlePropertyReference bestProperty;
	if(xAxisProperty().isNull() || yAxisProperty().isNull()) {
		// Select the first available particle property from the input state.
		PipelineFlowState input = pipeline->evaluatePipeline(dataset()->animationSettings()->time(), modApp, false);
		for(DataObject* o : input.objects()) {
			ParticlePropertyObject* property = dynamic_object_cast<ParticlePropertyObject>(o);
			if(property && (property->dataType() == qMetaTypeId<int>() || property->dataType() == qMetaTypeId<FloatType>())) {
				bestProperty = ParticlePropertyReference(property, (property->componentCount() > 1) ? 0 : -1);
			}
		}
	}
	if(xAxisProperty().isNull() && !bestProperty.isNull()) {
		setXAxisProperty(bestProperty);
	}
	if(yAxisProperty().isNull() && !bestProperty.isNull()) {
		setYAxisProperty(bestProperty);
	}
}

/******************************************************************************
* This modifies the input object.
******************************************************************************/
PipelineStatus ScatterPlotModifier::modifyParticles(TimePoint time, TimeInterval& validityInterval)
{
	// Get the source property.
	if(xAxisProperty().isNull())
		throwException(tr("Select a particle property first."));
	ParticlePropertyObject* xProperty = xAxisProperty().findInState(input());
	ParticlePropertyObject* yProperty = yAxisProperty().findInState(input());
	if(!xProperty)
		throwException(tr("The selected particle property with the name '%1' does not exist.").arg(xAxisProperty().name()));
	if(!yProperty)
		throwException(tr("The selected particle property with the name '%1' does not exist.").arg(yAxisProperty().name()));
	if(xAxisProperty().vectorComponent() >= (int)xProperty->componentCount())
		throwException(tr("The selected vector component is out of range. The particle property '%1' contains only %2 values per particle.").arg(xAxisProperty().name()).arg(xProperty->componentCount()));
	if(yAxisProperty().vectorComponent() >= (int)yProperty->componentCount())
		throwException(tr("The selected vector component is out of range. The particle property '%1' contains only %2 values per particle.").arg(yAxisProperty().name()).arg(yProperty->componentCount()));

	size_t xVecComponent = std::max(0, xAxisProperty().vectorComponent());
	size_t xVecComponentCount = xProperty->componentCount();
	size_t yVecComponent = std::max(0, yAxisProperty().vectorComponent());
	size_t yVecComponentCount = yProperty->componentCount();

	int numParticleTypes = 0;
	ParticleTypeProperty* typeProperty = static_object_cast<ParticleTypeProperty>(inputStandardProperty(ParticleProperty::ParticleTypeProperty));
	if(typeProperty) {
		_colorMap = typeProperty->colorMap();

		for(const ParticleType *type : typeProperty->particleTypes()) {
			numParticleTypes = std::max(numParticleTypes, type->id());
		}
		numParticleTypes++;
	}
	else {
		_colorMap.clear();
		numParticleTypes = 1;
	}

	ParticlePropertyObject* selProperty = nullptr;
	FloatType selectionXAxisRangeStart = _selectionXAxisRangeStart;
	FloatType selectionXAxisRangeEnd = _selectionXAxisRangeEnd;
	FloatType selectionYAxisRangeStart = _selectionYAxisRangeStart;
	FloatType selectionYAxisRangeEnd = _selectionYAxisRangeEnd;
	size_t numSelected = 0;
	if(_selectXAxisInRange || _selectYAxisInRange) {
		selProperty = outputStandardProperty(ParticleProperty::SelectionProperty, true);
		int* s_begin = selProperty->dataInt();
		int* s_end = s_begin + selProperty->size();
		for(auto s = s_begin; s != s_end; ++s) {
			*s = 1;
			numSelected++;
		}
	}
	if(_selectXAxisInRange) {
		if(selectionXAxisRangeStart > selectionXAxisRangeEnd)
			std::swap(selectionXAxisRangeStart, selectionXAxisRangeEnd);
	}
	if(_selectYAxisInRange) {
		if(selectionYAxisRangeStart > selectionYAxisRangeEnd)
			std::swap(selectionYAxisRangeStart, selectionYAxisRangeEnd);
	}

	double xIntervalStart = _xAxisRangeStart;
	double xIntervalEnd = _xAxisRangeEnd;
	double yIntervalStart = _yAxisRangeStart;
	double yIntervalEnd = _yAxisRangeEnd;

	_xData.clear();
	_yData.clear();
	_xData.resize(numParticleTypes);
	_yData.resize(numParticleTypes);

	if(xProperty->size() > 0) {
		if(xProperty->dataType() == qMetaTypeId<FloatType>()) {
			const FloatType* vx_begin = xProperty->constDataFloat() + xVecComponent;
			const FloatType* vx_end = vx_begin + (xProperty->size() * xVecComponentCount);
			if (!_fixXAxisRange) {
				xIntervalStart = xIntervalEnd = *vx_begin;
				for(auto vx = vx_begin; vx != vx_end; vx += xVecComponentCount) {
					if(*vx < xIntervalStart) xIntervalStart = *vx;
					if(*vx > xIntervalEnd) xIntervalEnd = *vx;
				}
			}
			if(xIntervalEnd != xIntervalStart) {
				if(typeProperty) {
					const int *particleTypeId = typeProperty->constDataInt();
					for(auto vx = vx_begin; vx != vx_end; vx += xVecComponentCount, particleTypeId++) {
						_xData[*particleTypeId].append(*vx);
					}
				}
				else {
					for(auto vx = vx_begin; vx != vx_end; vx += xVecComponentCount) {
						_xData[0].append(*vx);
					}
				}
			}

			if(selProperty && _selectXAxisInRange) {
				OVITO_ASSERT(selProperty->size() == xProperty->size());
				int* s = selProperty->dataInt();
				int* s_end = s + selProperty->size();
				for(auto vx = vx_begin; vx != vx_end; vx += xVecComponentCount, ++s) {
					if(*vx < selectionXAxisRangeStart || *vx > selectionXAxisRangeEnd) {
						*s = 0;
						numSelected--;
					}
				}
			}
		}
		else if(xProperty->dataType() == qMetaTypeId<int>()) {
			const int* vx_begin = xProperty->constDataInt() + xVecComponent;
			const int* vx_end = vx_begin + (xProperty->size() * xVecComponentCount);
			if (!_fixXAxisRange) {
				xIntervalStart = xIntervalEnd = *vx_begin;
				for(auto vx = vx_begin; vx != vx_end; vx += xVecComponentCount) {
					if(*vx < xIntervalStart) xIntervalStart = *vx;
					if(*vx > xIntervalEnd) xIntervalEnd = *vx;
				}
			}
			if(xIntervalEnd != xIntervalStart) {
				if(typeProperty) {
					const int *particleTypeId = typeProperty->constDataInt();
					for(auto vx = vx_begin; vx != vx_end; vx += xVecComponentCount, particleTypeId++) {
						_xData[*particleTypeId].append(*vx);
					}
				}
				else {
					for(auto vx = vx_begin; vx != vx_end; vx += xVecComponentCount) {
						_xData[0].append(*vx);
					}
				}
			}

			if(selProperty && _selectXAxisInRange) {
				OVITO_ASSERT(selProperty->size() == xProperty->size());
				int* s = selProperty->dataInt();
				int* s_end = s + selProperty->size();
				for(auto vx = vx_begin; vx != vx_end; vx += xVecComponentCount, ++s) {
					if(*vx < selectionXAxisRangeStart || *vx > selectionXAxisRangeEnd) {
						*s = 0;
						numSelected--;
					}
				}
			}
		}
	}
	if (yProperty->size() > 0) {
		if(yProperty->dataType() == qMetaTypeId<FloatType>()) {
			const FloatType* vy_begin = yProperty->constDataFloat() + yVecComponent;
			const FloatType* vy_end = vy_begin + (yProperty->size() * yVecComponentCount);
			if (!_fixYAxisRange) {
				yIntervalStart = yIntervalEnd = *vy_begin;
				for(auto vy = vy_begin; vy != vy_end; vy += yVecComponentCount) {
					if(*vy < yIntervalStart) yIntervalStart = *vy;
					if(*vy > yIntervalEnd) yIntervalEnd = *vy;
				}
			}
			if(yIntervalEnd != yIntervalStart) {
				if(typeProperty) {
					const int *particleTypeId = typeProperty->constDataInt();
					for(auto vy = vy_begin; vy != vy_end; vy += yVecComponentCount, particleTypeId++) {
						_yData[*particleTypeId].append(*vy);
					}
				}
				else {
					for(auto vy = vy_begin; vy != vy_end; vy += yVecComponentCount) {
						_yData[0].append(*vy);
					}
				}
			}

			if(selProperty && _selectYAxisInRange) {
				OVITO_ASSERT(selProperty->size() == yProperty->size());
				int* s = selProperty->dataInt();
				int* s_end = s + selProperty->size();
				for(auto vy = vy_begin; vy != vy_end; vy += yVecComponentCount, ++s) {
					if(*vy < selectionYAxisRangeStart || *vy > selectionYAxisRangeEnd) {
						if(*s) {
							*s = 0;
							numSelected--;
						}
					}
				}
			}
		}
		else if(yProperty->dataType() == qMetaTypeId<int>()) {
			const int* vy_begin = yProperty->constDataInt() + yVecComponent;
			const int* vy_end = vy_begin + (yProperty->size() * yVecComponentCount);
			if (!_fixYAxisRange) {
				yIntervalStart = yIntervalEnd = *vy_begin;
				for(auto vy = vy_begin; vy != vy_end; vy += yVecComponentCount) {
					if(*vy < yIntervalStart) yIntervalStart = *vy;
					if(*vy > yIntervalEnd) yIntervalEnd = *vy;
				}
			}
			if(yIntervalEnd != yIntervalStart) {
				if(typeProperty) {
					const int *particleTypeId = typeProperty->constDataInt();
					for(auto vy = vy_begin; vy != vy_end; vy += yVecComponentCount, particleTypeId++) {
						_yData[*particleTypeId].append(*vy);
					}
				}
				else {
					for(auto vy = vy_begin; vy != vy_end; vy += yVecComponentCount) {
						_yData[0].append(*vy);
					}
				}
			}

			if(selProperty && _selectYAxisInRange) {
				OVITO_ASSERT(selProperty->size() == yProperty->size());
				int* s = selProperty->dataInt();
				int* s_end = s + selProperty->size();
				for(auto vy = vy_begin; vy != vy_end; vy += yVecComponentCount, ++s) {
					if(*vy < selectionYAxisRangeStart || *vy > selectionYAxisRangeEnd) {
						if(*s) {
							*s = 0;
							numSelected--;
						}
					}
				}
			}
		}
	}
	else {
		xIntervalStart = xIntervalEnd = 0;
		yIntervalStart = yIntervalEnd = 0;
	}

	QString statusMessage;
	if(selProperty) {
		selProperty->changed();
		statusMessage += tr("%1 particles selected (%2%)").arg(numSelected).arg((FloatType)numSelected * 100 / std::max(1,(int)selProperty->size()), 0, 'f', 1);
	}

	_xAxisRangeStart = xIntervalStart;
	_xAxisRangeEnd = xIntervalEnd;
	_yAxisRangeStart = yIntervalStart;
	_yAxisRangeEnd = yIntervalEnd;

	notifyDependents(ReferenceEvent::ObjectStatusChanged);

	return PipelineStatus(PipelineStatus::Success, statusMessage);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
