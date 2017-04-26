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
#include <plugins/particles/data/ParticleProperty.h>
#include "../../ParticleModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * \brief This modifier computes a scatter plot for two particle properties.
 */
class OVITO_PARTICLES_EXPORT ScatterPlotModifier : public ParticleModifier
{
public:

	/// Constructor.
	Q_INVOKABLE ScatterPlotModifier(DataSet* dataset);

	/// Returns the stored scatter plot data.
	const QVector<QPointF>& xyData() const { return _xyData; }

	/// Returns the stored point type data.
	const QVector<int>& typeData() const { return _typeData; }

	/// Returns the map from particle types to colors.
	const std::map<int, Color>& colorMap() const { return _colorMap; }

	/// Set start and end value of the x-axis.
	void setXAxisRange(FloatType start, FloatType end) { _xAxisRangeStart = start; _xAxisRangeEnd = end; }

	/// Set start and end value of the y-axis.
	void setYAxisRange(FloatType start, FloatType end) { _yAxisRangeStart = start; _yAxisRangeEnd = end; }

protected:

	/// Modifies the particle object.
	virtual PipelineStatus modifyParticles(TimePoint time, TimeInterval& validityInterval) override;

	/// This virtual method is called by the system when the modifier has been inserted into a PipelineObject.
	virtual void initializeModifier(PipelineObject* pipelineObject, ModifierApplication* modApp) override;

private:

	/// The particle type property that is used as source for the x-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(ParticlePropertyReference, xAxisProperty, setXAxisProperty);

	/// The particle type property that is used as source for the y-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(ParticlePropertyReference, yAxisProperty, setYAxisProperty);

	/// Controls the whether particles within the specified range should be selected (x-axis).
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, selectXAxisInRange, setSelectXAxisInRange);

	/// Controls the start value of the selection interval (x-axis).
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, selectionXAxisRangeStart, setSelectionXAxisRangeStart);

	/// Controls the end value of the selection interval (x-axis).
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, selectionXAxisRangeEnd, setSelectionXAxisRangeEnd);

	/// Controls the whether particles within the specified range should be selected (y-axis).
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, selectYAxisInRange, setSelectYAxisInRange);

	/// Controls the start value of the selection interval (y-axis).
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, selectionYAxisRangeStart, setSelectionYAxisRangeStart);

	/// Controls the end value of the selection interval (y-axis).
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, selectionYAxisRangeEnd, setSelectionYAxisRangeEnd);

	/// Controls the whether the range of the x-axis of the scatter plot should be fixed.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, fixXAxisRange, setFixXAxisRange);

	/// Controls the start value of the x-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, xAxisRangeStart, setXAxisRangeStart);

	/// Controls the end value of the x-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, xAxisRangeEnd, setXAxisRangeEnd);

	/// Controls the whether the range of the y-axis of the scatter plot should be fixed.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, fixYAxisRange, setFixYAxisRange);

	/// Controls the start value of the y-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, yAxisRangeStart, setYAxisRangeStart);

	/// Controls the end value of the y-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, yAxisRangeEnd, setYAxisRangeEnd);

	/// Stores the scatter plot data.
	QVector<QPointF> _xyData;

	/// Stores the point type data.
	QVector<int> _typeData;

	/// Maps particle types to colors.
	std::map<int, Color> _colorMap;

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Scatter plot");
	Q_CLASSINFO("ModifierCategory", "Analysis");
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


