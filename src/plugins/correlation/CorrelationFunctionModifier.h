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

	/// \brief Sets the cutoff radius used to build the neighbor lists for the analysis.
	void setCutoff(FloatType newCutoff) { _cutoff = newCutoff; }

	/// Returns the cutoff radius used to build the neighbor lists for the analysis.
	FloatType cutoff() const { return _cutoff; }

	/// Returns the Y coordinates of the real-space correlation function.
	const QVector<FloatType>& realSpaceCorrelationFunction() const { return _realSpaceCorrelationFunction; }

	/// Returns the X coordinates of the real-space correlation function.
	const QVector<FloatType>& realSpaceCorrelationFunctionX() const { return _realSpaceCorrelationFunctionX; }

	/// Returns the Y coordinates of the short-ranged part of the real-space correlation function.
	const QVector<FloatType>& shortRangedRealSpaceCorrelationFunction() const { return _shortRangedRealSpaceCorrelationFunction; }

	/// Returns the X coordinates of the short-ranged part of the real-space correlation function.
	const QVector<FloatType>& shortRangedRealSpaceCorrelationFunctionX() const { return _shortRangedRealSpaceCorrelationFunctionX; }

	/// Returns the Y coordinates of the reciprocal-space correlation function.
	const QVector<FloatType>& reciprocalSpaceCorrelationFunction() const { return _reciprocalSpaceCorrelationFunction; }

	/// Returns the X coordinates of the reciprocal-space correlation function.
	const QVector<FloatType>& reciprocalSpaceCorrelationFunctionX() const { return _reciprocalSpaceCorrelationFunctionX; }

	/// Returns the number of bins in the computed RDF histogram.
	int numberOfBinsForShortRangedCalculation() const { return _numberOfBinsForShortRangedCalculation; }

	/// Sets the number of bins in the computed RDF histogram.
	void setnumberOfBinsForShortRangedCalculation(int n) { _numberOfBinsForShortRangedCalculation = n; }

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
								  FloatType cutoff,
								  int numberOfBinsForShortRangedCalculation) :
			ComputeEngine(validityInterval), _positions(positions),
			_sourceProperty1(sourceProperty1), _sourceProperty2(sourceProperty2),
			_simCell(simCell), _cutoff(cutoff),
			_shortRangedRealSpaceCorrelationFunction(numberOfBinsForShortRangedCalculation, 0.0),
			_shortRangedRealSpaceCorrelationFunctionX(numberOfBinsForShortRangedCalculation) {}

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

		/// Returns the cutoff radius.
		FloatType cutoff() const { return _cutoff; }

		/// Returns the real-space correlation function.
		const QVector<FloatType>& realSpaceCorrelationFunction() const { return _realSpaceCorrelationFunction; }

		/// Returns the distances for which the real-space correlation function is tabulated.
		const QVector<FloatType>& realSpaceCorrelationFunctionX() const { return _realSpaceCorrelationFunctionX; }

		/// Returns the short-ranged real-space correlation function.
		const QVector<FloatType>& shortRangedRealSpaceCorrelationFunction() const { return _shortRangedRealSpaceCorrelationFunction; }

		/// Returns the distances for which the short-ranged real-space correlation function is tabulated.
		const QVector<FloatType>& shortRangedRealSpaceCorrelationFunctionX() const { return _shortRangedRealSpaceCorrelationFunctionX; }

		/// Returns the reciprocal-space correlation function.
		const QVector<FloatType>& reciprocalSpaceCorrelationFunction() const { return _reciprocalSpaceCorrelationFunction; }

		/// Returns the wavevectors for which the reciprocal-space correlation function is tabulated.
		const QVector<FloatType>& reciprocalSpaceCorrelationFunctionX() const { return _reciprocalSpaceCorrelationFunctionX; }

	private:

		/// Map property onto grid.
		void mapToSpatialGrid(ParticleProperty *property,
							  size_t propertyVectorComponent,
							  const AffineTransformation &reciprocalCell,
							  int nX, int nY, int nZ,
							  QVector<FloatType> &gridData);

		FloatType _cutoff;
		SimulationCell _simCell;
		QExplicitlySharedDataPointer<ParticleProperty> _positions;
		QExplicitlySharedDataPointer<ParticleProperty> _sourceProperty1;
		QExplicitlySharedDataPointer<ParticleProperty> _sourceProperty2;
		QVector<FloatType> _realSpaceCorrelationFunction;
		QVector<FloatType> _realSpaceCorrelationFunctionX;
		QVector<FloatType> _shortRangedRealSpaceCorrelationFunction;
		QVector<FloatType> _shortRangedRealSpaceCorrelationFunctionX;
		QVector<FloatType> _reciprocalSpaceCorrelationFunction;
		QVector<FloatType> _reciprocalSpaceCorrelationFunctionX;
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

	/// Controls the cutoff radius for the neighbor lists.
	PropertyField<FloatType> _cutoff;

	/// Controls the number of RDF histogram bins.
	PropertyField<FloatType> _numberOfBinsForShortRangedCalculation;

	/// The real-space correlation function.
	QVector<FloatType> _realSpaceCorrelationFunction;

	/// The distances for which the real-space correlation function is tabulated.
	QVector<FloatType> _realSpaceCorrelationFunctionX;

	/// The short-ranged part of the real-space correlation function.
	QVector<FloatType> _shortRangedRealSpaceCorrelationFunction;

	/// The distances for which short-ranged part of the real-space correlation function is tabulated.
	QVector<FloatType> _shortRangedRealSpaceCorrelationFunctionX;

	/// The reciprocal-space correlation function.
	QVector<FloatType> _reciprocalSpaceCorrelationFunction;

	/// The wavevevtors for which the reciprocal-space correlation function is tabulated.
	QVector<FloatType> _reciprocalSpaceCorrelationFunctionX;

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Correlation function");
	Q_CLASSINFO("ModifierCategory", "Analysis");

	DECLARE_PROPERTY_FIELD(_sourceProperty1);
	DECLARE_PROPERTY_FIELD(_sourceProperty2);
	DECLARE_PROPERTY_FIELD(_cutoff);
	DECLARE_PROPERTY_FIELD(_numberOfBinsForShortRangedCalculation);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace

#endif // __OVITO_CORRELATION_FUNCTION_MODIFIER_H
