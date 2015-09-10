///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2015) Alexander Stukowski
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

#ifndef __OVITO_RADIAL_COMPUTE_MODIFIER_H
#define __OVITO_RADIAL_COMPUTE_MODIFIER_H

#include <plugins/particles/Particles.h>
#include <core/gui/properties/StringParameterUI.h>
#include <core/gui/properties/VariantComboBoxParameterUI.h>
#include <plugins/particles/data/ParticleProperty.h>
#include "../AsynchronousParticleModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Properties)

/**
 * \brief Computes the values of a particle property from a user-defined math expression.
 */
class OVITO_PARTICLES_EXPORT RadialComputeModifier : public AsynchronousParticleModifier
{
public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE RadialComputeModifier(DataSet* dataset);

	/// Returns the cutoff radius used to build the neighbor lists for the computation.
	FloatType cutoff() const { return _cutoff; }

	/// \brief Sets the cutoff radius used to build the neighbor lists for the computation.
	void setCutoff(FloatType newCutoff) { _cutoff = newCutoff; }

	/// \brief Sets the math expressions that are used to compute the self-term of the property function.
	/// \param expressions The formulas, one for each component of the property to create.
	/// \undoable
	void setSelfExpressions(const QStringList& expressions) { _selfExpressions = expressions; }

	/// \brief Returns the math expressions that are used to compute self-term of the property function.
	/// \return The math formulas.
	const QStringList& selfExpressions() const { return _selfExpressions; }

	/// \brief Sets the math expression that is used to compute the self-term of the property function.
	/// \param index The property component for which the expression should be set.
	/// \param expression The math formula.
	/// \undoable
	void setSelfExpression(const QString& expression, int index = 0) {
		if(index < 0 || index >= selfExpressions().size())
			throw Exception("Property component index is out of range.");
		QStringList copy = _selfExpressions;
		copy[index] = expression;
		_selfExpressions = copy;
	}

	/// \brief Returns the math expression that is used to compute the self-term of the property function.
	/// \param index The property component for which the expression should be returned.
	/// \return The math formula.
	/// \undoable
	const QString& selfExpression(int index = 0) const {
		if(index < 0 || index >= selfExpressions().size())
			throw Exception("Property component index is out of range.");
		return selfExpressions()[index];
	}

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

	/// \brief Sets the output particle property that receives the computed per-particle values.
	void setOutputProperty(const ParticlePropertyReference& prop) { _outputProperty = prop; }

	/// \brief Returns the output particle property that receives the computed per-particle values.
	const ParticlePropertyReference& outputProperty() const { return _outputProperty; }

	/// \brief Returns the number of vector components of the property to create.
	/// \return The number of vector components.
	int propertyComponentCount() const { return selfExpressions().size(); }

	/// \brief Sets the number of vector components of the property to create.
	/// \param newComponentCount The number of vector components.
	/// \undoable
	void setPropertyComponentCount(int newComponentCount);

	/// \brief Returns the list of available input variables.
	const QStringList& inputVariableNames() const { return _inputVariableNames; }

	/// \brief Returns a human-readable text listing the input variables.
	const QString& inputVariableTable() const { return _inputVariableTable; }

protected:

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

	/// Does the actual work.
	class RadialComputeEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		RadialComputeEngine(const TimeInterval& validityInterval,
				ParticleProperty* outputProperty, ParticleProperty* positions,
				const SimulationCell& simCell, FloatType cutoff,
				const QStringList& selfExpressions, const QStringList& neighborExpressions,
				std::vector<QExplicitlySharedDataPointer<ParticleProperty>>&& inputProperties,
				int frameNumber, int simulationTimestep) :
			ComputeEngine(validityInterval),
			_outputProperty(outputProperty),
			_positions(positions), _simCell(simCell),
			_selfExpressions(selfExpressions), _neighborExpressions(neighborExpressions),
			_cutoff(cutoff),
			_frameNumber(frameNumber), _simulationTimestep(simulationTimestep),
			_inputProperties(std::move(inputProperties)) {}

		/// Computes the modifier's results and stores them in this object for later retrieval.
		virtual void perform() override;

		/// Returns the property storage that contains the input particle positions.
		ParticleProperty* positions() const { return _positions.data(); }

		/// Returns the simulation cell data.
		const SimulationCell& cell() const { return _simCell; }

		/// Returns the property storage that will receive the computed values.
		ParticleProperty* outputProperty() const { return _outputProperty.data(); }

	private:

		FloatType _cutoff;
		SimulationCell _simCell;
		int _frameNumber;
		int _simulationTimestep;
		QStringList _selfExpressions;
		QStringList _neighborExpressions;
		QExplicitlySharedDataPointer<ParticleProperty> _positions;
		QExplicitlySharedDataPointer<ParticleProperty> _outputProperty;
		std::vector<QExplicitlySharedDataPointer<ParticleProperty>> _inputProperties;
	};

private:

	/// The math expressions for calculating the self-term of the property function.
	PropertyField<QStringList> _selfExpressions;

	/// The math expressions for calculating the neighbor-terms of the property function.
	PropertyField<QStringList> _neighborExpressions;

	/// Specifies the output property that will receive the computed per-particles values.
	PropertyField<ParticlePropertyReference> _outputProperty;

	/// Controls the cutoff radius for the neighbor lists.
	PropertyField<FloatType> _cutoff;

	/// This stores the cached results of the modifier.
	QExplicitlySharedDataPointer<ParticleProperty> _computedProperty;

	/// The list of input variables during the last evaluation.
	QStringList _inputVariableNames;

	/// Human-readable text listing the input variables during the last evaluation.
	QString _inputVariableTable;

private:

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Radial compute");
	Q_CLASSINFO("ModifierCategory", "Modification");

	DECLARE_PROPERTY_FIELD(_selfExpressions);
	DECLARE_PROPERTY_FIELD(_neighborExpressions);
	DECLARE_PROPERTY_FIELD(_outputProperty);
	DECLARE_PROPERTY_FIELD(_cutoff);
};

OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * A properties editor for the RadialComputeModifier class.
 */
class RadialComputeModifierEditor : public ParticleModifierEditor
{
public:

	/// Default constructor.
	Q_INVOKABLE RadialComputeModifierEditor() {}

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
	QList<QLineEdit*> expressionBoxes;
	QList<QLabel*> expressionBoxLabels;

	QVBoxLayout* expressionsLayout;

	QLabel* variableNamesList;

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace

#endif // __OVITO_RADIAL_COMPUTE_MODIFIER_H
