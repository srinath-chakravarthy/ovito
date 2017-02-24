///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2014) Alexander Stukowski
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
#include <core/scene/pipeline/PipelineFlowState.h>

#include <muParser.h>
#include <boost/utility.hpp>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * \brief Helper class that evaluates one or more math expressions for every particle.
 *
 * This class is used by the ComputePropertyModifier and the SelectExpressionModifier.
 */
class OVITO_PARTICLES_EXPORT ParticleExpressionEvaluator
{
	Q_DECLARE_TR_FUNCTIONS(ParticleExpressionEvaluator);

public:

	/// \brief Constructor.
	ParticleExpressionEvaluator() : _particleCount(0), _isTimeDependent(false) {}

	/// Specifies the expressions to be evaluated for each particle and creates the input variables.
	void initialize(const QStringList& expressions, const PipelineFlowState& inputState, int animationFrame = 0);

	/// Specifies the expressions to be evaluated for each particle and creates the input variables.
	void initialize(const QStringList& expressions, const std::vector<ParticleProperty*>& inputProperties, const SimulationCell* simCell, const QVariantMap& attributes, int animationFrame = 0);

	/// Initializes the parser object and evaluates the expressions for every particle.
	void evaluate(const std::function<void(size_t,size_t,double)>& callback, const std::function<bool(size_t)>& filter = std::function<bool(size_t)>());

	/// Returns the list of expressions.
	const std::vector<std::string>& expression() const { return _expressions; }

	/// Returns the list of available input variables.
	QStringList inputVariableNames() const;

	/// Returns a human-readable text listing the input variables.
	QString inputVariableTable() const;

	/// Returns whether the expression results depend on animation time.
	bool isTimeDependent() const { return _isTimeDependent; }

	/// Registers a new input variable whose value is recomputed for each particle.
	template<typename Function>
	void registerComputedVariable(const QString& variableName, Function&& function, QString description = QString()) {
		ExpressionVariable v;
		v.type = DERIVED_PARTICLE_PROPERTY;
		v.name = variableName.toStdString();
		v.function = std::forward<Function>(function);
		v.description = description;
		addVariable(std::move(v));
	}

	/// Registers a new input variable whose value is recomputed for each particle.
	void registerGlobalParameter(const QString& variableName, double value, QString description = QString()) {
		ExpressionVariable v;
		v.type = GLOBAL_PARAMETER;
		v.name = variableName.toStdString();
		v.value = value;
		v.description = description;
		addVariable(std::move(v));
	}

	/// Registers a new input variable whose value is recomputed for each particle.
	void registerConstant(const QString& variableName, double value, QString description = QString()) {
		ExpressionVariable v;
		v.type = CONSTANT;
		v.name = variableName.toStdString();
		v.value = value;
		v.description = description;
		addVariable(std::move(v));
	}

protected:

	enum ExpressionVariableType {
		PARTICLE_FLOAT_PROPERTY,
		PARTICLE_INT_PROPERTY,
		DERIVED_PARTICLE_PROPERTY,
		PARTICLE_INDEX,
		GLOBAL_PARAMETER,
		CONSTANT
	};

	/// Data structure representing an input variable.
	struct ExpressionVariable {
		/// The variable's value for the current particle.
		double value;
		/// Pointer into the particle property storage.
		const char* dataPointer;
		/// Data array stride in the property storage.
		size_t stride;
		/// The type of variable.
		ExpressionVariableType type;
		/// The name of the variable.
		std::string name;
		/// Human-readable description.
		QString description;
		/// A function that computes the variable's value for each particle.
		std::function<double(size_t)> function;
		/// Reference the origin particle property that contains the data.
		QExplicitlySharedDataPointer<ParticleProperty> particleProperty;
	};

public:

	/// One instance of this class is created per thread.
	class Worker : boost::noncopyable {
	public:

		/// Initializes the worker instance.
		Worker(ParticleExpressionEvaluator& evaluator);

		/// Evaluates the expression for a specific particle and a specific vector component.
		double evaluate(size_t particleIndex, size_t component);

		/// Returns the storage address of a variable value.
		double* variableAddress(const char* varName) {
			for(ExpressionVariable& var : _inputVariables)
				if(var.name == varName)
					return &var.value;
			OVITO_ASSERT(false);
			return nullptr;
		}

		// Returns whether the given variable is being referenced in one of the expressions.
		bool isVariableUsed(const char* varName) const {
			for(const ExpressionVariable* var : _activeVariables)
				if(var->name == varName) return true;
			return false;
		}

	private:

		/// The worker routine.
		void run(size_t startIndex, size_t endIndex, std::function<void(size_t,size_t,double)> callback, std::function<bool(size_t)> filter);

		/// List of parser objects used by this thread.
		std::vector<mu::Parser> _parsers;

		/// List of input variables used by the parsers of this thread.
		QVector<ExpressionVariable> _inputVariables;

		/// List of input variables which are actually used in the expression.
		std::vector<ExpressionVariable*> _activeVariables;

		/// The index of the last particle for which the expressions were evaluated.
		size_t _lastParticleIndex;

		/// Error message reported by one of the parser objects (remains empty on success).
		QString _errorMsg;

		friend class ParticleExpressionEvaluator;
	};

protected:

	/// Initializes the list of input variables from the given input state.
	void createInputVariables(const std::vector<ParticleProperty*>& inputProperties, const SimulationCell* simCell, const QVariantMap& attributes, int animationFrame);

	/// Registers an input variable if the name does not exist yet.
	void addVariable(ExpressionVariable&& v);

	/// The list of expression that should be evaluated for each particle.
	std::vector<std::string> _expressions;

	/// The list of input variables.
	QVector<ExpressionVariable> _inputVariables;

	/// Indicates that the expression produces time-dependent results.
	std::atomic<bool> _isTimeDependent;

	/// The number of input particles.
	size_t _particleCount;

	/// List of characters allowed in variable names.
	static QByteArray _validVariableNameChars;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


