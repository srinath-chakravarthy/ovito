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

#include <plugins/pyscript/PyScript.h>
#include <core/animation/AnimationSettings.h>
#include <core/dataset/DataSetContainer.h>
#include <core/dataset/UndoStack.h>
#include <core/gui/properties/StringParameterUI.h>
#include <core/utilities/concurrent/TaskManager.h>
#include "PythonScriptModifier.h"

#ifndef signals
#define signals Q_SIGNALS
#endif
#ifndef slots
#define slots Q_SLOTS
#endif
#include <Qsci/qsciscintilla.h>
#include <Qsci/qscilexerpython.h>

namespace PyScript {

OVITO_BEGIN_INLINE_NAMESPACE(Internal)
	IMPLEMENT_OVITO_OBJECT(PyScript, PythonScriptModifierEditor, PropertiesEditor);
OVITO_END_INLINE_NAMESPACE

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(PyScript, PythonScriptModifier, Modifier);
SET_OVITO_OBJECT_EDITOR(PythonScriptModifier, PythonScriptModifierEditor);
DEFINE_PROPERTY_FIELD(PythonScriptModifier, _script, "Script");
SET_PROPERTY_FIELD_LABEL(PythonScriptModifier, _script, "Script");

/******************************************************************************
* Constructor.
******************************************************************************/
PythonScriptModifier::PythonScriptModifier(DataSet* dataset) : Modifier(dataset),
		_scriptExecutionQueued(false),
		_computingInterval(TimeInterval::empty())
{
	INIT_PROPERTY_FIELD(PythonScriptModifier::_script);

	// Load example script.
	setScript("from ovito.data import *\n\n"
			"def modify(frame, input, output):\n"
			"\tprint(\"Hello world!\")\n"
			"\tprint(\"The input contains %i particles.\" % input.number_of_particles)\n");
}

/******************************************************************************
* This method is called by the system when the upstream modification pipeline
* has changed.
******************************************************************************/
void PythonScriptModifier::upstreamPipelineChanged(ModifierApplication* modApp)
{
	Modifier::upstreamPipelineChanged(modApp);
	invalidateCachedResults(true);
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void PythonScriptModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	Modifier::propertyChanged(field);

	// Recompute results when script has been changed.
	if(field == PROPERTY_FIELD(PythonScriptModifier::_script)) {
		_modifyScriptFunction = boost::none;
		invalidateCachedResults(false);
	}
}

/******************************************************************************
* Invalidates the modifier's result cache so that the results will be recomputed
* next time the modifier is evaluated.
******************************************************************************/
void PythonScriptModifier::invalidateCachedResults(bool discardCache)
{
	// Stop an already running script as soon as possible.
	stopRunningScript();

	// Discard cached result data.
	if(discardCache)
		_outputCache.clear();
	else
		_outputCache.setStateValidity(TimeInterval::empty());
}

/******************************************************************************
* This modifies the input data.
******************************************************************************/
PipelineStatus PythonScriptModifier::modifyObject(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	if(state.status().type() != PipelineStatus::Pending) {
		if(!_outputCache.stateValidity().contains(time) && !_computingInterval.contains(time)) {
			// Stop already running script.
			stopRunningScript();

			// Limit validity interval of the computation to the current frame.
			_inputCache = state;
			_inputCache.intersectStateValidity(time);
			_computingInterval = _inputCache.stateValidity();

			// Request script execution.
			if(ScriptEngine::activeEngine() != nullptr) {
				// When running in the context of an active script engine, process request immediately.
				runScriptFunction();
			}
			else if(!_scriptExecutionQueued) {
				// When running in GUI mode, process request as soon as possible.
				_scriptExecutionQueued = true;
				QMetaObject::invokeMethod(this, "runScriptFunction", Qt::QueuedConnection);
			}
		}
	}

	PipelineStatus status;
	if(!_computingInterval.contains(time)) {
		if(!_outputCache.stateValidity().contains(time)) {
			if(state.status().type() != PipelineStatus::Pending)
				status = PipelineStatus(PipelineStatus::Error, tr("The modifier results have not been computed yet."));
			else
				status = PipelineStatus(PipelineStatus::Warning, tr("Waiting for input data to become ready..."));
		}
		else {
			state = _outputCache;
			status = state.status();
		}
	}
	else {
		if(!_outputCache.isEmpty()) {
			state = _outputCache;
			state.setStateValidity(time);
		}
		status = PipelineStatus(PipelineStatus::Pending, tr("Results are being computed..."));
	}
	// Always restrict validity of results to current time.
	state.intersectStateValidity(time);

	setStatus(status);
	return status;
}

/******************************************************************************
* Executes the Python script function to compute the modifier results.
******************************************************************************/
void PythonScriptModifier::runScriptFunction()
{
	_scriptExecutionQueued = false;

	do {
		if(!_generatorObject || _generatorObject->is_none()) {

			// Check if an evaluation request is still pending.
			_computingInterval = _inputCache.stateValidity();
			if(_computingInterval.isEmpty())
				return;

			// This function is not reentrant.
			OVITO_ASSERT(!_runningTask);

			// Reset script log buffer.
			_scriptLogOutput.clear();

			// Set output cache to input by default.
			_outputCache = _inputCache;

			// Input is no longer needed.
			_inputCache.clear();

			try {

				// Initialize local script engine if there is no active engine to re-use.
				if(!_scriptEngine) {
					_scriptEngine.reset(new ScriptEngine(dataset(), nullptr, false));
					connect(_scriptEngine.get(), &ScriptEngine::scriptOutput, this, &PythonScriptModifier::onScriptOutput);
					connect(_scriptEngine.get(), &ScriptEngine::scriptError, this, &PythonScriptModifier::onScriptOutput);
					_mainNamespacePrototype = _scriptEngine->mainNamespace();
				}

				// Compile script if needed.
				if(!_modifyScriptFunction || _modifyScriptFunction->is_none()) {
					compileScript();
				}

				// Check if script function has been set.
				if(!_modifyScriptFunction || _modifyScriptFunction->is_none())
					throw Exception(tr("PythonScriptModifier script function has not been set."));

				// Get animation frame at which the modifier is evaluated.
				int animationFrame = dataset()->animationSettings()->timeToFrame(_computingInterval.start());

				// Construct progress callback object.
				_runningTask = std::make_shared<ProgressHelper>();
				// Register background task so user/system can cancel it.
				dataset()->container()->taskManager().registerTask(_runningTask);
				_runningTask->reportStarted();
				_runningTask->setProgressText(tr("Running modifier script"));

				// Make sure the actions of the modify() function are not recorded on the undo stack.
				UndoSuspender noUndo(dataset());

				// Wrap data in a Python DataCollection object, because the PipelineFlowState class
				// is not accessible from Python.
				_dataCollection = new CompoundObject(dataset());
				_dataCollection->setDataObjects(_outputCache);

				// Create an extra DataCollection that holds the modifier's input.
				OORef<CompoundObject> inputDataCollection = new CompoundObject(dataset());
				inputDataCollection->setDataObjects(_outputCache);

				ScriptEngine* engine = ScriptEngine::activeEngine();
				if(!engine) engine = _scriptEngine.get();

				engine->execute([this,animationFrame,&inputDataCollection,engine]() {
					// Prepare arguments to be passed to script function.
					boost::python::tuple arguments = boost::python::make_tuple(animationFrame, inputDataCollection, _dataCollection);

					// Execute modify() script function.
					_generatorObject = engine->callObject(*_modifyScriptFunction, arguments);
				});
			}
			catch(const Exception& ex) {
				_scriptLogOutput += ex.message();
				_outputCache.setStatus(PipelineStatus(PipelineStatus::Error, ex.message()));
			}

			// Check if the function has returned a generator object.
			if(_generatorObject && !_generatorObject->is_none()) {

				// Keep calling this method in GUI mode. Otherwise stay in the outer while loop.
				if(ScriptEngine::activeEngine() == nullptr)
					QMetaObject::invokeMethod(this, "runScriptFunction", Qt::QueuedConnection);
			}
			else {
				// Indicate that we are done.
				scriptCompleted();
				break;
			}
		}
		else {
			OVITO_ASSERT(_runningTask);

			// Perform one computation step by calling the generator object.
			bool exhausted = false;
			try {
				// Make sure the actions of the modify() function are not recorded on the undo stack.
				UndoSuspender noUndo(dataset());

				// Get script engine to execute the script.
				ScriptEngine* engine = ScriptEngine::activeEngine();
				if(!engine) engine = _scriptEngine.get();

				if(_runningTask->isCanceled()) {
					_outputCache.setStateValidity(TimeInterval::empty());
					throw Exception(tr("Modifier script execution has been canceled by the user."));
				}

				// Measure how long the script is running.
				QTime time;
				time.start();
				do {

					engine->execute([this, &exhausted]() {
						PyObject* item = PyIter_Next(_generatorObject->ptr());
						if(item != NULL) {
							boost::python::object itemObj{boost::python::handle<>(item)};
							if(PyFloat_Check(item)) {
								double progressValue = boost::python::extract<double>(item);
								if(progressValue >= 0.0 && progressValue <= 1.0) {
									_runningTask->setProgressRange(100);
									_runningTask->setProgressValue((int)(progressValue * 100.0));
								}
								else {
									_runningTask->setProgressRange(0);
									_runningTask->setProgressValue(0);
								}
							}
							else {
								boost::python::extract<QString> get_str(item);
								if(get_str.check())
									_runningTask->setProgressText(get_str());
							}
						}
						else {
							exhausted = true;
							if(PyErr_Occurred())
								boost::python::throw_error_already_set();
						}
					});

					// Keep calling the generator object until
					// 30 milliseconds have passed or until it is exhausted.
				}
				while(!exhausted && time.elapsed() < 30);

				if(!exhausted) {
					// Keep calling this method in GUI mode. Otherwise stay in the outer while loop.
					if(ScriptEngine::activeEngine() == nullptr)
						QMetaObject::invokeMethod(this, "runScriptFunction", Qt::QueuedConnection);
				}
				else {
					// Indicate that we are done.
					scriptCompleted();
					break;
				}
			}
			catch(const Exception& ex) {
				_outputCache.setStatus(PipelineStatus(PipelineStatus::Error, ex.message()));
				scriptCompleted();
				break;
			}
		}
	}
	while(ScriptEngine::activeEngine() != nullptr);

	// Notify UI that the log output has changed.
	notifyDependents(ReferenceEvent::ObjectStatusChanged);
}

/******************************************************************************
* This is called when the script function was successfully completed.
******************************************************************************/
void PythonScriptModifier::scriptCompleted()
{
	// Collect results produced by script.
	if(_outputCache.status().type() != PipelineStatus::Error && _dataCollection != nullptr) {
		_outputCache.attributes() = _dataCollection->attributes();
		_outputCache.clearObjects();
		for(DataObject* obj : _dataCollection->dataObjects())
			_outputCache.addObject(obj);
	}

	// Indicate that we are done.
	_computingInterval.setEmpty();
	_dataCollection.reset();
	_generatorObject = boost::none;

	// Set output status.
	setStatus(_outputCache.status());

	// Signal completion of background task.
	if(_runningTask) {
		_runningTask->reportFinished();
		_runningTask.reset();
	}

	// Notify pipeline system that the evaluation request was satisfied or not satisfied.
	notifyDependents(ReferenceEvent::PendingStateChanged);
}

/******************************************************************************
* Compiles the script entered by the user.
******************************************************************************/
void PythonScriptModifier::compileScript()
{
	OVITO_ASSERT(_scriptEngine);

	// Reset Python environment.
	_scriptEngine->mainNamespace() = _mainNamespacePrototype->copy();
	_modifyScriptFunction = boost::python::object();
	// Run script once.
	_scriptEngine->executeCommands(script());
	// Extract the modify() function defined by the script.
	_scriptEngine->execute([this]() {
		try {
			_modifyScriptFunction = _scriptEngine->mainNamespace()["modify"];
			if(!PyCallable_Check(_modifyScriptFunction->ptr())) {
				_modifyScriptFunction = boost::python::object();
				throw Exception(tr("Invalid Python script. It does not define a callable function modify()."));
			}
		}
		catch(const boost::python::error_already_set&) {
			PyErr_Clear();
			throw Exception(tr("Invalid Python script. It does not define the function modify()."));
		}
	});
}

/******************************************************************************
* Is called when the script generates some output.
******************************************************************************/
void PythonScriptModifier::onScriptOutput(const QString& text)
{
	_scriptLogOutput += text;
}

/******************************************************************************
* Sets the status returned by the modifier and generates a
* ReferenceEvent::ObjectStatusChanged event.
******************************************************************************/
void PythonScriptModifier::setStatus(const PipelineStatus& status)
{
	if(status == _modifierStatus) return;
	_modifierStatus = status;
	notifyDependents(ReferenceEvent::ObjectStatusChanged);
}

/******************************************************************************
* Stops a currently running script engine.
******************************************************************************/
void PythonScriptModifier::stopRunningScript()
{
	_inputCache.clear();
	_dataCollection.reset();
	if(_runningTask) {
		_runningTask->cancel();
		_runningTask->reportFinished();
		_runningTask.reset();
	}
	// Discard active generator object.
	_generatorObject = boost::none;
}

/******************************************************************************
* Asks this object to delete itself.
******************************************************************************/
void PythonScriptModifier::deleteReferenceObject()
{
	// Interrupt running script when modifier is deleted.
	stopRunningScript();
	invalidateCachedResults(true);

	Modifier::deleteReferenceObject();
}

OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void PythonScriptModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Python script"), rolloutParams, "particles.modifiers.python_script.html");

