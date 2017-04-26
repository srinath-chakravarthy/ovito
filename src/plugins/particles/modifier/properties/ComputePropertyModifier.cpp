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

#include <plugins/particles/Particles.h>
#include <plugins/particles/util/CutoffNeighborFinder.h>
#include <core/animation/AnimationSettings.h>
#include <core/scene/pipeline/PipelineObject.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include "ComputePropertyModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Properties)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(ComputePropertyModifier, AsynchronousParticleModifier);
DEFINE_PROPERTY_FIELD(ComputePropertyModifier, expressions, "Expressions");
DEFINE_PROPERTY_FIELD(ComputePropertyModifier, outputProperty, "OutputProperty");
DEFINE_PROPERTY_FIELD(ComputePropertyModifier, onlySelectedParticles, "OnlySelectedParticles");
DEFINE_PROPERTY_FIELD(ComputePropertyModifier, neighborModeEnabled, "NeighborModeEnabled");
DEFINE_PROPERTY_FIELD(ComputePropertyModifier, neighborExpressions, "NeighborExpressions");
DEFINE_FLAGS_PROPERTY_FIELD(ComputePropertyModifier, cutoff, "Cutoff", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_VECTOR_REFERENCE_FIELD(ComputePropertyModifier, cachedDisplayObjects, "CachedDisplayObjects", DisplayObject, PROPERTY_FIELD_NEVER_CLONE_TARGET|PROPERTY_FIELD_NO_CHANGE_MESSAGE|PROPERTY_FIELD_NO_UNDO|PROPERTY_FIELD_NO_SUB_ANIM);
SET_PROPERTY_FIELD_LABEL(ComputePropertyModifier, expressions, "Expressions");
SET_PROPERTY_FIELD_LABEL(ComputePropertyModifier, outputProperty, "Output property");
SET_PROPERTY_FIELD_LABEL(ComputePropertyModifier, onlySelectedParticles, "Compute only for selected particles");
SET_PROPERTY_FIELD_LABEL(ComputePropertyModifier, neighborModeEnabled, "Include neighbor terms");
SET_PROPERTY_FIELD_LABEL(ComputePropertyModifier, neighborExpressions, "Neighbor expressions");
SET_PROPERTY_FIELD_LABEL(ComputePropertyModifier, cutoff, "Cutoff radius");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ComputePropertyModifier, cutoff, WorldParameterUnit, 0);

