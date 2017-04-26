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
#include <core/reference/CloneHelper.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/DataSetContainer.h>
#include <core/scene/SceneNode.h>
#include <core/scene/ObjectNode.h>
#include <core/scene/SceneRoot.h>
#include <core/scene/SelectionSet.h>
#include <core/animation/AnimationSettings.h>
#include <core/viewport/ViewportConfiguration.h>
#include <core/rendering/RenderSettings.h>
#include <core/utilities/concurrent/TaskManager.h>
#include "PythonBinding.h"

namespace PyScript {

using namespace Ovito;

void defineAppSubmodule(py::module parentModule)
{
	py::module m = parentModule.def_submodule("App");

	py::class_<OvitoObject, OORef<OvitoObject>>(m, "OvitoObject")
		.def("__str__", [](py::object& pyobj) {
			return py::str("<{} at 0x{:x}>").format(pyobj.attr("__class__").attr("__name__"), (std::intptr_t)pyobj.cast<OvitoObject*>());
		})
		.def("__repr__", [](py::object& pyobj) {
			return py::str("{}()").format(pyobj.attr("__class__").attr("__name__"));
		})
		.def("__eq__", [](OvitoObject* o, py::object& other) {
			try { return o == other.cast<OvitoObject*>(); }
			catch(const py::cast_error&) { return false; }
		})
		.def("__ne__", [](OvitoObject* o, py::object& other) {
			try { return o != other.cast<OvitoObject*>(); }
			catch(const py::cast_error&) { return true; }
		})
	;

	ovito_abstract_class<RefMaker, OvitoObject>{m}
		.def_property_readonly("dataset", py::cpp_function(&RefMaker::dataset, py::return_value_policy::reference))
	;

	ovito_abstract_class<RefTarget, RefMaker>{m}
		//.def("isReferencedBy", &RefTarget::isReferencedBy)
		//.def("deleteReferenceObject", &RefTarget::deleteReferenceObject)

		// This is used by DataCollection.copy_if_needed():
		.def_property_readonly("num_dependents", [](RefTarget& t) { return t.dependents().size(); })
		// This is used by DataCollection.__getitem__():
		.def_property_readonly("object_title", &RefTarget::objectTitle)
	;

	// Note that, for DataSet, we are not using OORef<> as holder type like we normally do for other OvitoObject-derived classes, because
	// we don't want a ScriptEngine to hold a counted reference to a DataSet that it belongs to.
	// This would create a cyclic reference and potentially lead to a memory leak.
	py::class_<DataSet>(m, "DataSet",
			"A container object holding all data associated with an OVITO program session. "
			"It provides access to the scene data, the viewports, the current selection, and the animation settings. "
			"Basically everything that would get saved in an OVITO state file. "
			"\n\n"
			"There exists only one global instance of this class, which can be accessed via the :py:data:`ovito.dataset` module-level attribute.")
		.def_property_readonly("scene_root", &DataSet::sceneRoot)
		.def_property_readonly("anim", &DataSet::animationSettings,
				"An :py:class:`~ovito.anim.AnimationSettings` object, which manages various animation-related settings in OVITO such as the number of frames, the current frame, playback speed etc.")
		.def_property_readonly("viewports", &DataSet::viewportConfig,
				"A :py:class:`~ovito.vis.ViewportConfiguration` object managing the viewports in OVITO's main window.")
		.def_property_readonly("render_settings", &DataSet::renderSettings,
				"The global :py:class:`~ovito.vis.RenderSettings` object, which stores the current settings for rendering pictures and movies. "
				"These are the settings the user can edit in the graphical version of OVITO.")
		.def("save", &DataSet::saveToFile,
			"save(filename)"
			"\n\n"
		    "Saves the dataset including the viewports, all nodes in the scene, modification pipelines, and other settings to an OVITO file. "
    		"This function works like the *Save State As* function in OVITO's file menu."
			"\n\n"
			":param str filename: The path of the file to be written\n",
			py::arg("filename"))
		// This is needed for the DataSet.selected_node attribute:
		.def_property_readonly("selection", &DataSet::selection)
		// This is needed by Viewport.render():
		.def("render_scene", &DataSet::renderScene)
		.def_property_readonly("container", &DataSet::container, py::return_value_policy::reference)
		//.def("clearScene", &DataSet::clearScene)
		//.def("rescaleTime", &DataSet::rescaleTime)
		//.def("waitUntilSceneIsReady", &DataSet::waitUntilSceneIsReady)
		//.def_property("filePath", &DataSet::filePath, &DataSet::setFilePath)
	;

	py::class_<DataSetContainer>{m, "DataSetContainer"}
		//.def_property("currentSet", &DataSetContainer::currentSet, &DataSetContainer::setCurrentSet)
	;

	py::class_<CloneHelper>(m, "CloneHelper")
		.def(py::init<>())
		.def("clone", static_cast<OORef<RefTarget> (CloneHelper::*)(RefTarget*, bool)>(&CloneHelper::cloneObject<RefTarget>))
	;

	py::class_<TaskManager>(m, "TaskManager")
		//.def_property_readonly("canceled", &AbstractProgressDisplay::wasCanceled)
	;
}

};
