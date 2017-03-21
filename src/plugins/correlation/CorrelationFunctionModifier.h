///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2014) Alexander Stukowski
//  Copyright (2017) Lars Pastewka
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

#ifndef __OVITO_CORRELATION_FUNCTION_MODIFIER_H
#define __OVITO_CORRELATION_FUNCTION_MODIFIER_H

#include <plugins/particles/Particles.h>
#include <plugins/particles/data/ParticleProperty.h>
#include <plugins/particles/util/CutoffNeighborFinder.h>
#include "../particles/modifier/AsynchronousParticleModifier.h"

#include <complex>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * \brief This modifier computes the coordination number of each particle (i.e. the number of neighbors within a given cutoff radius).
 */
class OVITO_PARTICLES_EXPORT CorrelationFunctionModifier : public AsynchronousParticleModifier
{
public:

	/// Constructor.
	Q_INVOKABLE CorrelationFunctionModifier(DataSet* dataset);

	/// Returns the Y coordinates of the real-space correlation function.
	const QVector<FloatType>& realSpaceCorrelation() const { return _realSpaceCorrelation; }

	/// Returns the X coordinates of the real-space correlation function.
	const QVector<FloatType>& realSpaceCorrelationX() const { return _realSpaceCorrelationX; }

	/// Returns the Y coordinates of the short-ranged part of the real-space correlation function.
	const QVector<FloatType>& neighCorrelation() const { return _neighCorrelation; }

	/// Returns the X coordinates of the short-ranged part of the real-space correlation function.
	const QVector<FloatType>& neighCorrelationX() const { return _neighCorrelationX; }

	/// Returns the Y coordinates of the reciprocal-space correlation function.
	const QVector<FloatType>& reciprocalSpaceCorrelation() const { return _reciprocalSpaceCorrelation; }

	/// Returns the X coordinates of the reciprocal-space correlation function.
	const QVector<FloatType>& reciprocalSpaceCorrelationX() const { return _reciprocalSpaceCorrelationX; }

	/// Returns the mean of the first property.
	FloatType mean1() const { return _mean1; }

	/// Returns the mean of the second property.
	FloatType mean2() const { return _mean2; }

	/// Returns the (co)variance.
	FloatType covariance() const { return _covariance; }

	/// Update plot ranges.
	virtual void updateRanges(FloatType offset, FloatType fac, FloatType reciprocalFac);

private:

	/// Computes the modifier's results.
	class CorrelationAnalysisEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		CorrelationAnalysisEngine(const TimeInterval& validityInterval,
								  ParticleProperty* positions,
								  ParticleProperty* sourceProperty1,
								  size_t vecComponent1,
								  ParticleProperty* sourceProperty2,
								  size_t vecComponent2,
								  const SimulationCell& simCell,
								  FloatType fftGridSpacing,
								  bool doComputeNeighCorrelation,
								  FloatType neighCutoff,
								  int numberOfNeighBins) :
			ComputeEngine(validityInterval), _positions(positions),
			_sourceProperty1(sourceProperty1), _vecComponent1(vecComponent1),
			_sourceProperty2(sourceProperty2), _vecComponent2(vecComponent2),
			_simCell(simCell), _fftGridSpacing(fftGridSpacing),
			_neighCutoff(neighCutoff),
			_neighCorrelation(numberOfNeighBins, 0.0),
			_neighCorrelationX(numberOfNeighBins) {
				if (!doComputeNeighCorrelation) {
					_neighCorrelation.clear();
					_neighCorrelationX.clear();
				}
			}

		/// Compute real and reciprocal space correlation function via FFT.
		void computeFftCorrelation();

		/// Compute real space correlation function via direction summation over neighbors.
		void computeNeighCorrelation();

		/// Compute means and covariance.
		void computeLimits();

		/// Computes the modifier's results and stores them in this object for later retrieval.
		virtual void perform() override;

		/// Returns the property storage that contains the input particle positions.
		ParticleProperty* positions() const { return _positions.data(); }

		/// Returns the property storage that contains the first input particle property.
		ParticleProperty* sourceProperty1() const { return _sourceProperty1.data(); }

