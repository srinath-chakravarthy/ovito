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
#include <gui/rendering/StandardSceneRenderer.h>
#include <gui/widgets/rendering/FrameBufferWindow.h>
#include <gui/mainwin/MainWindow.h>
#include <gui/dataset/GuiDataSetContainer.h>
#include <plugins/pyscript/binding/PythonBinding.h>

namespace PyScript {

using namespace boost::python;
using namespace Ovito;

BOOST_PYTHON_MODULE(PyScriptGUI)
{
	docstring_options docoptions(true, false);

	class_<MainWindow, bases<>, MainWindow, boost::noncopyable>("MainWindow", no_init)
		.add_property("frame_buffer_window", make_function(&MainWindow::frameBufferWindow, return_internal_reference<>()))
	;

	ovito_abstract_class<GuiDataSetContainer, DataSetContainer>()
		.def("fileNew", &GuiDataSetContainer::fileNew)
		.def("fileLoad", &GuiDataSetContainer::fileLoad)
		.def("fileSave", &GuiDataSetContainer::fileSave)
		.def("fileSaveAs", &GuiDataSetContainer::fileSaveAs)
		.def("askForSaveChanges", &GuiDataSetContainer::askForSaveChanges)
		.add_property("window", make_function(&GuiDataSetContainer::mainWindow, return_value_policy<reference_existing_object>()))
	;

	class_<FrameBufferWindow, bases<>, FrameBufferWindow, boost::noncopyable>("FrameBufferWindow", no_init)
		.add_property("frame_buffer", make_function(&FrameBufferWindow::frameBuffer, return_value_policy<copy_const_reference>()))
		.def("create_frame_buffer", make_function(&FrameBufferWindow::createFrameBuffer, return_value_policy<copy_const_reference>()))
		.def("show_and_activate", &FrameBufferWindow::showAndActivateWindow)
	;

	ovito_class<StandardSceneRenderer, SceneRenderer>(
			"The standard OpenGL-based renderer."
			"\n\n"
			"This is the default built-in rendering engine that is also used by OVITO to render the contents of the interactive viewports. "
			"Since it accelerates the generation of images by using the computer's graphics hardware, it is very fast.",
			"OpenGLRenderer")
		.add_property("antialiasing_level", &StandardSceneRenderer::antialiasingLevel, &StandardSceneRenderer::setAntialiasingLevel,
				"A positive integer controlling the level of supersampling. If 1, no supersampling is performed. For larger values, "
				"the image in rendered at a higher resolution and then scaled back to the output size to reduce aliasing artifacts."
				"\n\n"
				":Default: 3")
	;
}

OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(PyScriptGUI);


};
