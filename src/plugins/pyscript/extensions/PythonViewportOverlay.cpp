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

#include <plugins/pyscript/PyScript.h>
#include <plugins/pyscript/binding/PythonBinding.h>
#include <core/viewport/Viewport.h>
#include <core/rendering/RenderSettings.h>
#include <core/dataset/DataSetContainer.h>
#include "PythonViewportOverlay.h"

namespace PyScript {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(PythonViewportOverlay, ViewportOverlay);
DEFINE_PROPERTY_FIELD(PythonViewportOverlay, script, "Script");
SET_PROPERTY_FIELD_LABEL(PythonViewportOverlay, script, "Script");

/******************************************************************************
* Constructor.
******************************************************************************/
PythonViewportOverlay::PythonViewportOverlay(DataSet* dataset) : ViewportOverlay(dataset)
{
	INIT_PROPERTY_FIELD(script);
}

/******************************************************************************
* Loads the default values of this object's parameter fields.
******************************************************************************/
void PythonViewportOverlay::loadUserDefaults()
{
	ViewportOverlay::loadUserDefaults();

	// Load example script.
	setScript("import ovito\n"
			"\n"
			"# This user-defined function is called by OVITO to let it draw arbitrary graphics on top of the viewport.\n"
			"# It is passed a QPainter (see http://qt-project.org/doc/qt-5/qpainter.html).\n"
			"def render(painter, **args):\n"
			"\n"
			"\t# This demo code prints the current animation frame into the upper left corner of the viewport.\n"
			"\ttext1 = \"Frame {}\".format(ovito.dataset.anim.current_frame)\n"
			"\tpainter.drawText(10, 10 + painter.fontMetrics().ascent(), text1)\n"
			"\n"
			"\t# Also print the current number of particles into the lower left corner of the viewport.\n"
			"\tnode = ovito.dataset.selected_node\n"
			"\tnum_particles = (node.compute().number_of_particles if node else 0)\n"
			"\ttext2 = \"{} particles\".format(num_particles)\n"
			"\tpainter.drawText(10, painter.window().height() - 10, text2)\n"
			"\n"
			"\t# Print to the log window:\n"
			"\tprint(text1)\n"
			"\tprint(text2)\n");	
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void PythonViewportOverlay::propertyChanged(const PropertyFieldDescriptor& field)
{
	ViewportOverlay::propertyChanged(field);
	if(field == PROPERTY_FIELD(script)) {
		compileScript();
	}
}

/******************************************************************************
* Compiles the script entered by the user.
******************************************************************************/
void PythonViewportOverlay::compileScript()
{
	// Cannot execute scripts during file loading.
	if(isBeingLoaded()) return;

	_scriptOutput.clear();
	_overlayScriptFunction = py::function();
	try {

		// Initialize a local script engine.
		if(!_scriptEngine) {
			_scriptEngine.reset(new ScriptEngine(dataset(), dataset()->container()->taskManager(), true));
			connect(_scriptEngine.get(), &ScriptEngine::scriptOutput, this, &PythonViewportOverlay::onScriptOutput);
			connect(_scriptEngine.get(), &ScriptEngine::scriptError, this, &PythonViewportOverlay::onScriptOutput);
		}
		
		_scriptEngine->executeCommands(script());

		// Extract the render() function defined by the script.
		_scriptEngine->execute([this]() {
			try {
				_overlayScriptFunction = py::function(_scriptEngine->mainNamespace()["render"]);
				if(!py::isinstance<py::function>(_overlayScriptFunction)) {
					_overlayScriptFunction = py::function();
					throwException(tr("Invalid Python script. It does not define a callable function render()."));
				}
			}
			catch(const py::error_already_set&) {
				throwException(tr("Invalid Python script. It does not define the function render()."));
			}
		});
	}
	catch(const Exception& ex) {
		_scriptOutput += ex.messages().join('\n');
	}

	notifyDependents(ReferenceEvent::ObjectStatusChanged);
}

/******************************************************************************
* Is called when the script generates some output.
******************************************************************************/
void PythonViewportOverlay::onScriptOutput(const QString& text)
{
	_scriptOutput += text;
}

/******************************************************************************
* This method asks the overlay to paint its contents over the given viewport.
******************************************************************************/
void PythonViewportOverlay::render(Viewport* viewport, QPainter& painter, const ViewProjectionParameters& projParams, RenderSettings* renderSettings)
{
	// When the overlay was loaded from a scene file, the script is not compiled yet.
	// Let's do it now.
	if(!_scriptEngine)
		compileScript();

	if(!compilationSucessful())
		return;

	// This object instance may be deleted while this method is being executed.
	// This is to detect this situation.
	QPointer<PythonViewportOverlay> thisPointer(this);
	
	_scriptOutput.clear();
	try {
		// Enable antialiasing for the QPainter by default.
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setRenderHint(QPainter::TextAntialiasing);

		_scriptEngine->execute([this,viewport,&painter,&projParams,renderSettings]() {

			// Pass viewport, QPainter, and other information to the Python script function.
			// The QPainter pointer has to be converted to the representation used by PyQt.

			py::module numpy_module = py::module::import("numpy");
			py::module sip_module = py::module::import("sip");
			py::module qtgui_module = py::module::import("PyQt5.QtGui");

			py::dict kwargs;
			kwargs["viewport"] = py::cast(viewport);
			kwargs["render_settings"] = py::cast(renderSettings);
			kwargs["is_perspective"] = py::cast(projParams.isPerspective);
			kwargs["fov"] = py::cast(projParams.fieldOfView);
			kwargs["view_tm"] = py::cast(projParams.viewMatrix);
			kwargs["proj_tm"] = py::cast(projParams.projectionMatrix);

			py::object painter_ptr = py::cast(reinterpret_cast<std::uintptr_t>(&painter));
			py::object qpainter_class = qtgui_module.attr("QPainter");
			py::object sip_painter = sip_module.attr("wrapinstance")(painter_ptr, qpainter_class);
			py::tuple arguments = py::make_tuple(sip_painter);

			// Execute render() script function.
			_scriptEngine->callObject(_overlayScriptFunction, arguments, kwargs);
		});
	}
	catch(const Exception& ex) {
		_scriptOutput += ex.messages().join('\n');
	}

	if(thisPointer)
		thisPointer->notifyDependents(ReferenceEvent::ObjectStatusChanged);
}

}	// End of namespace