		/// Returns the property storage that contains the second input particle property.
		ParticleProperty* sourceProperty2() const { return _sourceProperty2.data(); }

		/// Returns the simulation cell data.
		const SimulationCell& cell() const { return _simCell; }

		/// Returns the FFT cutoff radius.
		FloatType fftGridSpacing() const { return _fftGridSpacing; }

		/// Returns the neighbor cutoff radius.
		FloatType neighCutoff() const { return _neighCutoff; }

		/// Returns the real-space correlation function.
		const QVector<FloatType>& realSpaceCorrelation() const { return _realSpaceCorrelation; }

		/// Returns the distances for which the real-space correlation function is tabulated.
		const QVector<FloatType>& realSpaceCorrelationX() const { return _realSpaceCorrelationX; }

		/// Returns the short-ranged real-space correlation function.
		const QVector<FloatType>& neighCorrelation() const { return _neighCorrelation; }

		/// Returns the distances for which the short-ranged real-space correlation function is tabulated.
		const QVector<FloatType>& neighCorrelationX() const { return _neighCorrelationX; }

		/// Returns the reciprocal-space correlation function.
		const QVector<FloatType>& reciprocalSpaceCorrelation() const { return _reciprocalSpaceCorrelation; }

		/// Returns the wavevectors for which the reciprocal-space correlation function is tabulated.
		const QVector<FloatType>& reciprocalSpaceCorrelationX() const { return _reciprocalSpaceCorrelationX; }

		/// Returns the mean of the first property.
		FloatType mean1() const { return _mean1; }

		/// Returns the mean of the second property.
		FloatType mean2() const { return _mean2; }

		/// Returns the (co)variance.
		FloatType covariance() const { return _covariance; }

	private:

		/// Real-to-complex FFT.
		void r2cFFT(int nX, int nY, int nZ, QVector<FloatType> &rData, QVector<std::complex<FloatType>> &cData);

		/// Complex-to-real inverse FFT
		void c2rFFT(int nX, int nY, int nZ, QVector<std::complex<FloatType>> &cData, QVector<FloatType> &rData);

		/// Map property onto grid.
		void mapToSpatialGrid(ParticleProperty *property,
							  size_t propertyVectorComponent,
							  const AffineTransformation &reciprocalCell,
							  int nX, int nY, int nZ,
							  QVector<FloatType> &gridData);

		size_t _vecComponent1;
		size_t _vecComponent2;
		FloatType _fftGridSpacing;
		FloatType _neighCutoff;
		SimulationCell _simCell;
		QExplicitlySharedDataPointer<ParticleProperty> _positions;
		QExplicitlySharedDataPointer<ParticleProperty> _sourceProperty1;
		QExplicitlySharedDataPointer<ParticleProperty> _sourceProperty2;
		QVector<FloatType> _realSpaceCorrelation;
		QVector<FloatType> _realSpaceCorrelationX;
		QVector<FloatType> _neighCorrelation;
		QVector<FloatType> _neighCorrelationX;
		QVector<FloatType> _reciprocalSpaceCorrelation;
		QVector<FloatType> _reciprocalSpaceCorrelationX;
		FloatType _mean1;
		FloatType _mean2;
		FloatType _covariance;
	};

protected:

	/// This virtual method is called by the system when the modifier has been inserted into a PipelineObject.
	virtual void initializeModifier(PipelineObject* pipelineObject, ModifierApplication* modApp) override;

	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// Creates a computation engine that will compute the modifier's results.
	virtual std::shared_ptr<ComputeEngine> createEngine(TimePoint time, TimeInterval validityInterval) override;

	/// Unpacks the results of the computation engine and stores them in the modifier.
	virtual void transferComputationResults(ComputeEngine* engine) override;

	/// Lets the modifier insert the cached computation results into the modification pipeline.
	virtual PipelineStatus applyComputationResults(TimePoint time, TimeInterval& validityInterval) override;

