///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
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
#include <gui/widgets/rendering/FrameBufferWindow.h>
#include <gui/mainwin/MainWindow.h>
#include <gui/dataset/GuiDataSetContainer.h>
#include <plugins/pyscript/binding/PythonBinding.h>
#include <core/plugins/PluginManager.h>

namespace PyScript {

using namespace Ovito;

PYBIND11_PLUGIN(PyScriptGui)
{
	// Register the classes of this plugin with the global PluginManager.
	PluginManager::instance().registerLoadedPluginClasses();
	
	py::options options;
	options.disable_function_signatures();

	py::module m("PyScriptGui");

	py::class_<MainWindow>(m, "MainWindow")
		.def_property_readonly("frame_buffer_window", &MainWindow::frameBufferWindow, py::return_value_policy::reference)
	;

	py::class_<GuiDataSetContainer, DataSetContainer>(m, "GuiDataSetContainer")
		//.def("fileNew", &GuiDataSetContainer::fileNew)
		//.def("fileLoad", &GuiDataSetContainer::fileLoad)
		//.def("fileSave", &GuiDataSetContainer::fileSave)
		//.def("fileSaveAs", &GuiDataSetContainer::fileSaveAs)
		//.def("askForSaveChanges", &GuiDataSetContainer::askForSaveChanges)
		.def_property_readonly("window", &GuiDataSetContainer::mainWindow, py::return_value_policy::reference)
	;

	py::class_<FrameBufferWindow>(m, "FrameBufferWindow")
		.def_property_readonly("frame_buffer", &FrameBufferWindow::frameBuffer)
		.def("create_frame_buffer", &FrameBufferWindow::createFrameBuffer)
		.def("show_and_activate", &FrameBufferWindow::showAndActivateWindow)
	;

	return m.ptr();
}

OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(PyScriptGui);

};
