///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2014) Alexander Stukowski
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

#ifndef __OVITO_BIN_AND_REDUCE_MODIFIER_H
#define __OVITO_BIN_AND_REDUCE_MODIFIER_H

#include <plugins/particles/Particles.h>
#include <plugins/particles/data/ParticleProperty.h>
#include <plugins/particles/objects/ParticlePropertyObject.h>
#include "../../ParticleModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * \brief This modifier computes a spatial average (over splices) for a particle property.
 */
class OVITO_PARTICLES_EXPORT BinAndReduceModifier : public ParticleModifier
{
public:

    enum ReductionOperationType { RED_MEAN, RED_SUM, RED_SUM_VOL, RED_MIN, RED_MAX };
    Q_ENUMS(ReductionOperationType);
    enum BinDirectionType { CELL_VECTOR_1 = 0, CELL_VECTOR_2 = 1, CELL_VECTOR_3 = 2,
                            CELL_VECTORS_1_2 = 0+(1<<2), CELL_VECTORS_1_3 = 0+(2<<2), CELL_VECTORS_2_3 = 1+(2<<2) };
    Q_ENUMS(BinDirectionType);

	/// Constructor.
	Q_INVOKABLE BinAndReduceModifier(DataSet* dataset);

	/// Sets the source particle property for which the average should be computed.
	void setSourceProperty(const ParticlePropertyReference& prop) { _sourceProperty = prop; }

	/// Returns the source particle property for which the average is computed.
	const ParticlePropertyReference& sourceProperty() const { return _sourceProperty; }

	/// Returns the reduction operation
	ReductionOperationType reductionOperation() const { return _reductionOperation; }

	/// Sets the reduction operation
	void setReductionOperation(ReductionOperationType o) { _reductionOperation = o; }

	/// Returns compute first derivative
	bool firstDerivative() const { return _firstDerivative; }

	/// Sets compute first derivative
	void setFirstDerivative(bool d) { _firstDerivative = d; }

	/// Returns the bin direction
	BinDirectionType binDirection() const { return _binDirection; }

	/// Sets the bin direction
	void setBinDirection(BinDirectionType o) { _binDirection = o; }

	/// Returns the number of spatial bins of the computed average value.
	int numberOfBinsX() const { return _numberOfBinsX; }

	/// Sets the number of spatial bins of the computed average value.
	void setNumberOfBinsX(int n) { _numberOfBinsX = n; }

	/// Returns the number of spatial bins of the computed average value.
	int numberOfBinsY() const { return _numberOfBinsY; }

	/// Sets the number of spatial bins of the computed average value.
	void setNumberOfBinsY(int n) { _numberOfBinsY = n; }

	/// Returns the stored average data.
	const QVector<double>& binData() const { return _binData; }

	/// Returns the start value of the plotting x-axis.
	FloatType xAxisRangeStart() const { return _xAxisRangeStart; }

	/// Returns the end value of the plotting x-axis.
	FloatType xAxisRangeEnd() const { return _xAxisRangeEnd; }

	/// Returns the start value of the plotting y-axis.
	FloatType yAxisRangeStart() const { return _yAxisRangeStart; }

	/// Returns the end value of the plotting y-axis.
	FloatType yAxisRangeEnd() const { return _yAxisRangeEnd; }

	/// Set whether the plotting range of the property axis should be fixed.
	void setFixPropertyAxisRange(bool fix) { _fixPropertyAxisRange = fix; }

	/// Returns whether the plotting range of the property axis should be fixed.
	bool fixPropertyAxisRange() const { return _fixPropertyAxisRange; }

	/// Set start and end value of the plotting property axis.
	void setPropertyAxisRange(FloatType start, FloatType end) { _propertyAxisRangeStart = start; _propertyAxisRangeEnd = end; }

	/// Returns the start value of the plotting y-axis.
	FloatType propertyAxisRangeStart() const { return _propertyAxisRangeStart; }

	/// Returns the end value of the plotting y-axis.
	FloatType propertyAxisRangeEnd() const { return _propertyAxisRangeEnd; }

