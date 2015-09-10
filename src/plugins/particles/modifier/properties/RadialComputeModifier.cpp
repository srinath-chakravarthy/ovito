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

#include <plugins/particles/Particles.h>
#include <plugins/particles/util/ParticleExpressionEvaluator.h>
#include <plugins/particles/util/ParticlePropertyParameterUI.h>
#include <plugins/particles/util/CutoffNeighborFinder.h>
#include <core/gui/widgets/general/AutocompleteLineEdit.h>
#include <core/animation/AnimationSettings.h>
#include <core/scene/pipeline/PipelineObject.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include "RadialComputeModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Properties)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(Particles, RadialComputeModifier, ParticleModifier);
SET_OVITO_OBJECT_EDITOR(RadialComputeModifier, RadialComputeModifierEditor);
DEFINE_PROPERTY_FIELD(RadialComputeModifier, _selfExpressions, "SelfExpressions");
DEFINE_PROPERTY_FIELD(RadialComputeModifier, _neighborExpressions, "NeighborExpressions");
DEFINE_PROPERTY_FIELD(RadialComputeModifier, _outputProperty, "OutputProperty");
DEFINE_FLAGS_PROPERTY_FIELD(RadialComputeModifier, _cutoff, "Cutoff", PROPERTY_FIELD_MEMORIZE);
SET_PROPERTY_FIELD_LABEL(RadialComputeModifier, _selfExpressions, "Self-term expressions");
SET_PROPERTY_FIELD_LABEL(RadialComputeModifier, _neighborExpressions, "Neighbor-term expressions");
SET_PROPERTY_FIELD_LABEL(RadialComputeModifier, _outputProperty, "Output property");
SET_PROPERTY_FIELD_LABEL(RadialComputeModifier, _cutoff, "Cutoff radius");
SET_PROPERTY_FIELD_UNITS(RadialComputeModifier, _cutoff, WorldParameterUnit);

OVITO_BEGIN_INLINE_NAMESPACE(Internal)
	IMPLEMENT_OVITO_OBJECT(Particles, RadialComputeModifierEditor, ParticleModifierEditor);
OVITO_END_INLINE_NAMESPACE

/******************************************************************************
* Constructs a new instance of this class.
******************************************************************************/
RadialComputeModifier::RadialComputeModifier(DataSet* dataset) : AsynchronousParticleModifier(dataset),
	_outputProperty(tr("Custom property")), _selfExpressions(QStringList("0")), _neighborExpressions(QStringList("0")), _cutoff(3)
{
	INIT_PROPERTY_FIELD(RadialComputeModifier::_selfExpressions);
	INIT_PROPERTY_FIELD(RadialComputeModifier::_neighborExpressions);
	INIT_PROPERTY_FIELD(RadialComputeModifier::_outputProperty);
	INIT_PROPERTY_FIELD(RadialComputeModifier::_cutoff);
}