/******************************************************************************
* Constructs a new instance of this class.
******************************************************************************/
ComputePropertyModifier::ComputePropertyModifier(DataSet* dataset) : AsynchronousParticleModifier(dataset),
	_outputProperty(tr("My property")), _expressions(QStringList("0")), _onlySelectedParticles(false),
	_neighborExpressions(QStringList("0")), _cutoff(3), _neighborModeEnabled(false)
{
	INIT_PROPERTY_FIELD(expressions);
	INIT_PROPERTY_FIELD(onlySelectedParticles);
	INIT_PROPERTY_FIELD(outputProperty);
	INIT_PROPERTY_FIELD(neighborModeEnabled);
	INIT_PROPERTY_FIELD(cutoff);
	INIT_PROPERTY_FIELD(neighborExpressions);
	INIT_PROPERTY_FIELD(cachedDisplayObjects);
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void ComputePropertyModifier::loadFromStream(ObjectLoadStream& stream)
{
	// This is for backward compatibility with OVITO 2.5.1.
	// AsynchronousParticleModifier was not the base class before.
	if(stream.formatVersion() >= 20502)
		AsynchronousParticleModifier::loadFromStream(stream);
	else
		ParticleModifier::loadFromStream(stream);

	// This is also for backward compatibility with OVITO 2.5.1.
	// Make sure the number of neighbor expressions is equal to the number of central expressions.
	setPropertyComponentCount(propertyComponentCount());
}

/******************************************************************************
* Sets the number of vector components of the property to create.
******************************************************************************/
void ComputePropertyModifier::setPropertyComponentCount(int newComponentCount)
{
	if(newComponentCount < expressions().size()) {
		setExpressions(expressions().mid(0, newComponentCount));
	}
	else if(newComponentCount > expressions().size()) {
		QStringList newList = expressions();
		while(newList.size() < newComponentCount)
			newList.append("0");
		setExpressions(newList);
	}

	if(newComponentCount < neighborExpressions().size()) {
		setNeighborExpressions(neighborExpressions().mid(0, newComponentCount));
	}
	else if(newComponentCount > neighborExpressions().size()) {
		QStringList newList = neighborExpressions();
		while(newList.size() < newComponentCount)
			newList.append("0");
		setNeighborExpressions(newList);
	}
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void ComputePropertyModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	if(field == PROPERTY_FIELD(outputProperty)) {
		if(outputProperty().type() != ParticleProperty::UserProperty)
			setPropertyComponentCount(ParticleProperty::standardPropertyComponentCount(outputProperty().type()));
		else
			setPropertyComponentCount(1);
	}

	AsynchronousParticleModifier::propertyChanged(field);

	// Throw away cached results if parameters change.
	if(field == PROPERTY_FIELD(expressions) ||
			field == PROPERTY_FIELD(neighborExpressions) ||
			field == PROPERTY_FIELD(onlySelectedParticles) ||
			field == PROPERTY_FIELD(neighborModeEnabled) ||
			field == PROPERTY_FIELD(outputProperty) ||
			field == PROPERTY_FIELD(cutoff))
		invalidateCachedResults();
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a pipeline.
******************************************************************************/
void ComputePropertyModifier::initializeModifier(PipelineObject* pipeline, ModifierApplication* modApp)
{
	AsynchronousParticleModifier::initializeModifier(pipeline, modApp);

	// Generate list of available input variables.
	PipelineFlowState input = getModifierInput(modApp);
	ParticleExpressionEvaluator evaluator;
	evaluator.initialize(QStringList(), input);
	_inputVariableNames = evaluator.inputVariableNames();
	_inputVariableTable = evaluator.inputVariableTable();
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
std::shared_ptr<AsynchronousParticleModifier::ComputeEngine> ComputePropertyModifier::createEngine(TimePoint time, TimeInterval validityInterval)
{
	// Get the particle positions.
	ParticlePropertyObject* posProperty = expectStandardProperty(ParticleProperty::PositionProperty);

	// Get simulation cell.
	SimulationCellObject* inputCell = expectSimulationCell();

	// The current animation frame number.
	int currentFrame = dataset()->animationSettings()->timeToFrame(time);

	// Build list of all input particle properties, which will be passed to the compute engine.
	std::vector<QExplicitlySharedDataPointer<ParticleProperty>> inputProperties;
	for(DataObject* obj : input().objects()) {
		if(ParticlePropertyObject* prop = dynamic_object_cast<ParticlePropertyObject>(obj)) {
			inputProperties.emplace_back(prop->storage());
		}
	}

	// Get particle selection
	ParticleProperty* selProperty = nullptr;
	if(onlySelectedParticles()) {
		ParticlePropertyObject* selPropertyObj = inputStandardProperty(ParticleProperty::SelectionProperty);
		if(!selPropertyObj)
			throwException(tr("Compute modifier has been restricted to selected particles, but no particle selection is defined."));
		OVITO_ASSERT(selPropertyObj->size() == inputParticleCount());
		selProperty = selPropertyObj->storage();
	}

	// Prepare output property.
	QExplicitlySharedDataPointer<ParticleProperty> outp;
	if(outputProperty().type() != ParticleProperty::UserProperty) {
		outp = new ParticleProperty(posProperty->size(), outputProperty().type(), 0, onlySelectedParticles());
	}
	else if(!outputProperty().name().isEmpty() && propertyComponentCount() > 0) {
		outp = new ParticleProperty(posProperty->size(), qMetaTypeId<FloatType>(), propertyComponentCount(), 0, outputProperty().name(), onlySelectedParticles());
	}
	else {
		throwException(tr("Output property has not been specified."));
	}
	if(expressions().size() != outp->componentCount())
		throwException(tr("Number of expressions does not match component count of output property."));
	if(neighborModeEnabled() && neighborExpressions().size() != outp->componentCount())
		throwException(tr("Number of neighbor expressions does not match component count of output property."));

	// Initialize output property with original values when computation is restricted to selected particles.
	if(onlySelectedParticles()) {
		ParticlePropertyObject* originalPropertyObj = nullptr;
		if(outputProperty().type() != ParticleProperty::UserProperty) {
			originalPropertyObj = inputStandardProperty(outputProperty().type());
		}
		else {
			for(DataObject* o : input().objects()) {
				ParticlePropertyObject* property = dynamic_object_cast<ParticlePropertyObject>(o);
				if(property && property->type() == ParticleProperty::UserProperty && property->name() == outp->name()) {
					originalPropertyObj = property;
					break;
				}
			}

		}
		if(originalPropertyObj && originalPropertyObj->dataType() == outp->dataType() &&
				originalPropertyObj->componentCount() == outp->componentCount() && originalPropertyObj->stride() == outp->stride()) {
			memcpy(outp->data(), originalPropertyObj->constData(), outp->stride() * outp->size());
		}
		else if(outputProperty().type() == ParticleProperty::ColorProperty) {
			std::vector<Color> colors = inputParticleColors(time, validityInterval);
			OVITO_ASSERT(outp->stride() == sizeof(Color) && outp->size() == colors.size());
			memcpy(outp->data(), colors.data(), outp->stride() * outp->size());
		}
		else if(outputProperty().type() == ParticleProperty::RadiusProperty) {
			std::vector<FloatType> radii = inputParticleRadii(time, validityInterval);
			OVITO_ASSERT(outp->stride() == sizeof(FloatType) && outp->size() == radii.size());
			memcpy(outp->data(), radii.data(), outp->stride() * outp->size());
		}
	}

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<PropertyComputeEngine>(validityInterval, time, outp.data(), posProperty->storage(),
			selProperty, inputCell->data(), neighborModeEnabled() ? cutoff() : 0.0f,
			expressions(), neighborExpressions(),
			std::move(inputProperties), currentFrame, input().attributes());
}

/******************************************************************************
* This is called by the constructor to prepare the compute engine.
******************************************************************************/
void ComputePropertyModifier::PropertyComputeEngine::initializeEngine(TimePoint time)
{
	OVITO_ASSERT(_expressions.size() == outputProperty()->componentCount());

	// Make a copy of the list of input properties.
	std::vector<ParticleProperty*> inputProperties;
	for(const auto& p : _inputProperties)
		inputProperties.push_back(p.data());

	// Initialize expression evaluators.
	_evaluator.initialize(_expressions, inputProperties, &cell(), _attributes, _frameNumber);
	_inputVariableNames = _evaluator.inputVariableNames();
	_inputVariableTable = _evaluator.inputVariableTable();

	// Only used when neighbor mode is active.
	if(neighborMode()) {
		_evaluator.registerGlobalParameter("Cutoff", _cutoff);
		_evaluator.registerGlobalParameter("NumNeighbors", 0);
		OVITO_ASSERT(_neighborExpressions.size() == outputProperty()->componentCount());
		_neighborEvaluator.initialize(_neighborExpressions, inputProperties, &cell(), _attributes, _frameNumber);
		_neighborEvaluator.registerGlobalParameter("Cutoff", _cutoff);
		_neighborEvaluator.registerGlobalParameter("NumNeighbors", 0);
		_neighborEvaluator.registerGlobalParameter("Distance", 0);
		_neighborEvaluator.registerGlobalParameter("Delta.X", 0);
		_neighborEvaluator.registerGlobalParameter("Delta.Y", 0);
		_neighborEvaluator.registerGlobalParameter("Delta.Z", 0);
	}

	// Determine if math expressions are time-dependent, i.e. if they reference the animation
	// frame number. If yes, then we have to restrict the validity interval of the computation
	// to the current time.
	bool isTimeDependent = false;
	ParticleExpressionEvaluator::Worker worker(_evaluator);
	if(worker.isVariableUsed("Frame") || worker.isVariableUsed("Timestep"))
		isTimeDependent = true;
	else if(neighborMode()) {
		ParticleExpressionEvaluator::Worker worker(_neighborEvaluator);
		if(worker.isVariableUsed("Frame") || worker.isVariableUsed("Timestep"))
			isTimeDependent = true;
	}
	if(isTimeDependent) {
		TimeInterval iv = validityInterval();
		iv.intersect(time);
		setValidityInterval(iv);
	}
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void ComputePropertyModifier::PropertyComputeEngine::perform()
{
	setProgressText(tr("Computing particle property '%1'").arg(outputProperty()->name()));

	// Only used when neighbor mode is active.
	CutoffNeighborFinder neighborFinder;
	if(neighborMode()) {
		// Prepare the neighbor list.
		if(!neighborFinder.prepare(_cutoff, positions(), cell(), nullptr, *this))
			return;
	}

	setProgressValue(0);
	setProgressMaximum(positions()->size());

	// Parallelized loop over all particles.
	parallelForChunks(positions()->size(), *this, [this, &neighborFinder](size_t startIndex, size_t count, PromiseBase& promise) {
		ParticleExpressionEvaluator::Worker worker(_evaluator);
		ParticleExpressionEvaluator::Worker neighborWorker(_neighborEvaluator);

		double* distanceVar;
		double* deltaX;
		double* deltaY;
		double* deltaZ;
		double* selfNumNeighbors = nullptr;
		double* neighNumNeighbors = nullptr;

		if(neighborMode()) {
			distanceVar = neighborWorker.variableAddress("Distance");
			deltaX = neighborWorker.variableAddress("Delta.X");
			deltaY = neighborWorker.variableAddress("Delta.Y");
			deltaZ = neighborWorker.variableAddress("Delta.Z");
			selfNumNeighbors = worker.variableAddress("NumNeighbors");
			neighNumNeighbors = neighborWorker.variableAddress("NumNeighbors");
			if(!worker.isVariableUsed("NumNeighbors") && !neighborWorker.isVariableUsed("NumNeighbors"))
				selfNumNeighbors = neighNumNeighbors = nullptr;
		}

		size_t endIndex = startIndex + count;
		size_t componentCount = outputProperty()->componentCount();
		for(size_t particleIndex = startIndex; particleIndex < endIndex; particleIndex++) {

			// Update progress indicator.
			if((particleIndex % 1024) == 0)
				promise.incrementProgressValue(1024);

			// Stop loop if canceled.
			if(promise.isCanceled())
				return;

			// Skip unselected particles if requested.
			if(selection() && !selection()->getInt(particleIndex))
				continue;

			if(selfNumNeighbors != nullptr) {
				// Determine number of neighbors.
				int nneigh = 0;
				for(CutoffNeighborFinder::Query neighQuery(neighborFinder, particleIndex); !neighQuery.atEnd(); neighQuery.next())
					nneigh++;
				*selfNumNeighbors = *neighNumNeighbors = nneigh;
			}

			for(size_t component = 0; component < componentCount; component++) {

				// Compute self term.
				FloatType value = worker.evaluate(particleIndex, component);

				if(neighborMode()) {
					// Compute sum of neighbor terms.
					for(CutoffNeighborFinder::Query neighQuery(neighborFinder, particleIndex); !neighQuery.atEnd(); neighQuery.next()) {
						*distanceVar = sqrt(neighQuery.distanceSquared());
						*deltaX = neighQuery.delta().x();
						*deltaY = neighQuery.delta().y();
						*deltaZ = neighQuery.delta().z();
						value += neighborWorker.evaluate(neighQuery.current(), component);
					}
				}

				// Store results.
				if(outputProperty()->dataType() == QMetaType::Int) {
					outputProperty()->setIntComponent(particleIndex, component, (int)value);
				}
				else {
					outputProperty()->setFloatComponent(particleIndex, component, value);
				}
			}
		}
	});
}

/******************************************************************************
* Unpacks the results of the computation engine and stores them in the modifier.
******************************************************************************/
void ComputePropertyModifier::transferComputationResults(ComputeEngine* engine)
{
	PropertyComputeEngine* eng = static_cast<PropertyComputeEngine*>(engine);
	_computedProperty = eng->outputProperty();
	_inputVariableNames = eng->inputVariableNames();
	_inputVariableTable = eng->inputVariableTable();
}

/******************************************************************************
* Lets the modifier insert the cached computation results into the
* modification pipeline.
******************************************************************************/
PipelineStatus ComputePropertyModifier::applyComputationResults(TimePoint time, TimeInterval& validityInterval)
{
	if(!_computedProperty)
		throwException(tr("No computation results available."));

	if(outputParticleCount() != _computedProperty->size())
		throwException(tr("The number of input particles has changed. The stored results have become invalid."));

	ParticlePropertyObject* outputPropertyObj;
	if(_computedProperty->type() == ParticleProperty::UserProperty)
		outputPropertyObj = outputCustomProperty(_computedProperty.data());
	else
		outputPropertyObj = outputStandardProperty(_computedProperty.data());

	// Replace display objects of output property with cached ones and cache any new display objects.
	// This is required to avoid losing the output property's display settings
	// each time the modifier is re-evaluated or when serializing the modifier.
	if(outputPropertyObj) {
		QVector<DisplayObject*> currentDisplayObjs = outputPropertyObj->displayObjects();
		// Replace with cached display objects if they are of the same class type.
		for(int i = 0; i < currentDisplayObjs.size() && i < _cachedDisplayObjects.size(); i++) {
			if(currentDisplayObjs[i]->getOOType() == _cachedDisplayObjects[i]->getOOType()) {
				currentDisplayObjs[i] = _cachedDisplayObjects[i];
			}
		}
		outputPropertyObj->setDisplayObjects(currentDisplayObjs);
		_cachedDisplayObjects = currentDisplayObjs;
	}

	return PipelineStatus::Success;
}

/******************************************************************************
* Allows the object to parse the serialized contents of a property field in a custom way.
******************************************************************************/
bool ComputePropertyModifier::loadPropertyFieldFromStream(ObjectLoadStream& stream, const ObjectLoadStream::SerializedPropertyField& serializedField)
{
	// This is to maintain compatibility with old file format.
	if(serializedField.identifier == "PropertyName") {
		QString propertyName;
		stream >> propertyName;
		setOutputProperty(ParticlePropertyReference(outputProperty().type(), propertyName));
		return true;
	}
	else if(serializedField.identifier == "PropertyType") {
		int propertyType;
		stream >> propertyType;
		setOutputProperty(ParticlePropertyReference((ParticleProperty::Type)propertyType, outputProperty().name()));
		return true;
	}
	return AsynchronousParticleModifier::loadPropertyFieldFromStream(stream, serializedField);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