	/// Returns whether analysis takes only selected particles into account.
	bool onlySelected() const { return _onlySelected; }

	/// Sets whether analysis only selected particles are taken into account.
	void setOnlySelected(bool onlySelected) { _onlySelected = onlySelected; }

    /// Returns true if binning in a single direction only.
    bool is1D() {
        return bin1D(_binDirection);
    }

    /// Returns true if binning in a single direction only.
    static bool bin1D(BinDirectionType d) {
        return d == CELL_VECTOR_1 || d == CELL_VECTOR_2 || d == CELL_VECTOR_3;
    }

    /// Return the coordinate index to be mapped on the X-axis.
    static int binDirectionX(BinDirectionType d) {
        return d & 3;
    }

    /// Return the coordinate index to be mapped on the Y-axis.
    static int binDirectionY(BinDirectionType d) {
        return (d >> 2) & 3;
    }

protected:

	/// Modifies the particle object.
	virtual PipelineStatus modifyParticles(TimePoint time, TimeInterval& validityInterval) override;

	/// This virtual method is called by the system when the modifier has been inserted into a PipelineObject.
	virtual void initializeModifier(PipelineObject* pipelineObject, ModifierApplication* modApp) override;

private:

	/// The particle property that serves as data source to be averaged.
	PropertyField<ParticlePropertyReference> _sourceProperty;

	/// Type of reduction operation
	PropertyField<ReductionOperationType, int> _reductionOperation;

	/// Compute first derivative.
	PropertyField<bool> _firstDerivative;

	/// Bin alignment
	PropertyField<BinDirectionType, int> _binDirection;

	/// Controls the number of spatial bins.
	PropertyField<int> _numberOfBinsX;

	/// Controls the number of spatial bins.
	PropertyField<int> _numberOfBinsY;

	/// Controls the whether the plotting range along the y-axis should be fixed.
	PropertyField<bool> _fixPropertyAxisRange;

	/// Controls the start value of the plotting y-axis.
	PropertyField<FloatType> _propertyAxisRangeStart;

	/// Controls the end value of the plotting y-axis.
	PropertyField<FloatType> _propertyAxisRangeEnd;

	/// Controls whether the modifier should take into account only selected particles.
	PropertyField<bool> _onlySelected;

	/// Stores the start value of the plotting x-axis.
	FloatType _xAxisRangeStart;

	/// Stores the end value of the plotting x-axis.
	FloatType _xAxisRangeEnd;

	/// Stores the start value of the plotting y-axis.
	FloatType _yAxisRangeStart;

	/// Stores the end value of the plotting y-axis.
	FloatType _yAxisRangeEnd;

	/// Stores the averaged data.
	/// Use double precision numbers to avoid precision loss when adding up a large number of values.
	QVector<double> _binData;

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Bin and reduce");
	Q_CLASSINFO("ModifierCategory", "Analysis");

	DECLARE_PROPERTY_FIELD(_reductionOperation);
	DECLARE_PROPERTY_FIELD(_firstDerivative);
	DECLARE_PROPERTY_FIELD(_binDirection);
	DECLARE_PROPERTY_FIELD(_numberOfBinsX);
	DECLARE_PROPERTY_FIELD(_numberOfBinsY);
	DECLARE_PROPERTY_FIELD(_fixPropertyAxisRange);
	DECLARE_PROPERTY_FIELD(_propertyAxisRangeStart);
	DECLARE_PROPERTY_FIELD(_propertyAxisRangeEnd);
	DECLARE_PROPERTY_FIELD(_sourceProperty);
	DECLARE_PROPERTY_FIELD(_onlySelected);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::Particles::BinAndReduceModifier::ReductionOperationType);
Q_DECLARE_METATYPE(Ovito::Particles::BinAndReduceModifier::BinDirectionType);
Q_DECLARE_TYPEINFO(Ovito::Particles::BinAndReduceModifier::ReductionOperationType, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(Ovito::Particles::BinAndReduceModifier::BinDirectionType, Q_PRIMITIVE_TYPE);

#endif // __OVITO_BIN_AND_REDUCE_MODIFIER_H
