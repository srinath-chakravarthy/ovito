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

#ifndef __OVITO_HISTOGRAM_MODIFIER_H
#define __OVITO_HISTOGRAM_MODIFIER_H

#include <plugins/particles/Particles.h>
#include <plugins/particles/data/ParticleProperty.h>
#include <plugins/particles/objects/ParticlePropertyObject.h>
#include "../../ParticleModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * \brief This modifier computes a value histogram for a particle property.
 */
class OVITO_PARTICLES_EXPORT HistogramModifier : public ParticleModifier
{
public:

	/// Constructor.
	Q_INVOKABLE HistogramModifier(DataSet* dataset);

	/// Sets the source particle property for which the histogram should be computed.
	void setSourceProperty(const ParticlePropertyReference& prop) { _sourceProperty = prop; }

	/// Returns the source particle property for which the histogram is computed.
	const ParticlePropertyReference& sourceProperty() const { return _sourceProperty; }

	/// Returns the number of bins in the computed histogram.
	int numberOfBins() const { return _numberOfBins; }

	/// Sets the number of bins in the computed histogram.
	void setNumberOfBins(int n) { _numberOfBins = n; }

	/// Returns the stored histogram data.
	const QVector<int>& histogramData() const { return _histogramData; }

	/// Returns whether particles within the specified range should be selected.
	bool selectInRange() const { return _selectInRange; }

	/// Sets whether particles within the specified range should be selected.
	void setSelectInRange(bool select) { _selectInRange = select; }

	/// Returns the start value of the selection interval.
	FloatType selectionRangeStart() const { return _selectionRangeStart; }

	/// Returns the end value of the selection interval.
	FloatType selectionRangeEnd() const { return _selectionRangeEnd; }

	/// Set whether the range of the x-axis of the scatter plot should be fixed.
	void setFixXAxisRange(bool fix) { _fixXAxisRange = fix; }

	/// Returns whether the range of the x-axis of the histogram should be fixed.
	bool fixXAxisRange() const { return _fixXAxisRange; }

	/// Set start and end value of the x-axis.
	void setXAxisRange(FloatType start, FloatType end) { _xAxisRangeStart = start; _xAxisRangeEnd = end; }

	/// Set start value of the x-axis.
	void setXAxisRangeStart(FloatType start) { _xAxisRangeStart = start; }

	/// Set end value of the x-axis.
	void setXAxisRangeEnd(FloatType end) { _xAxisRangeEnd = end; }

	/// Returns the start value of the x-axis.
	FloatType xAxisRangeStart() const { return _xAxisRangeStart; }

	/// Returns the end value of the x-axis.
	FloatType xAxisRangeEnd() const { return _xAxisRangeEnd; }

	/// Returns whether the range of the y-axis of the histogram should be fixed.
	bool fixYAxisRange() const { return _fixYAxisRange; }

	/// Returns the start value of the y-axis.
	FloatType yAxisRangeStart() const { return _yAxisRangeStart; }

	/// Returns the end value of the y-axis.
	FloatType yAxisRangeEnd() const { return _yAxisRangeEnd; }

	/// Returns whether analysis takes only selected particles into account.
	bool onlySelected() const { return _onlySelected; }

	/// Sets whether analysis only selected particles are taken into account.
	void setOnlySelected(bool onlySelected) { _onlySelected = onlySelected; }

protected:

	/// Modifies the particle object.
	virtual PipelineStatus modifyParticles(TimePoint time, TimeInterval& validityInterval) override;

	/// This virtual method is called by the system when the modifier has been inserted into a PipelineObject.
	virtual void initializeModifier(PipelineObject* pipelineObject, ModifierApplication* modApp) override;

private:

	/// The particle property that serves as data source of the histogram.
	PropertyField<ParticlePropertyReference> _sourceProperty;

	/// Controls the number of histogram bins.
	PropertyField<int> _numberOfBins;

	/// Controls the whether particles within the specified range should be selected.
	PropertyField<bool> _selectInRange;

	/// Controls the start value of the selection interval.
	PropertyField<FloatType> _selectionRangeStart;

	/// Controls the end value of the selection interval.
	PropertyField<FloatType> _selectionRangeEnd;

	/// Controls the whether the range of the x-axis of the histogram should be fixed.
	PropertyField<bool> _fixXAxisRange;

	/// Controls the start value of the x-axis.
	PropertyField<FloatType> _xAxisRangeStart;

	/// Controls the end value of the x-axis.
	PropertyField<FloatType> _xAxisRangeEnd;

	/// Controls the whether the range of the y-axis of the histogram should be fixed.
	PropertyField<bool> _fixYAxisRange;

	/// Controls the start value of the y-axis.
	PropertyField<FloatType> _yAxisRangeStart;

	/// Controls the end value of the y-axis.
	PropertyField<FloatType> _yAxisRangeEnd;

	/// Controls whether the modifier should take into account only selected particles.
	PropertyField<bool> _onlySelected;

	/// Stores the histogram data.
	QVector<int> _histogramData;

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Histogram");
	Q_CLASSINFO("ModifierCategory", "Analysis");

	DECLARE_PROPERTY_FIELD(_numberOfBins);
	DECLARE_PROPERTY_FIELD(_selectInRange);
	DECLARE_PROPERTY_FIELD(_selectionRangeStart);
	DECLARE_PROPERTY_FIELD(_selectionRangeEnd);
	DECLARE_PROPERTY_FIELD(_fixXAxisRange);
	DECLARE_PROPERTY_FIELD(_xAxisRangeStart);
	DECLARE_PROPERTY_FIELD(_xAxisRangeEnd);
	DECLARE_PROPERTY_FIELD(_fixYAxisRange);
	DECLARE_PROPERTY_FIELD(_yAxisRangeStart);
	DECLARE_PROPERTY_FIELD(_yAxisRangeEnd);
	DECLARE_PROPERTY_FIELD(_sourceProperty);
	DECLARE_PROPERTY_FIELD(_onlySelected);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace

#endif // __OVITO_HISTOGRAM_MODIFIER_H