    // Create the rollout contents.
	QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	int row = 0;

	StringParameterUI* namePUI = new StringParameterUI(this, PROPERTY_FIELD(Modifier::_title));
	layout->addWidget(new QLabel(tr("User-defined modifier name:")), row++, 0);
	static_cast<QLineEdit*>(namePUI->textBox())->setPlaceholderText(PythonScriptModifier::OOType.displayName());
	layout->addWidget(namePUI->textBox(), row++, 0);

	QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);

	layout->addWidget(new QLabel(tr("Python script:")), row++, 0);
	_codeEditor = new QsciScintilla();
	_codeEditor->setEnabled(false);
	_codeEditor->setAutoIndent(true);
	_codeEditor->setTabWidth(4);
	_codeEditor->setFont(font);
	QsciLexerPython* lexer = new QsciLexerPython(_codeEditor);
	lexer->setDefaultFont(font);
	_codeEditor->setLexer(lexer);
	_codeEditor->setMarginWidth(0, QFontMetrics(font).width("0000"));
	_codeEditor->setMarginWidth(1, 0);
	_codeEditor->setMarginLineNumbers(0, true);
	layout->addWidget(_codeEditor, row++, 0);

	QPushButton* applyButton = new QPushButton(tr("Commit script"));
	layout->addWidget(applyButton, row++, 0);

	layout->addWidget(new QLabel(tr("Script output:")), row++, 0);
	_errorDisplay = new QsciScintilla();
	_errorDisplay->setTabWidth(_codeEditor->tabWidth());
	_errorDisplay->setFont(font);
	_errorDisplay->setReadOnly(true);
	_errorDisplay->setMarginWidth(1, 0);
	layout->addWidget(_errorDisplay, row++, 0);

	connect(this, &PropertiesEditor::contentsChanged, this, &PythonScriptModifierEditor::onContentsChanged);
	connect(applyButton, &QPushButton::clicked, this, &PythonScriptModifierEditor::onApplyChanges);
}

