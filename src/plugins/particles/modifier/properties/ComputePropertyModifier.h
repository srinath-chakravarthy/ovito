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

#ifndef __OVITO_COMPUTE_PROPERTY_MODIFIER_H
#define __OVITO_COMPUTE_PROPERTY_MODIFIER_H

#include <plugins/particles/Particles.h>
#include <core/gui/widgets/general/AutocompleteLineEdit.h>
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

	/// \brief Sets the math expressions that are used to calculate the values of the new property's components.
	/// \param expressions The mathematical formulas, one for each component of the property to create.
	/// \undoable
	void setExpressions(const QStringList& expressions) { _expressions = expressions; }

	/// \brief Returns the math expressions that are used to calculate the values of the new property's component.
	/// \return The math formulas.
	const QStringList& expressions() const { return _expressions; }

	/// \brief Sets the math expression that is used to calculate the values of one of the new property's components.
	/// \param index The property component for which the expression should be set.
	/// \param expression The math formula.
	/// \undoable
	void setExpression(const QString& expression, int index = 0) {
		if(index < 0 || index >= expressions().size())
			throw Exception("Property component index is out of range.");
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
			throw Exception("Property component index is out of range.");
		return expressions()[index];
	}

	/// \brief Sets the output particle property that receives the computed per-particle values.
	void setOutputProperty(const ParticlePropertyReference& prop) { _outputProperty = prop; }

	/// \brief Returns the output particle property that receives the computed per-particle values.
	const ParticlePropertyReference& outputProperty() const { return _outputProperty; }

	/// \brief Returns the number of vector components of the property to create.
	/// \return The number of vector components.
	/// \sa setPropertyComponentCount()
	int propertyComponentCount() const { return expressions().size(); }

	/// \brief Sets the number of vector components of the property to create.
	/// \param newComponentCount The number of vector components.
	/// \undoable
	void setPropertyComponentCount(int newComponentCount);

	/// \brief Returns whether the math expression is only evaluated for selected particles.
	/// \return \c true if the expression is only evaluated for selected particles; \c false if it is calculated for all particles.
	bool onlySelectedParticles() const { return _onlySelectedParticles; }

	/// \brief Sets whether the math expression is only evaluated for selected particles.
	/// \param enable Specifies the restriction to selected particles.
	/// \undoable
	void setOnlySelectedParticles(bool enable) { _onlySelectedParticles = enable; }

	/// Returns whether the contributions from neighbor terms are included in the computation.
	bool neighborModeEnabled() const { return _neighborModeEnabled; }

	/// Sets whether the contributions from neighbor terms are included in the computation.
	void setNeighborModeEnabled(bool enable) { _neighborModeEnabled = enable; }

	/// Returns the cutoff radius used to build the neighbor lists for the computation.
	FloatType cutoff() const { return _cutoff; }

	/// \brief Sets the cutoff radius used to build the neighbor lists for the computation.
	void setCutoff(FloatType newCutoff) { _cutoff = newCutoff; }

	/// \brief Sets the math expressions that are used to compute the neighbor-terms of the property function.
	/// \param expressions The formulas, one for each component of the property to create.
	/// \undoable
	void setNeighborExpressions(const QStringList& expressions) { _neighborExpressions = expressions; }

	/// \brief Returns the math expressions that are used to compute the neighbor-terms of the property function.
	/// \return The math formulas.
	const QStringList& neighborExpressions() const { return _neighborExpressions; }

	/// \brief Sets the math expression that is used to compute the neighbor-terms of the property function.
	/// \param index The property component for which the expression should be set.
	/// \param expression The math formula.
	/// \undoable
	void setNeighborExpression(const QString& expression, int index = 0) {
		if(index < 0 || index >= neighborExpressions().size())
			throw Exception("Property component index is out of range.");
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
			throw Exception("Property component index is out of range.");
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
				int frameNumber, int simulationTimestep) :
			ComputeEngine(validityInterval),
			_outputProperty(outputProperty),
			_positions(positions), _simCell(simCell),
			_selection(selectionProperty),
			_expressions(expressions), _neighborExpressions(neighborExpressions),
			_cutoff(cutoff),
			_frameNumber(frameNumber), _simulationTimestep(simulationTimestep),
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
		int _simulationTimestep;
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
	PropertyField<QStringList> _expressions;

	/// Specifies the output property that will receive the computed per-particles values.
	PropertyField<ParticlePropertyReference> _outputProperty;

	/// Controls whether the math expression is evaluated and output only for selected particles.
	PropertyField<bool> _onlySelectedParticles;

	/// Controls whether the contributions from neighbor terms are included in the computation.
	PropertyField<bool> _neighborModeEnabled;

	/// The math expressions for calculating the neighbor-terms of the property function.
	PropertyField<QStringList> _neighborExpressions;

	/// Controls the cutoff radius for the neighbor lists.
	PropertyField<FloatType> _cutoff;

	/// The list of input variables during the last evaluation.
	QStringList _inputVariableNames;

	/// Human-readable text listing the input variables during the last evaluation.
	QString _inputVariableTable;

	/// This stores the cached results of the modifier.
	QExplicitlySharedDataPointer<ParticleProperty> _computedProperty;

private:

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Compute property");
	Q_CLASSINFO("ModifierCategory", "Modification");
	Q_CLASSINFO("ClassNameAlias", "CreateExpressionPropertyModifier");	// This for backward compatibility with files written by Ovito 2.4 and older.

	DECLARE_PROPERTY_FIELD(_expressions);
	DECLARE_PROPERTY_FIELD(_outputProperty);
	DECLARE_PROPERTY_FIELD(_onlySelectedParticles);
	DECLARE_PROPERTY_FIELD(_neighborModeEnabled);
	DECLARE_PROPERTY_FIELD(_neighborExpressions);
	DECLARE_PROPERTY_FIELD(_cutoff);
};

OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * A properties editor for the ComputePropertyModifier class.
 */
class ComputePropertyModifierEditor : public ParticleModifierEditor
{
public:

	/// Default constructor.
	Q_INVOKABLE ComputePropertyModifierEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

	/// This method is called when a reference target changes.
	virtual bool referenceEvent(RefTarget* source, ReferenceEvent* event) override;

protected Q_SLOTS:

	/// Is called when the user has typed in an expression.
	void onExpressionEditingFinished();

	/// Updates the enabled/disabled status of the editor's controls.
	void updateEditorFields();

private:

	QWidget* rollout;
	QGroupBox* expressionsGroupBox;
	QList<AutocompleteLineEdit*> expressionBoxes;
	QList<QLabel*> expressionBoxLabels;
	QGridLayout* expressionsLayout;

	QGroupBox* neighborExpressionsGroupBox;
	QList<AutocompleteLineEdit*> neighborExpressionBoxes;
	QList<QLabel*> neighborExpressionBoxLabels;
	QGridLayout* neighborExpressionsLayout;

	bool editorUpdatePending;

	QLabel* variableNamesDisplay;

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace

#endif // __OVITO_COMPUTE_PROPERTY_MODIFIER_H