/******************************************************************************
* Sets the number of vector components of the property to create.
******************************************************************************/
void RadialComputeModifier::setPropertyComponentCount(int newComponentCount)
{
	if(newComponentCount == propertyComponentCount()) return;

	if(newComponentCount < propertyComponentCount()) {
		setSelfExpressions(selfExpressions().mid(0, newComponentCount));
		setNeighborExpressions(neighborExpressions().mid(0, newComponentCount));
	}
	else {
		QStringList newList = selfExpressions();
		while(newList.size() < newComponentCount)
			newList.append("0");
		setSelfExpressions(newList);

		newList = neighborExpressions();
		while(newList.size() < newComponentCount)
			newList.append("0");
		setNeighborExpressions(newList);
	}
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void RadialComputeModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	if(field == PROPERTY_FIELD(RadialComputeModifier::_outputProperty)) {
		if(outputProperty().type() != ParticleProperty::UserProperty)
			setPropertyComponentCount(ParticleProperty::standardPropertyComponentCount(outputProperty().type()));
		else
			setPropertyComponentCount(1);
	}

	AsynchronousParticleModifier::propertyChanged(field);

	// Throw away cached results if parameters change.
	if(field == PROPERTY_FIELD(RadialComputeModifier::_selfExpressions) ||
			field == PROPERTY_FIELD(RadialComputeModifier::_neighborExpressions) ||
			field == PROPERTY_FIELD(RadialComputeModifier::_outputProperty) ||
			field == PROPERTY_FIELD(RadialComputeModifier::_cutoff))
		invalidateCachedResults();
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a pipeline.
******************************************************************************/
void RadialComputeModifier::initializeModifier(PipelineObject* pipeline, ModifierApplication* modApp)
{
	AsynchronousParticleModifier::initializeModifier(pipeline, modApp);

	// Generate list of available input variables.
	PipelineFlowState input = pipeline->evaluatePipeline(dataset()->animationSettings()->time(), modApp, false);
	ParticleExpressionEvaluator evaluator;
	evaluator.initialize(QStringList(), input);
	evaluator.registerGlobalParameter("Cutoff", _cutoff, tr("radius"));
	evaluator.registerGlobalParameter("NumNeighbors", 0, tr("of central particle"));
	evaluator.registerGlobalParameter("Distance", 0, tr("from central particle"));
	evaluator.registerGlobalParameter("Delta.X", 0, tr("neighbor vector"));
	evaluator.registerGlobalParameter("Delta.Y", 0, tr("neighbor vector"));
	evaluator.registerGlobalParameter("Delta.Z", 0, tr("neighbor vector"));
	_inputVariableNames = evaluator.inputVariableNames();
	_inputVariableTable = evaluator.inputVariableTable();
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
std::shared_ptr<AsynchronousParticleModifier::ComputeEngine> RadialComputeModifier::createEngine(TimePoint time, TimeInterval validityInterval)
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

	// Prepare output property.
	QExplicitlySharedDataPointer<ParticleProperty> outp;
	if(outputProperty().type() != ParticleProperty::UserProperty) {
		outp = new ParticleProperty(posProperty->size(), outputProperty().type(), 0, false);
	}
	else if(!outputProperty().name().isEmpty() && propertyComponentCount() > 0) {
		outp = new ParticleProperty(posProperty->size(), qMetaTypeId<FloatType>(), propertyComponentCount(), 0, outputProperty().name(), false);
	}
	else {
		throw Exception(tr("Output property has not been specified."));
	}
	if(selfExpressions().size() != outp->componentCount())
		throw Exception(tr("Number of central expressions does not match component count of output property."));
	if(neighborExpressions().size() != outp->componentCount())
		throw Exception(tr("Number of neighbor expressions does not match component count of output property."));

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<RadialComputeEngine>(validityInterval, outp.data(), posProperty->storage(), inputCell->data(), cutoff(),
			selfExpressions(), neighborExpressions(),
			std::move(inputProperties), currentFrame, simulationTimestep);
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void RadialComputeModifier::RadialComputeEngine::perform()
{
	setProgressText(tr("Computing particle property '%1'").arg(outputProperty()->name()));

	OVITO_ASSERT(_selfExpressions.size() == outputProperty()->componentCount());
	OVITO_ASSERT(_neighborExpressions.size() == outputProperty()->componentCount());

	// Prepare the neighbor list.
	CutoffNeighborFinder neighborFinder;
	if(!neighborFinder.prepare(_cutoff, positions(), cell(), nullptr, this))
		return;

	// Make a copy of the list of input properties.
	std::vector<ParticleProperty*> inputProperties;
	for(const auto& p : _inputProperties)
		inputProperties.push_back(p.data());

	// Initialize expression evaluators.
	ParticleExpressionEvaluator selfEvaluator;
	ParticleExpressionEvaluator neighborEvaluator;
	selfEvaluator.initialize(_selfExpressions, inputProperties, &cell(), _frameNumber, _simulationTimestep);
	neighborEvaluator.initialize(_neighborExpressions, inputProperties, &cell(), _frameNumber, _simulationTimestep);

	selfEvaluator.registerGlobalParameter("Cutoff", _cutoff);
	selfEvaluator.registerGlobalParameter("NumNeighbors", 0);
	neighborEvaluator.registerGlobalParameter("Cutoff", _cutoff, tr("radius"));
	neighborEvaluator.registerGlobalParameter("NumNeighbors", 0, tr("of central particle"));
	neighborEvaluator.registerGlobalParameter("Distance", 0, tr("from central particle"));
	neighborEvaluator.registerGlobalParameter("Delta.X", 0, tr("neighbor vector"));
	neighborEvaluator.registerGlobalParameter("Delta.Y", 0, tr("neighbor vector"));
	neighborEvaluator.registerGlobalParameter("Delta.Z", 0, tr("neighbor vector"));

	_inputVariableNames = neighborEvaluator.inputVariableNames();
	_inputVariableTable = neighborEvaluator.inputVariableTable();

	// Parallelized loop over all particles.
	setProgressRange(positions()->size());
	setProgressValue(0);
	parallelForChunks(positions()->size(), *this, [this, &selfEvaluator, &neighborEvaluator, &neighborFinder](size_t startIndex, size_t count, FutureInterfaceBase& futureInterface) {
		ParticleExpressionEvaluator::Worker selfWorker(selfEvaluator);
		ParticleExpressionEvaluator::Worker neighborWorker(neighborEvaluator);

		double* distanceVar = neighborWorker.variableAddress("Distance");
		double* deltaX = neighborWorker.variableAddress("Delta.X");
		double* deltaY = neighborWorker.variableAddress("Delta.Y");
		double* deltaZ = neighborWorker.variableAddress("Delta.Z");
		double* selfNumNeighbors = selfWorker.variableAddress("NumNeighbors");
		double* neighNumNeighbors = neighborWorker.variableAddress("NumNeighbors");
		if(!selfWorker.isVariableUsed("NumNeighbors") && !neighborWorker.isVariableUsed("NumNeighbors"))
			selfNumNeighbors = neighNumNeighbors = nullptr;

		size_t endIndex = startIndex + count;
		size_t componentCount = outputProperty()->componentCount();
		for(size_t particleIndex = startIndex; particleIndex < endIndex; particleIndex++) {

			if(selfNumNeighbors != nullptr) {
				// Determine number of neighbors.
				int nneigh = 0;
				for(CutoffNeighborFinder::Query neighQuery(neighborFinder, particleIndex); !neighQuery.atEnd(); neighQuery.next())
					nneigh++;
				*selfNumNeighbors = *neighNumNeighbors = nneigh;
			}

			for(size_t component = 0; component < componentCount; component++) {

				// Compute self term.
				FloatType value = selfWorker.evaluate(particleIndex, component);

				// Compute sum of neighbor terms.
				for(CutoffNeighborFinder::Query neighQuery(neighborFinder, particleIndex); !neighQuery.atEnd(); neighQuery.next()) {
					*distanceVar = sqrt(neighQuery.distanceSquared());
					*deltaX = neighQuery.delta().x();
					*deltaY = neighQuery.delta().y();
					*deltaZ = neighQuery.delta().z();
					value += neighborWorker.evaluate(neighQuery.current(), component);
				}

				// Store results.
				if(outputProperty()->dataType() == QMetaType::Int) {
					outputProperty()->setIntComponent(particleIndex, component, (int)value);
				}
				else {
					outputProperty()->setFloatComponent(particleIndex, component, value);
				}
			}

			// Update progress indicator.
			if((particleIndex % 1024) == 0)
				futureInterface.incrementProgressValue(1024);

			// Stop loop if canceled.
			if(futureInterface.isCanceled())
				return;
		}
	});
}

/******************************************************************************
* Unpacks the results of the computation engine and stores them in the modifier.
******************************************************************************/
void RadialComputeModifier::transferComputationResults(ComputeEngine* engine)
{
	RadialComputeEngine* eng = static_cast<RadialComputeEngine*>(engine);
	_computedProperty = eng->outputProperty();
	_inputVariableNames = eng->inputVariableNames();
	_inputVariableTable = eng->inputVariableTable();
}

/******************************************************************************
* Lets the modifier insert the cached computation results into the
* modification pipeline.
******************************************************************************/
PipelineStatus RadialComputeModifier::applyComputationResults(TimePoint time, TimeInterval& validityInterval)
{
	if(!_computedProperty)
		throw Exception(tr("No computation results available."));

	if(outputParticleCount() != _computedProperty->size())
		throw Exception(tr("The number of input particles has changed. The stored results have become invalid."));

	if(_computedProperty->type() == ParticleProperty::UserProperty)
		outputCustomProperty(_computedProperty.data());
	else
		outputStandardProperty(_computedProperty.data());

	return PipelineStatus::Success;
}

OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void RadialComputeModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	rollout = createRollout(tr("Radial compute"), rolloutParams);

    // Create the rollout contents.
	QVBoxLayout* mainLayout = new QVBoxLayout(rollout);
	mainLayout->setContentsMargins(4,4,4,4);

	QGridLayout* gridlayout = new QGridLayout();
	gridlayout->setContentsMargins(4,4,4,4);
	gridlayout->setColumnStretch(1, 1);

	// Cutoff parameter.
	FloatParameterUI* cutoffRadiusPUI = new FloatParameterUI(this, PROPERTY_FIELD(RadialComputeModifier::_cutoff));
	gridlayout->addWidget(cutoffRadiusPUI->label(), 0, 0);
	gridlayout->addLayout(cutoffRadiusPUI->createFieldLayout(), 0, 1);
	cutoffRadiusPUI->setMinValue(0);
	mainLayout->addLayout(gridlayout);

	QGroupBox* propertiesGroupBox = new QGroupBox(tr("Output property"), rollout);
	mainLayout->addWidget(propertiesGroupBox);
	QVBoxLayout* propertiesLayout = new QVBoxLayout(propertiesGroupBox);
	propertiesLayout->setContentsMargins(6,6,6,6);
	propertiesLayout->setSpacing(4);

	// Output property
	ParticlePropertyParameterUI* outputPropertyUI = new ParticlePropertyParameterUI(this, PROPERTY_FIELD(RadialComputeModifier::_outputProperty), false, false);
	propertiesLayout->addWidget(outputPropertyUI->comboBox());

	selfExpressionsGroupBox = new QGroupBox(tr("Central expression"));
	mainLayout->addWidget(selfExpressionsGroupBox);
	selfExpressionsLayout = new QGridLayout(selfExpressionsGroupBox);
	selfExpressionsLayout->setContentsMargins(4,4,4,4);
	selfExpressionsLayout->setSpacing(1);
	selfExpressionsLayout->setColumnStretch(1,1);

	neighborExpressionsGroupBox = new QGroupBox(tr("Neighbor expression"));
	mainLayout->addWidget(neighborExpressionsGroupBox);
	neighborExpressionsLayout = new QGridLayout(neighborExpressionsGroupBox);
	neighborExpressionsLayout->setContentsMargins(4,4,4,4);
	neighborExpressionsLayout->setSpacing(1);
	neighborExpressionsLayout->setColumnStretch(1,1);

	// Status label.
	mainLayout->addWidget(statusLabel());

	QWidget* variablesRollout = createRollout(tr("Variables"), rolloutParams.after(rollout));
    QVBoxLayout* variablesLayout = new QVBoxLayout(variablesRollout);
    variablesLayout->setContentsMargins(4,4,4,4);
	variableNamesList = new QLabel();
	variableNamesList->setWordWrap(true);
	variableNamesList->setTextInteractionFlags(Qt::TextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard | Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard));
	variablesLayout->addWidget(variableNamesList);

	// Update input variables list if another modifier has been loaded into the editor.
	connect(this, &RadialComputeModifierEditor::contentsReplaced, this, &RadialComputeModifierEditor::updateEditorFields);
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool RadialComputeModifierEditor::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	if(source == editObject() && event->type() == ReferenceEvent::TargetChanged) {
		updateEditorFields();
	}
	return ParticleModifierEditor::referenceEvent(source, event);
}

