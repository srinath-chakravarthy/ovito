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
#include <plugins/particles/util/ParticleExpressionEvaluator.h>
#include "../AsynchronousParticleModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Properties)

/**
 * \brief Computes the values of a particle property from a user-defined math expression.
 */
class OVITO_PARTICLES_EXPORT ComputePropertyModifier : public AsynchronousParticleModifier
{
public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE ComputePropertyModifier(DataSet* dataset);

	/// \brief Sets the math expression that is used to calculate the values of one of the new property's components.
	/// \param index The property component for which the expression should be set.
	/// \param expression The math formula.
	/// \undoable
	void setExpression(const QString& expression, int index = 0) {
		if(index < 0 || index >= expressions().size())
			throwException("Property component index is out of range.");
		QStringList copy = _expressions;
		copy[index] = expression;
		_expressions = copy;
	}

	/// \brief Returns the math expression that is used to calculate the values of one of the new property's components.
	/// \param index The property component for which the expression should be returned.
	/// \return The math formula used to calculates the channel's values.
	/// \undoable
	const QString& expression(int index = 0) const {
		if(index < 0 || index >= expressions().size())
			throwException("Property component index is out of range.");
		return expressions()[index];
	}

	/// \brief Returns the number of vector components of the property to create.
	/// \return The number of vector components.
	/// \sa setPropertyComponentCount()
	int propertyComponentCount() const { return expressions().size(); }

	/// \brief Sets the number of vector components of the property to create.
	/// \param newComponentCount The number of vector components.
	/// \undoable
	void setPropertyComponentCount(int newComponentCount);

	/// \brief Sets the math expression that is used to compute the neighbor-terms of the property function.
	/// \param index The property component for which the expression should be set.
	/// \param expression The math formula.
	/// \undoable
	void setNeighborExpression(const QString& expression, int index = 0) {
		if(index < 0 || index >= neighborExpressions().size())
			throwException("Property component index is out of range.");
		QStringList copy = _neighborExpressions;
		copy[index] = expression;
		_neighborExpressions = copy;
	}

	/// \brief Returns the math expression that is used to compute the neighbor-terms of the property function.
	/// \param index The property component for which the expression should be returned.
	/// \return The math formula.
	/// \undoable
	const QString& neighborExpression(int index = 0) const {
		if(index < 0 || index >= neighborExpressions().size())
			throwException("Property component index is out of range.");
		return neighborExpressions()[index];
	}

	/// \brief Returns the list of available input variables.
	const QStringList& inputVariableNames() const { return _inputVariableNames; }

	/// \brief Returns a human-readable text listing the input variables.
	const QString& inputVariableTable() const { return _inputVariableTable; }

protected:

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

	/// \brief Allows the object to parse the serialized contents of a property field in a custom way.
	virtual bool loadPropertyFieldFromStream(ObjectLoadStream& stream, const ObjectLoadStream::SerializedPropertyField& serializedField) override;

	/// \brief This virtual method is called by the system when the modifier has been inserted into a PipelineObject.
	virtual void initializeModifier(PipelineObject* pipeline, ModifierApplication* modApp) override;

	/// \brief Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// Creates a computation engine that will compute the modifier's results.
	virtual std::shared_ptr<ComputeEngine> createEngine(TimePoint time, TimeInterval validityInterval) override;

	/// Unpacks the results of the computation engine and stores them in the modifier.
	virtual void transferComputationResults(ComputeEngine* engine) override;

	/// Lets the modifier insert the cached computation results into the modification pipeline.
	virtual PipelineStatus applyComputationResults(TimePoint time, TimeInterval& validityInterval) override;

	/// Asynchronous compute engine that does the actual work in a background thread.
	class PropertyComputeEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		PropertyComputeEngine(const TimeInterval& validityInterval, TimePoint time,
				ParticleProperty* outputProperty, ParticleProperty* positions, ParticleProperty* selectionProperty,
				const SimulationCell& simCell, FloatType cutoff,
				const QStringList& expressions, const QStringList& neighborExpressions,
				std::vector<QExplicitlySharedDataPointer<ParticleProperty>>&& inputProperties,
				int frameNumber, const QVariantMap& attributes) :
			ComputeEngine(validityInterval),
			_outputProperty(outputProperty),
			_positions(positions), _simCell(simCell),
			_selection(selectionProperty),
			_expressions(expressions), _neighborExpressions(neighborExpressions),
			_cutoff(cutoff),
			_frameNumber(frameNumber), _attributes(attributes),
			_inputProperties(std::move(inputProperties)) {
			initializeEngine(time);
		}

		/// This is called by the constructor to prepare the compute engine.
		void initializeEngine(TimePoint time);

		/// Computes the modifier's results and stores them for later retrieval.
		virtual void perform() override;

		/// Returns the property storage that contains the input particle positions.
		ParticleProperty* positions() const { return _positions.data(); }

		/// Returns the property storage that contains the input particle selection.
		ParticleProperty* selection() const { return _selection.data(); }

		/// Returns the simulation cell data.
		const SimulationCell& cell() const { return _simCell; }

		/// Returns the property storage that will receive the computed values.
		ParticleProperty* outputProperty() const { return _outputProperty.data(); }

		/// Returns the list of available input variables.
		const QStringList& inputVariableNames() const { return _inputVariableNames; }

		/// Returns a human-readable text listing the input variables.
		const QString& inputVariableTable() const { return _inputVariableTable; }

		/// Indicates whether contributions from particle neighbors are taken into account.
		bool neighborMode() const { return _cutoff != 0; }

	private:

		FloatType _cutoff;
		SimulationCell _simCell;
		int _frameNumber;
		QVariantMap _attributes;
		QStringList _expressions;
		QStringList _neighborExpressions;
		QExplicitlySharedDataPointer<ParticleProperty> _positions;
		QExplicitlySharedDataPointer<ParticleProperty> _selection;
		QExplicitlySharedDataPointer<ParticleProperty> _outputProperty;
		std::vector<QExplicitlySharedDataPointer<ParticleProperty>> _inputProperties;
		QStringList _inputVariableNames;
		QString _inputVariableTable;
		ParticleExpressionEvaluator _evaluator;
		ParticleExpressionEvaluator _neighborEvaluator;
	};

	/// The math expressions for calculating the property values. One for every vector component.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QStringList, expressions, setExpressions);

	/// Specifies the output property that will receive the computed per-particles values.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(ParticlePropertyReference, outputProperty, setOutputProperty);

	/// Controls whether the math expression is evaluated and output only for selected particles.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, onlySelectedParticles, setOnlySelectedParticles);

	/// Controls whether the contributions from neighbor terms are included in the computation.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, neighborModeEnabled, setNeighborModeEnabled);

	/// The math expressions for calculating the neighbor-terms of the property function.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QStringList, neighborExpressions, setNeighborExpressions);

	/// Controls the cutoff radius for the neighbor lists.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, cutoff, setCutoff);

	/// The list of input variables during the last evaluation.
	QStringList _inputVariableNames;

	/// Human-readable text listing the input variables during the last evaluation.
	QString _inputVariableTable;

	/// This stores the cached results of the modifier.
	QExplicitlySharedDataPointer<ParticleProperty> _computedProperty;

	/// The cached display objects that are attached to the output particle property.
	DECLARE_VECTOR_REFERENCE_FIELD(DisplayObject, cachedDisplayObjects);

private:

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Compute property");
	Q_CLASSINFO("ModifierCategory", "Modification");
	Q_CLASSINFO("ClassNameAlias", "CreateExpressionPropertyModifier");	// This for backward compatibility with files written by Ovito 2.4 and older.
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


