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
#include <plugins/particles/util/ParticlePropertyParameterUI.h>
#include <plugins/particles/util/CutoffNeighborFinder.h>
#include <gui/properties/BooleanGroupBoxParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/properties/StringParameterUI.h>
#include <gui/properties/VariantComboBoxParameterUI.h>
#include <core/animation/AnimationSettings.h>
#include <core/scene/pipeline/PipelineObject.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include "ComputePropertyModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Properties)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(Particles, ComputePropertyModifier, AsynchronousParticleModifier);
SET_OVITO_OBJECT_EDITOR(ComputePropertyModifier, ComputePropertyModifierEditor);
DEFINE_PROPERTY_FIELD(ComputePropertyModifier, _expressions, "Expressions");
DEFINE_PROPERTY_FIELD(ComputePropertyModifier, _outputProperty, "OutputProperty");
DEFINE_PROPERTY_FIELD(ComputePropertyModifier, _onlySelectedParticles, "OnlySelectedParticles");
DEFINE_PROPERTY_FIELD(ComputePropertyModifier, _neighborModeEnabled, "NeighborModeEnabled");
DEFINE_PROPERTY_FIELD(ComputePropertyModifier, _neighborExpressions, "NeighborExpressions");
DEFINE_FLAGS_PROPERTY_FIELD(ComputePropertyModifier, _cutoff, "Cutoff", PROPERTY_FIELD_MEMORIZE);
SET_PROPERTY_FIELD_LABEL(ComputePropertyModifier, _expressions, "Expressions");
SET_PROPERTY_FIELD_LABEL(ComputePropertyModifier, _outputProperty, "Output property");
SET_PROPERTY_FIELD_LABEL(ComputePropertyModifier, _onlySelectedParticles, "Compute only for selected particles");
SET_PROPERTY_FIELD_LABEL(ComputePropertyModifier, _neighborModeEnabled, "Include neighbor terms");
SET_PROPERTY_FIELD_LABEL(ComputePropertyModifier, _neighborExpressions, "Neighbor expressions");
SET_PROPERTY_FIELD_LABEL(ComputePropertyModifier, _cutoff, "Cutoff radius");
SET_PROPERTY_FIELD_UNITS(ComputePropertyModifier, _cutoff, WorldParameterUnit);

OVITO_BEGIN_INLINE_NAMESPACE(Internal)
	IMPLEMENT_OVITO_OBJECT(Particles, ComputePropertyModifierEditor, ParticleModifierEditor);
OVITO_END_INLINE_NAMESPACE

