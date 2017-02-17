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
#include <core/app/Application.h>
#include <core/plugins/PluginManager.h>
#include "PythonBinding.h"

namespace PyScript {

void defineAppSubmodule(py::module parentModule);	// Defined in AppBinding.cpp
void defineSceneSubmodule(py::module parentModule);	// Defined in SceneBinding.cpp
void defineIOSubmodule(py::module parentModule);	// Defined in FileIOBinding.cpp
void defineViewportSubmodule(py::module parentModule);	// Defined in ViewportBinding.cpp
void defineAnimationSubmodule(py::module parentModule);	// Defined in AnimationBinding.cpp
void defineRenderingSubmodule(py::module parentModule);	// Defined in RenderingBinding.cpp

using namespace Ovito;

PYBIND11_PLUGIN(PyScript)
{
	py::options options;
	options.disable_function_signatures();

	qDebug() << "Initializing PyScript module 1.";
	py::module m("PyScript");
	qDebug() << "Initializing PyScript module 2.";

	// Install Ovito to Python exception translator.
	py::register_exception_translator([](std::exception_ptr p) {
		try {
			if(p) std::rethrow_exception(p);
		}
		catch(const Exception& ex) {
			PyErr_SetString(PyExc_RuntimeError, ex.messages().join(QChar('\n')).toUtf8().constData());
		}
	});

	// Initialize an ad-hoc environment when not running as a standalone app. 
	// Otherwise this has already been done by the StandaloneApplication class.
	if(!Application::instance()) {
		try {
			qDebug() << "Creating OVITO ad-hoc application object.";
			Application* app = new Application(); // This will leak, but it doesn't matter because this Python module will never be unloaded.
			if(!app->initialize())
				throw Exception("Application object could not be initialized.");
			PluginManager::initialize();
		}
		catch(const Exception& ex) {
			ex.logError();
			throw std::runtime_error("Error while initializing OVITO environment.");
		}
		OVITO_ASSERT(Application::instance() != nullptr);
	}

	// Make Ovito program version number available to script.
	m.attr("version") = py::make_tuple(Application::applicationVersionMajor(), Application::applicationVersionMinor(), Application::applicationVersionRevision());
	m.attr("version_string") = py::cast(QCoreApplication::applicationVersion());

	// Make environment information available to the script.
	m.attr("gui_mode") = py::cast(Application::instance()->guiMode());
	m.attr("headless_mode") = py::cast(Application::instance()->headlessMode());

	// Register submodules.
	defineAppSubmodule(m);
	defineSceneSubmodule(m);
	defineAnimationSubmodule(m);
	defineIOSubmodule(m);
	defineViewportSubmodule(m);
	defineRenderingSubmodule(m);

	qDebug() << "PyScript module initalization finished.";

	return m.ptr();
}

OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(PyScript);

};