/******************************************************************************
* Is called when the current edit object has generated a change
* event or if a new object has been loaded into editor.
******************************************************************************/
void PythonScriptModifierEditor::onContentsChanged(RefTarget* editObject)
{
	PythonScriptModifier* modifier = static_object_cast<PythonScriptModifier>(editObject);
	if(modifier) {
		_codeEditor->setText(modifier->script());
		_codeEditor->setEnabled(true);
		_errorDisplay->setText(modifier->scriptLogOutput());
	}
	else {
		_codeEditor->setEnabled(false);
		_codeEditor->clear();
		_errorDisplay->clear();
	}
}

/******************************************************************************
* Is called when the user presses the 'Apply' button to commit the Python script.
******************************************************************************/
void PythonScriptModifierEditor::onApplyChanges()
{
	PythonScriptModifier* modifier = static_object_cast<PythonScriptModifier>(editObject());
	if(!modifier) return;

	undoableTransaction(tr("Change Python script"), [this, modifier]() {
		modifier->setScript(_codeEditor->text());
	});
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool PythonScriptModifierEditor::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	if(source == editObject() && event->type() == ReferenceEvent::ObjectStatusChanged) {
		PythonScriptModifier* modifier = static_object_cast<PythonScriptModifier>(editObject());
		if(modifier)
			_errorDisplay->setText(modifier->scriptLogOutput());
	}
	return PropertiesEditor::referenceEvent(source, event);
}

OVITO_END_INLINE_NAMESPACE

}	// End of namespace