/******************************************************************************
* Constructs a new instance of this class.
******************************************************************************/
ComputePropertyModifier::ComputePropertyModifier(DataSet* dataset) : AsynchronousParticleModifier(dataset),
	_outputProperty(tr("My property")), _expressions(QStringList("0")), _onlySelectedParticles(false),
	_neighborExpressions(QStringList("0")), _cutoff(3), _neighborModeEnabled(false)
{
	INIT_PROPERTY_FIELD(ComputePropertyModifier::_expressions);
	INIT_PROPERTY_FIELD(ComputePropertyModifier::_onlySelectedParticles);
	INIT_PROPERTY_FIELD(ComputePropertyModifier::_outputProperty);
	INIT_PROPERTY_FIELD(ComputePropertyModifier::_neighborModeEnabled);
	INIT_PROPERTY_FIELD(ComputePropertyModifier::_cutoff);
	INIT_PROPERTY_FIELD(ComputePropertyModifier::_neighborExpressions);
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
	if(field == PROPERTY_FIELD(ComputePropertyModifier::_outputProperty)) {
		if(outputProperty().type() != ParticleProperty::UserProperty)
			setPropertyComponentCount(ParticleProperty::standardPropertyComponentCount(outputProperty().type()));
		else
			setPropertyComponentCount(1);
	}

	AsynchronousParticleModifier::propertyChanged(field);

	// Throw away cached results if parameters change.
	if(field == PROPERTY_FIELD(ComputePropertyModifier::_expressions) ||
			field == PROPERTY_FIELD(ComputePropertyModifier::_neighborExpressions) ||
			field == PROPERTY_FIELD(ComputePropertyModifier::_onlySelectedParticles) ||
			field == PROPERTY_FIELD(ComputePropertyModifier::_neighborModeEnabled) ||
			field == PROPERTY_FIELD(ComputePropertyModifier::_outputProperty) ||
			field == PROPERTY_FIELD(ComputePropertyModifier::_cutoff))
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
	PipelineFlowState input = pipeline->evaluatePipeline(dataset()->animationSettings()->time(), modApp, false);
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

	// Get simulation timestep.
	int simulationTimestep = -1;
	if(input().attributes().contains(QStringLiteral("Timestep"))) {
		simulationTimestep = input().attributes().value(QStringLiteral("Timestep")).toInt();
	}

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
			std::move(inputProperties), currentFrame, simulationTimestep);
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
	_evaluator.initialize(_expressions, inputProperties, &cell(), _frameNumber, _simulationTimestep);
	_inputVariableNames = _evaluator.inputVariableNames();
	_inputVariableTable = _evaluator.inputVariableTable();

	// Only used when neighbor mode is active.
	if(neighborMode()) {
		_evaluator.registerGlobalParameter("Cutoff", _cutoff);
		_evaluator.registerGlobalParameter("NumNeighbors", 0);
		OVITO_ASSERT(_neighborExpressions.size() == outputProperty()->componentCount());
		_neighborEvaluator.initialize(_neighborExpressions, inputProperties, &cell(), _frameNumber, _simulationTimestep);
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
		if(!neighborFinder.prepare(_cutoff, positions(), cell(), nullptr, this))
			return;
	}

	// Parallelized loop over all particles.
	setProgressRange(positions()->size());
	setProgressValue(0);
	parallelForChunks(positions()->size(), *this, [this, &neighborFinder](size_t startIndex, size_t count, FutureInterfaceBase& futureInterface) {
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
				futureInterface.incrementProgressValue(1024);

			// Stop loop if canceled.
			if(futureInterface.isCanceled())
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

	if(_computedProperty->type() == ParticleProperty::UserProperty)
		outputCustomProperty(_computedProperty.data());
	else
		outputStandardProperty(_computedProperty.data());

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

OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void ComputePropertyModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	rollout = createRollout(tr("Compute property"), rolloutParams, "particles.modifiers.compute_property.html");

    // Create the rollout contents.
	QVBoxLayout* mainLayout = new QVBoxLayout(rollout);
	mainLayout->setContentsMargins(4,4,4,4);

	QGroupBox* propertiesGroupBox = new QGroupBox(tr("Output property"), rollout);
	mainLayout->addWidget(propertiesGroupBox);
	QVBoxLayout* propertiesLayout = new QVBoxLayout(propertiesGroupBox);
	propertiesLayout->setContentsMargins(6,6,6,6);
	propertiesLayout->setSpacing(4);

	// Output property
	ParticlePropertyParameterUI* outputPropertyUI = new ParticlePropertyParameterUI(this, PROPERTY_FIELD(ComputePropertyModifier::_outputProperty), false, false);
	propertiesLayout->addWidget(outputPropertyUI->comboBox());

	// Create the check box for the selection flag.
	BooleanParameterUI* selectionFlagUI = new BooleanParameterUI(this, PROPERTY_FIELD(ComputePropertyModifier::_onlySelectedParticles));
	propertiesLayout->addWidget(selectionFlagUI->checkBox());

	expressionsGroupBox = new QGroupBox(tr("Expression"));
	mainLayout->addWidget(expressionsGroupBox);
	expressionsLayout = new QGridLayout(expressionsGroupBox);
	expressionsLayout->setContentsMargins(4,4,4,4);
	expressionsLayout->setSpacing(1);
	expressionsLayout->setColumnStretch(1,1);

	// Status label.
	mainLayout->addWidget(statusLabel());

    // Neighbor mode panel.
	QWidget* neighorRollout = createRollout(tr("Neighbor particles"), rolloutParams.after(rollout), "particles.modifiers.compute_property.html");

	mainLayout = new QVBoxLayout(neighorRollout);
	mainLayout->setContentsMargins(4,4,4,4);

	BooleanGroupBoxParameterUI* neighborModeUI = new BooleanGroupBoxParameterUI(this, PROPERTY_FIELD(ComputePropertyModifier::_neighborModeEnabled));
	mainLayout->addWidget(neighborModeUI->groupBox());

	QGridLayout* gridlayout = new QGridLayout(neighborModeUI->childContainer());
	gridlayout->setContentsMargins(4,4,4,4);
	gridlayout->setColumnStretch(1, 1);
	gridlayout->setRowStretch(1, 1);

	// Cutoff parameter.
	FloatParameterUI* cutoffRadiusUI = new FloatParameterUI(this, PROPERTY_FIELD(ComputePropertyModifier::_cutoff));
	gridlayout->addWidget(cutoffRadiusUI->label(), 0, 0);
	gridlayout->addLayout(cutoffRadiusUI->createFieldLayout(), 0, 1);
	cutoffRadiusUI->setMinValue(0);

	neighborExpressionsGroupBox = new QGroupBox(tr("Neighbor expression"));
	gridlayout->addWidget(neighborExpressionsGroupBox, 1, 0, 1, 2);
	neighborExpressionsLayout = new QGridLayout(neighborExpressionsGroupBox);
	neighborExpressionsLayout->setContentsMargins(4,4,4,4);
	neighborExpressionsLayout->setSpacing(1);
	neighborExpressionsLayout->setColumnStretch(1,1);

	QWidget* variablesRollout = createRollout(tr("Variables"), rolloutParams.after(neighorRollout), "particles.modifiers.compute_property.html");
    QVBoxLayout* variablesLayout = new QVBoxLayout(variablesRollout);
    variablesLayout->setContentsMargins(4,4,4,4);
    variableNamesDisplay = new QLabel();
	variableNamesDisplay->setWordWrap(true);
	variableNamesDisplay->setTextInteractionFlags(Qt::TextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard | Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard));
	variablesLayout->addWidget(variableNamesDisplay);

	// Update input variables list if another modifier has been loaded into the editor.
	connect(this, &ComputePropertyModifierEditor::contentsReplaced, this, &ComputePropertyModifierEditor::updateEditorFields);
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool ComputePropertyModifierEditor::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	if(source == editObject() && (event->type() == ReferenceEvent::TargetChanged || event->type() == ReferenceEvent::ObjectStatusChanged)) {
		if(!editorUpdatePending) {
			editorUpdatePending = true;
			QMetaObject::invokeMethod(this, "updateEditorFields", Qt::QueuedConnection, Q_ARG(bool, event->type() == ReferenceEvent::TargetChanged));
		}
	}
	return ParticleModifierEditor::referenceEvent(source, event);
}

