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

#include <complex>

#include <plugins/particles/Particles.h>
#include <plugins/particles/data/ParticleProperty.h>
#include <plugins/particles/util/CutoffNeighborFinder.h>
#include "../particles/modifier/AsynchronousParticleModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * \brief This modifier computes the coordination number of each particle (i.e. the number of neighbors within a given cutoff radius).
 */
class OVITO_PARTICLES_EXPORT CorrelationFunctionModifier : public AsynchronousParticleModifier
{
public:

	/// Constructor.
	Q_INVOKABLE CorrelationFunctionModifier(DataSet* dataset);

	/// Sets the first source particle property for which the correlation should be computed.
	void setSourceProperty1(const ParticlePropertyReference& prop) { _sourceProperty1 = prop; }

	/// Returns the first source particle property for which the correlation is computed.
	const ParticlePropertyReference& sourceProperty1() const { return _sourceProperty1; }

	/// Sets the second source particle property for which the correlation should be computed.
	void setSourceProperty2(const ParticlePropertyReference& prop) { _sourceProperty2 = prop; }

	/// Returns the second source particle property for which the correlation is computed.
	const ParticlePropertyReference& sourceProperty2() const { return _sourceProperty2; }

	/// \brief Sets the cutoff radius used for the FFT grid.
	void setFftGridSpacing(FloatType newFftBinSize) { _fftGridSpacing = newFftBinSize; }

	/// Returns the cutoff radius used to build the neighbor lists for the analysis.
	FloatType fftGridSpacing() const { return _fftGridSpacing; }

	/// \brief Sets the cutoff radius used to build the neighbor lists for the analysis.
	void setNeighCutoff(FloatType newNeighCutoff) { _neighCutoff = newNeighCutoff; }

	/// Returns the cutoff radius used to build the neighbor lists for the analysis.
	FloatType neighCutoff() const { return _neighCutoff; }

	/// Sets the number of bins in the computed RDF histogram.
	void setNumberOfNeighBins(int n) { _numberOfNeighBins = n; }

	/// Returns the number of bins in the computed RDF histogram.
	int numberOfNeighBins() const { return _numberOfNeighBins; }

	/// Sets the type of the real-space plot
	void setTypeOfRealSpacePlot(int t) { _typeOfRealSpacePlot = t; }

	/// Returns the type of the real-space plot
	int typeOfRealSpacePlot() const { return _typeOfRealSpacePlot; }

	/// Sets the type of the reciprocal-space plot
	void setTypeOfReciprocalSpacePlot(int t) { _typeOfReciprocalSpacePlot = t; }

	/// Returns the type of the reciprocal-space plot
	int typeOfReciprocalSpacePlot() const { return _typeOfReciprocalSpacePlot; }

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

private:

	/// Computes the modifier's results.
	class CorrelationAnalysisEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		CorrelationAnalysisEngine(const TimeInterval& validityInterval,
								  ParticleProperty* positions,
								  ParticleProperty* sourceProperty1,
								  ParticleProperty* sourceProperty2,
								  const SimulationCell& simCell,
								  FloatType fftGridSpacing,
								  FloatType neighCutoff,
								  int numberOfNeighBins) :
			ComputeEngine(validityInterval), _positions(positions),
			_sourceProperty1(sourceProperty1), _sourceProperty2(sourceProperty2),
			_simCell(simCell), _fftGridSpacing(fftGridSpacing), _neighCutoff(neighCutoff),
			_neighCorrelation(numberOfNeighBins, 0.0),
			_neighCorrelationX(numberOfNeighBins) {}

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

	/// The particle property that serves as the first data source for the correlation function.
	PropertyField<ParticlePropertyReference> _sourceProperty1;

	/// The particle property that serves as the second data source for the correlation function.
	PropertyField<ParticlePropertyReference> _sourceProperty2;

	/// Controls the cutoff radius for the FFT grid.
	PropertyField<FloatType> _fftGridSpacing;

	/// Controls the cutoff radius for the neighbor lists.
	PropertyField<FloatType> _neighCutoff;

	/// Controls the number of RDF histogram bins.
	PropertyField<FloatType> _numberOfNeighBins;

	/// Type of real-space plot (lin-lin, log-lin or log-log)
	PropertyField<int> _typeOfRealSpacePlot;

	/// Type of reciprocal-space plot (lin-lin, log-lin or log-log)
	PropertyField<int> _typeOfReciprocalSpacePlot;

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

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Correlation function");
	Q_CLASSINFO("ModifierCategory", "Analysis");

	DECLARE_PROPERTY_FIELD(_sourceProperty1);
	DECLARE_PROPERTY_FIELD(_sourceProperty2);
	DECLARE_PROPERTY_FIELD(_fftGridSpacing);
	DECLARE_PROPERTY_FIELD(_neighCutoff);
	DECLARE_PROPERTY_FIELD(_numberOfNeighBins);
	DECLARE_PROPERTY_FIELD(_typeOfRealSpacePlot);
	DECLARE_PROPERTY_FIELD(_typeOfReciprocalSpacePlot);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace

#endif // __OVITO_CORRELATION_FUNCTION_MODIFIER_H
