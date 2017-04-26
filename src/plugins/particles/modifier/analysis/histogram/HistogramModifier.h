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
#include <plugins/particles/objects/ParticlePropertyObject.h>
#include <plugins/particles/objects/BondPropertyObject.h>
#include "../../ParticleModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * \brief This modifier computes a value histogram for a particle property.
 */
class OVITO_PARTICLES_EXPORT HistogramModifier : public ParticleModifier
{
public:

	/// The data sources supported by the histogram modifier.
	enum DataSourceType {
		Particles,	//< Particle property values
		Bonds,		//< Bond property values
	};
	Q_ENUMS(DataSourceType);

public:

	/// Constructor.
	Q_INVOKABLE HistogramModifier(DataSet* dataset);

	/// Returns the stored histogram data.
	const QVector<int>& histogramData() const { return _histogramData; }

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

	/// The particle property that serves as data source of the histogram.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(ParticlePropertyReference, sourceParticleProperty, setSourceParticleProperty);

	/// The bond property that serves as data source of the histogram.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(BondPropertyReference, sourceBondProperty, setSourceBondProperty);

	/// Controls the number of histogram bins.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, numberOfBins, setNumberOfBins);

	/// Controls the whether particles within the specified range should be selected.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, selectInRange, setSelectInRange);

	/// Controls the start value of the selection interval.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, selectionRangeStart, setSelectionRangeStart);

	/// Controls the end value of the selection interval.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, selectionRangeEnd, setSelectionRangeEnd);

	/// Controls the whether the range of the x-axis of the histogram should be fixed.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, fixXAxisRange, setFixXAxisRange);

	/// Controls the start value of the x-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, xAxisRangeStart, setXAxisRangeStart);

	/// Controls the end value of the x-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, xAxisRangeEnd, setXAxisRangeEnd);

	/// Controls the whether the range of the y-axis of the histogram should be fixed.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, fixYAxisRange, setFixYAxisRange);

	/// Controls the start value of the y-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, yAxisRangeStart, setYAxisRangeStart);

	/// Controls the end value of the y-axis.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, yAxisRangeEnd, setYAxisRangeEnd);

	/// Controls whether the modifier should take into account only selected particles.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, onlySelected, setOnlySelected);

	/// Controls where this modifier takes its input values from.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(DataSourceType, dataSourceType, setDataSourceType);

	/// Stores the histogram data.
	QVector<int> _histogramData;

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Histogram");
	Q_CLASSINFO("ModifierCategory", "Analysis");
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


