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
#include <core/utilities/concurrent/TaskManager.h>
#include "PythonScriptModifier.h"

namespace PyScript {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(PythonScriptModifier, Modifier);
DEFINE_PROPERTY_FIELD(PythonScriptModifier, script, "Script");
SET_PROPERTY_FIELD_LABEL(PythonScriptModifier, script, "Script");

/******************************************************************************
* Constructor.
******************************************************************************/
PythonScriptModifier::PythonScriptModifier(DataSet* dataset) : Modifier(dataset),
		_scriptExecutionQueued(false),
		_computingInterval(TimeInterval::empty())
{
	INIT_PROPERTY_FIELD(script);
}

/******************************************************************************
* Loads the default values of this object's parameter fields.
******************************************************************************/
void PythonScriptModifier::loadUserDefaults()
{
	Modifier::loadUserDefaults();

	// Load example script.
	setScript("from ovito.data import *\n\n"
			"def modify(frame, input, output):\n"
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
	if(field == PROPERTY_FIELD(script)) {
		_modifyScriptFunction = py::object();
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
			if(ScriptEngine::activeEngine()) {
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
		if(!_generatorObject || _generatorObject.is_none()) {

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
					_scriptEngine.reset(new ScriptEngine(dataset(), dataset()->container()->taskManager(), true));
					connect(_scriptEngine.get(), &ScriptEngine::scriptOutput, this, &PythonScriptModifier::onScriptOutput);
					connect(_scriptEngine.get(), &ScriptEngine::scriptError, this, &PythonScriptModifier::onScriptOutput);
					_mainNamespacePrototype = _scriptEngine->mainNamespace();
				}

				// Compile script if needed.
				if(!_modifyScriptFunction) {
					compileScript();
				}

				// Check if script function has been set.
				if(!_modifyScriptFunction)
					throwException(tr("PythonScriptModifier script function has not been set."));

				// Get animation frame at which the modifier is evaluated.
				int animationFrame = dataset()->animationSettings()->timeToFrame(_computingInterval.start());

				// Construct progress callback object.
				_runningTask.reset(new SynchronousTask(dataset()->container()->taskManager()));
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

				_scriptEngine->execute([this,animationFrame,&inputDataCollection]() {
					// Prepare arguments to be passed to the script function.
					py::tuple arguments = py::make_tuple(animationFrame, inputDataCollection.get(), _dataCollection.get());

					// Execute modify() script function.
					_generatorObject = _scriptEngine->callObject(_modifyScriptFunction, arguments);
				});
			}
			catch(const Exception& ex) {
				_scriptLogOutput += ex.messages().join('\n');
				_outputCache.setStatus(PipelineStatus(PipelineStatus::Error, ex.message()));
			}

			// Check if the function has returned a generator object.
			if(_generatorObject && !_generatorObject.is_none()) {

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
				// Measure how long the script is running.
				QTime time;
				time.start();
				do {

					_scriptEngine->execute([this, &exhausted]() {
						py::handle item;
						{
							// Make sure the actions of the modify() function are not recorded on the undo stack.
							UndoSuspender noUndo(dataset());
							item = PyIter_Next(_generatorObject.ptr());
						}
						if(item) {
							py::object itemObj = py::reinterpret_steal<py::object>(item);
							if(PyFloat_Check(itemObj.ptr())) {
								double progressValue = itemObj.cast<double>();
								if(progressValue >= 0.0 && progressValue <= 1.0) {
									_runningTask->setProgressMaximum(100);
									_runningTask->setProgressValue((int)(progressValue * 100.0));
								}
								else {
									_runningTask->setProgressMaximum(0);
									_runningTask->setProgressValue(0);
								}
							}
							else {
								try {
									_runningTask->setProgressText(itemObj.cast<QString>());
								}
								catch(const py::cast_error&) {}
							}
						}
						else {
							exhausted = true;
							if(PyErr_Occurred())
								throw py::error_already_set();
						}
					});

					// Keep calling the generator object until
					// 30 milliseconds have passed or until it is exhausted.
				}
				while(!exhausted && time.elapsed() < 30);

				if(!_runningTask || _runningTask->isCanceled()) {
					_outputCache.setStateValidity(TimeInterval::empty());
					throwException(tr("Modifier script execution has been canceled by the user."));
				}

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
				_scriptLogOutput += ex.messages().join('\n');
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
	_generatorObject = py::object();

	// Set output status.
	setStatus(_outputCache.status());

	// Signal completion of background task.
	_runningTask.reset();

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
	_scriptEngine->mainNamespace() = _mainNamespacePrototype.attr("copy")();
	_modifyScriptFunction = py::function();
	// Run script once.
	_scriptEngine->executeCommands(script());
	// Extract the modify() function defined by the script.
	_scriptEngine->execute([this]() {
		try {
			_modifyScriptFunction = py::function(_scriptEngine->mainNamespace()["modify"]);
			if(!py::isinstance<py::function>(_modifyScriptFunction)) {
				_modifyScriptFunction = py::function();
				throwException(tr("Invalid Python script. It does not define a callable function modify()."));
			}
		}
		catch(const py::error_already_set&) {
			throwException(tr("Invalid Python script. It does not define the function modify()."));
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
		_runningTask.reset();
	}
	// Discard active generator object.
	_generatorObject = py::object();
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

}	// End of namespace