/******************************************************************************
* Updates the enabled/disabled status of the editor's controls.
******************************************************************************/
void RadialComputeModifierEditor::updateEditorFields()
{
	RadialComputeModifier* mod = static_object_cast<RadialComputeModifier>(editObject());
	if(!mod) return;

	const QStringList& selfExpr = mod->selfExpressions();
	while(selfExpr.size() > selfExpressionBoxes.size()) {
		QLabel* label = new QLabel();
		AutocompleteLineEdit* edit = new AutocompleteLineEdit();
		edit->setWordList(mod->inputVariableNames());
		selfExpressionsLayout->addWidget(label, selfExpressionBoxes.size(), 0);
		selfExpressionsLayout->addWidget(edit, selfExpressionBoxes.size(), 1);
		selfExpressionBoxes.push_back(edit);
		selfExpressionBoxLabels.push_back(label);
		connect(edit, &AutocompleteLineEdit::editingFinished, this, &RadialComputeModifierEditor::onExpressionEditingFinished);
	}
	while(selfExpr.size() < selfExpressionBoxes.size()) {
		delete selfExpressionBoxes.takeLast();
		delete selfExpressionBoxLabels.takeLast();
	}
	OVITO_ASSERT(selfExpressionBoxes.size() == selfExpr.size());
	OVITO_ASSERT(selfExpressionBoxLabels.size() == selfExpr.size());

	const QStringList& neighExpr = mod->neighborExpressions();
	while(neighExpr.size() > neighborExpressionBoxes.size()) {
		QLabel* label = new QLabel();
		AutocompleteLineEdit* edit = new AutocompleteLineEdit();
		edit->setWordList(mod->inputVariableNames());
		neighborExpressionsLayout->addWidget(label, neighborExpressionBoxes.size(), 0);
		neighborExpressionsLayout->addWidget(edit, neighborExpressionBoxes.size(), 1);
		neighborExpressionBoxes.push_back(edit);
		neighborExpressionBoxLabels.push_back(label);
		connect(edit, &AutocompleteLineEdit::editingFinished, this, &RadialComputeModifierEditor::onExpressionEditingFinished);
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

	for(int i = 0; i < selfExpr.size(); i++) {
		selfExpressionBoxes[i]->setText(selfExpr[i]);
		if(selfExpr.size() == 1)
			selfExpressionBoxLabels[i]->hide();
		else {
			if(i < standardPropertyComponentNames.size())
				selfExpressionBoxLabels[i]->setText(tr("%1:").arg(standardPropertyComponentNames[i]));
			else
				selfExpressionBoxLabels[i]->setText(tr("%1:").arg(i+1));
			selfExpressionBoxLabels[i]->show();
		}
	}
	for(int i = 0; i < neighExpr.size(); i++) {
		neighborExpressionBoxes[i]->setText(neighExpr[i]);
		if(selfExpr.size() == 1)
			neighborExpressionBoxLabels[i]->hide();
		else {
			if(i < standardPropertyComponentNames.size())
				neighborExpressionBoxLabels[i]->setText(tr("%1:").arg(standardPropertyComponentNames[i]));
			else
				neighborExpressionBoxLabels[i]->setText(tr("%1:").arg(i+1));
			neighborExpressionBoxLabels[i]->show();
		}
	}

	variableNamesList->setText(mod->inputVariableTable());

	container()->updateRolloutsLater();
}

/******************************************************************************
* Is called when the user has typed in an expression.
******************************************************************************/
void RadialComputeModifierEditor::onExpressionEditingFinished()
{
	RadialComputeModifier* mod = static_object_cast<RadialComputeModifier>(editObject());
	QLineEdit* edit = (QLineEdit*)sender();
	int index = selfExpressionBoxes.indexOf(edit);
	if(index >= 0) {
		undoableTransaction(tr("Change central expression"), [mod, edit, index]() {
			QStringList expr = mod->selfExpressions();
			expr[index] = edit->text();
			mod->setSelfExpressions(expr);
		});
	}
	else {
		int index = neighborExpressionBoxes.indexOf(edit);
		OVITO_ASSERT(index >= 0);
		undoableTransaction(tr("Change neighbor expression"), [mod, edit, index]() {
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