/******************************************************************************
* Updates the enabled/disabled status of the editor's controls.
******************************************************************************/
void ComputePropertyModifierEditor::updateEditorFields(bool updateExpressions)
{
	editorUpdatePending = false;

	ComputePropertyModifier* mod = static_object_cast<ComputePropertyModifier>(editObject());
	if(!mod) return;

	const QStringList& expr = mod->expressions();
	expressionsGroupBox->setTitle((expr.size() <= 1) ? tr("Expression") : tr("Expressions"));
	while(expr.size() > expressionBoxes.size()) {
		QLabel* label = new QLabel();
		AutocompleteLineEdit* edit = new AutocompleteLineEdit();
		expressionsLayout->addWidget(label, expressionBoxes.size(), 0);
		expressionsLayout->addWidget(edit, expressionBoxes.size(), 1);
		expressionBoxes.push_back(edit);
		expressionBoxLabels.push_back(label);
		connect(edit, &AutocompleteLineEdit::editingFinished, this, &ComputePropertyModifierEditor::onExpressionEditingFinished);
	}
	while(expr.size() < expressionBoxes.size()) {
		delete expressionBoxes.takeLast();
		delete expressionBoxLabels.takeLast();
	}
	OVITO_ASSERT(expressionBoxes.size() == expr.size());
	OVITO_ASSERT(expressionBoxLabels.size() == expr.size());

	const QStringList& neighExpr = mod->neighborExpressions();
	neighborExpressionsGroupBox->setTitle((neighExpr.size() <= 1) ? tr("Neighbor expression") : tr("Neighbor expressions"));
	while(neighExpr.size() > neighborExpressionBoxes.size()) {
		QLabel* label = new QLabel();
		AutocompleteLineEdit* edit = new AutocompleteLineEdit();
		neighborExpressionsLayout->addWidget(label, neighborExpressionBoxes.size(), 0);
		neighborExpressionsLayout->addWidget(edit, neighborExpressionBoxes.size(), 1);
		neighborExpressionBoxes.push_back(edit);
		neighborExpressionBoxLabels.push_back(label);
		connect(edit, &AutocompleteLineEdit::editingFinished, this, &ComputePropertyModifierEditor::onExpressionEditingFinished);
	}
	while(neighExpr.size() < neighborExpressionBoxes.size()) {
		delete neighborExpressionBoxes.takeLast();
		delete neighborExpressionBoxLabels.takeLast();
	}
	OVITO_ASSERT(neighborExpressionBoxes.size() == neighExpr.size());
	OVITO_ASSERT(neighborExpressionBoxLabels.size() == neighExpr.size());

	QStringList standardPropertyComponentNames;
	if(mod->outputProperty().type() != ParticleProperty::UserProperty) {
		standardPropertyComponentNames = ParticleProperty::standardPropertyComponentNames(mod->outputProperty().type());
	}

	QStringList inputVariableNames = mod->inputVariableNames();
	if(mod->neighborModeEnabled()) {
		inputVariableNames.append(QStringLiteral("Cutoff"));
		inputVariableNames.append(QStringLiteral("NumNeighbors"));
	}
	for(int i = 0; i < expr.size(); i++) {
		if(updateExpressions)
			expressionBoxes[i]->setText(expr[i]);
		expressionBoxes[i]->setWordList(inputVariableNames);
		if(expr.size() == 1)
			expressionBoxLabels[i]->hide();
		else {
			if(i < standardPropertyComponentNames.size())
				expressionBoxLabels[i]->setText(tr("%1:").arg(standardPropertyComponentNames[i]));
			else
				expressionBoxLabels[i]->setText(tr("%1:").arg(i+1));
			expressionBoxLabels[i]->show();
		}
	}
	if(mod->neighborModeEnabled()) {
		inputVariableNames.append(QStringLiteral("Distance"));
		inputVariableNames.append(QStringLiteral("Delta.X"));
		inputVariableNames.append(QStringLiteral("Delta.Y"));
		inputVariableNames.append(QStringLiteral("Delta.Z"));
	}
	for(int i = 0; i < neighExpr.size(); i++) {
		if(updateExpressions)
			neighborExpressionBoxes[i]->setText(neighExpr[i]);
		neighborExpressionBoxes[i]->setWordList(inputVariableNames);
		if(expr.size() == 1)
			neighborExpressionBoxLabels[i]->hide();
		else {
			if(i < standardPropertyComponentNames.size())
				neighborExpressionBoxLabels[i]->setText(tr("%1:").arg(standardPropertyComponentNames[i]));
			else
				neighborExpressionBoxLabels[i]->setText(tr("%1:").arg(i+1));
			neighborExpressionBoxLabels[i]->show();
		}
	}

	QString variableList = mod->inputVariableTable();
	if(mod->neighborModeEnabled()) {
		variableList.append(QStringLiteral("<p><b>Neighbor parameters:</b><ul>"));
		variableList.append(QStringLiteral("<li>Cutoff (<i style=\"color: #555;\">radius</i>)</li>"));
		variableList.append(QStringLiteral("<li>NumNeighbors (<i style=\"color: #555;\">of central particle</i>)</li>"));
		variableList.append(QStringLiteral("<li>Distance (<i style=\"color: #555;\">from central particle</i>)</li>"));
		variableList.append(QStringLiteral("<li>Delta.X (<i style=\"color: #555;\">neighbor vector</i>)</li>"));
		variableList.append(QStringLiteral("<li>Delta.Y (<i style=\"color: #555;\">neighbor vector</i>)</li>"));
		variableList.append(QStringLiteral("<li>Delta.X (<i style=\"color: #555;\">neighbor vector</i>)</li>"));
		variableList.append(QStringLiteral("</ul></p>"));
	}
	variableList.append(QStringLiteral("<p></p>"));
	variableNamesDisplay->setText(variableList);

	neighborExpressionsGroupBox->updateGeometry();
	container()->updateRolloutsLater();
}

/******************************************************************************
* Is called when the user has typed in an expression.
******************************************************************************/
void ComputePropertyModifierEditor::onExpressionEditingFinished()
{
	ComputePropertyModifier* mod = static_object_cast<ComputePropertyModifier>(editObject());
	AutocompleteLineEdit* edit = (AutocompleteLineEdit*)sender();
	int index = expressionBoxes.indexOf(edit);
	if(index >= 0) {
		undoableTransaction(tr("Change expression"), [mod, edit, index]() {
			QStringList expr = mod->expressions();
			expr[index] = edit->text();
			mod->setExpressions(expr);
		});
	}
	else {
		int index = neighborExpressionBoxes.indexOf(edit);
		OVITO_ASSERT(index >= 0);
		undoableTransaction(tr("Change neighbor function"), [mod, edit, index]() {
			QStringList expr = mod->neighborExpressions();
			expr[index] = edit->text();
			mod->setNeighborExpressions(expr);
		});
	}
}

OVITO_END_INLINE_NAMESPACE

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
