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
#include "PythonViewportOverlay.h"

namespace PyScript {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(PyScript, PythonViewportOverlay, ViewportOverlay);
DEFINE_PROPERTY_FIELD(PythonViewportOverlay, _script, "Script");
SET_PROPERTY_FIELD_LABEL(PythonViewportOverlay, _script, "Script");

/******************************************************************************
* Constructor.
******************************************************************************/
PythonViewportOverlay::PythonViewportOverlay(DataSet* dataset) : ViewportOverlay(dataset),
		_scriptEngine(dataset, nullptr, false)
{
	INIT_PROPERTY_FIELD(PythonViewportOverlay::_script);

	connect(&_scriptEngine, &ScriptEngine::scriptOutput, this, &PythonViewportOverlay::onScriptOutput);
	connect(&_scriptEngine, &ScriptEngine::scriptError, this, &PythonViewportOverlay::onScriptOutput);

	// Load example script.
	setScript("import ovito\n"
			"# The following function is called by OVITO to let the script\n"
			"# draw arbitrary graphics content into the viewport.\n"
			"# It is passed a QPainter (see http://qt-project.org/doc/qt-5/qpainter.html).\n"
			"def render(painter, **args):\n"
			"\t# This demo code prints the current animation frame\n"
			"\t# into the upper left corner of the viewport.\n"
			"\txpos = 10\n"
			"\typos = 10 + painter.fontMetrics().ascent()\n"
			"\ttext = \"Frame {}\".format(ovito.dataset.anim.current_frame)\n"
			"\tpainter.drawText(xpos, ypos, text)\n"
			"\t# The following code prints the current number of particles\n"
			"\t# into the lower left corner of the viewport.\n"
			"\txpos = 10\n"
			"\typos = painter.window().height() - 10\n"
			"\tif ovito.dataset.selected_node:\n"
			"\t\tnum_particles = ovito.dataset.selected_node.compute().number_of_particles\n"
			"\t\ttext = \"{} particles\".format(num_particles)\n"
			"\telse:\n"
			"\t\ttext = \"no particles\"\n"
			"\tpainter.drawText(xpos, ypos, text)\n");
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void PythonViewportOverlay::propertyChanged(const PropertyFieldDescriptor& field)
{
	ViewportOverlay::propertyChanged(field);
	if(field == PROPERTY_FIELD(PythonViewportOverlay::_script)) {
		compileScript();
	}
}

/******************************************************************************
* Compiles the script entered by the user.
******************************************************************************/
void PythonViewportOverlay::compileScript()
{
	_scriptOutput.clear();
	_overlayScriptFunction = py::function();
	try {
		_scriptEngine.executeCommands(script());

		// Extract the render() function defined by the script.
		_scriptEngine.execute([this]() {
			try {
				_overlayScriptFunction = _scriptEngine.mainNamespace()["render"];
				if(!_overlayScriptFunction.check()) {
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
		_scriptOutput += ex.message();
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
	if(!compilationSucessful())
		return;

	_scriptOutput.clear();
	try {

		// Enable antialiasing for the QPainter by default.
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setRenderHint(QPainter::TextAntialiasing);

		ScriptEngine* engine = ScriptEngine::activeEngine();
		if(!engine) engine = &_scriptEngine;

		engine->execute([this,engine,viewport,&painter,&projParams,renderSettings]() {

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
			engine->callObject(_overlayScriptFunction, arguments, kwargs);
		});
	}
	catch(const Exception& ex) {
		_scriptOutput += ex.message();
	}
	notifyDependents(ReferenceEvent::ObjectStatusChanged);
}

}	// End of namespace