private:

	/// The real-space correlation function.
	QVector<FloatType> _realSpaceCorrelation;

	/// The distances for which the real-space correlation function is tabulated.
	QVector<FloatType> _realSpaceCorrelationX;

	/// The short-ranged part of the real-space correlation function.
	QVector<FloatType> _neighCorrelation;

	/// The distances for which short-ranged part of the real-space correlation function is tabulated.
	QVector<FloatType> _neighCorrelationX;

	/// The reciprocal-space correlation function.
	QVector<FloatType> _reciprocalSpaceCorrelation;

	/// The wavevevtors for which the reciprocal-space correlation function is tabulated.
	QVector<FloatType> _reciprocalSpaceCorrelationX;

	/// Mean of the first property.
	FloatType _mean1;

	/// Mean of the second property.
	FloatType _mean2;

	/// (Co)variance.
	FloatType _covariance;

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Correlation function");
	Q_CLASSINFO("ModifierCategory", "Analysis");

	/// The particle property that serves as the first data source for the correlation function.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(ParticlePropertyReference, sourceProperty1, setSourceProperty1);
	/// The particle property that serves as the second data source for the correlation function.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(ParticlePropertyReference, sourceProperty2, setSourceProperty2);
	/// Controls the cutoff radius for the FFT grid.
	DECLARE_PROPERTY_FIELD(FloatType, fftGridSpacing);
	/// Controls whether the real-space correlation should be computed by direct summation.
	DECLARE_PROPERTY_FIELD(bool, doComputeNeighCorrelation);
	/// Controls the cutoff radius for the neighbor lists.
	DECLARE_PROPERTY_FIELD(FloatType, neighCutoff);
	/// Controls the number of bins for the neighbor part of the real-space correlation function.
	DECLARE_PROPERTY_FIELD(int, numberOfNeighBins);
	/// Controls the normalization of the real-space correlation function.
	DECLARE_PROPERTY_FIELD(bool, normalizeRealSpace);
	/// Type of real-space plot (lin-lin, log-lin or log-log)
	DECLARE_PROPERTY_FIELD(int, typeOfRealSpacePlot);
	/// Controls the whether the range of the x-axis of the plot should be fixed.
	DECLARE_PROPERTY_FIELD(bool, fixRealSpaceXAxisRange);
	/// Controls the start value of the x-axis.
	DECLARE_PROPERTY_FIELD(FloatType, realSpaceXAxisRangeStart);
	/// Controls the end value of the x-axis.
	DECLARE_PROPERTY_FIELD(FloatType, realSpaceXAxisRangeEnd);
	/// Controls the whether the range of the y-axis of the plot should be fixed.
	DECLARE_PROPERTY_FIELD(bool, fixRealSpaceYAxisRange);
	/// Controls the start value of the y-axis.
	DECLARE_PROPERTY_FIELD(FloatType, realSpaceYAxisRangeStart);
	/// Controls the end value of the y-axis.
	DECLARE_PROPERTY_FIELD(FloatType, realSpaceYAxisRangeEnd);
	/// Controls the normalization of the reciprocal-space correlation function.
	DECLARE_PROPERTY_FIELD(bool, normalizeReciprocalSpace);
	/// Type of reciprocal-space plot (lin-lin, log-lin or log-log)
	DECLARE_PROPERTY_FIELD(int, typeOfReciprocalSpacePlot);
	/// Controls the whether the range of the x-axis of the plot should be fixed.
	DECLARE_PROPERTY_FIELD(bool, fixReciprocalSpaceXAxisRange);
	/// Controls the start value of the x-axis.
	DECLARE_PROPERTY_FIELD(FloatType, reciprocalSpaceXAxisRangeStart);
	/// Controls the end value of the x-axis.
	DECLARE_PROPERTY_FIELD(FloatType, reciprocalSpaceXAxisRangeEnd);
	/// Controls the whether the range of the y-axis of the plot should be fixed.
	DECLARE_PROPERTY_FIELD(bool, fixReciprocalSpaceYAxisRange);
	/// Controls the start value of the y-axis.
	DECLARE_PROPERTY_FIELD(FloatType, reciprocalSpaceYAxisRangeStart);
	/// Controls the end value of the y-axis.
	DECLARE_PROPERTY_FIELD(FloatType, reciprocalSpaceYAxisRangeEnd);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace

#endif // __OVITO_CORRELATION_FUNCTION_MODIFIER_H
