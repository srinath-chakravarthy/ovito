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
#include <core/scene/SceneNode.h>
#include <core/scene/ObjectNode.h>
#include <core/scene/SceneRoot.h>
#include <core/scene/SelectionSet.h>
#include <core/scene/objects/DataObject.h>
#include <core/scene/objects/CompoundObject.h>
#include <core/scene/objects/geometry/TriMeshObject.h>
#include <core/scene/pipeline/Modifier.h>
#include <core/scene/pipeline/ModifierApplication.h>
#include <core/scene/pipeline/PipelineObject.h>
#include <core/utilities/concurrent/TaskManager.h>
#include <plugins/pyscript/extensions/PythonScriptModifier.h>
#include "PythonBinding.h"

namespace PyScript {

using namespace Ovito;

void defineSceneSubmodule(py::module parentModule)
{
	py::module m = parentModule.def_submodule("Scene");

	auto PipelineStatus_py = py::class_<PipelineStatus>(m, "PipelineStatus")
		.def(py::init<>())
		.def(py::init<PipelineStatus::StatusType, const QString&>())
		.def_property_readonly("type", &PipelineStatus::type)
		.def_property_readonly("text", &PipelineStatus::text)
		.def(py::self == PipelineStatus())
		.def(py::self != PipelineStatus())
	;

	py::enum_<PipelineStatus::StatusType>(PipelineStatus_py, "Type")
		.value("Success", PipelineStatus::Success)
		.value("Warning", PipelineStatus::Warning)
		.value("Error", PipelineStatus::Error)
		.value("Pending", PipelineStatus::Pending)
	;

	py::class_<PipelineFlowState>(m, "PipelineFlowState")
		//.def(py::init<>())
		//.def(py::init<DataObject*, TimeInterval>())
		//.def(py::init<const PipelineStatus&, const QVector<DataObject*>&, const TimeInterval&>())
		//.def("clear", &PipelineFlowState::clear)
		//.def("addObject", &PipelineFlowState::addObject)
		//.def("replaceObject", &PipelineFlowState::replaceObject)
		//.def("removeObject", &PipelineFlowState::removeObject)
		//.def("cloneObjectsIfNeeded", &PipelineFlowState::cloneObjectsIfNeeded)
		//.def_property_readonly("isEmpty", &PipelineFlowState::isEmpty)
		.def_property("status", &PipelineFlowState::status, &PipelineFlowState::setStatus)
		//.def_property_readonly("objects", &PipelineFlowState::objects)
	;

	auto DataObject_py = ovito_abstract_class<DataObject, RefTarget>(m,
			"Abstract base class for all data objects."
			"\n\n"
			"Some data objects are associated with a :py:class:`~ovito.vis.Display` object, which is responsible for "
			"displaying the data in the viewports and in rendered images. "
			"The :py:attr:`.display` attribute provides access to the attached display object and "
			"allows controlling the visual appearance of the data.")
		//.def("evaluate", &DataObject::evaluate)
		//.def("objectValidity", &DataObject::objectValidity)
		//.def("addDisplayObject", &DataObject::addDisplayObject)
		//.def("setDisplayObject", &DataObject::setDisplayObject)
		//.def_property("saveWithScene", &DataObject::saveWithScene, &DataObject::setSaveWithScene)
		// Required by FileSource.load():
		.def("wait_until_ready", [](DataObject* obj, TimePoint time) {
			Future<PipelineFlowState> future = obj->evaluateAsync(PipelineEvalRequest(time, false));
			return ScriptEngine::activeTaskManager().waitForTask(future);
		})
		.def_property("display", &DataObject::displayObject, &DataObject::setDisplayObject,
			"The :py:class:`~ovito.vis.Display` object associated with this data object, which is responsible for "
        	"displaying the data. If this field is ``None``, the data is non-visual and doesn't appear in the viewports or rendered images.")
		// This is needed by ovito.io.FileSource.load():
		.def_property_readonly("status", &DataObject::status)
	;
	expose_mutable_subobject_list<DataObject, 
								  DisplayObject, 
								  DataObject,
								  &DataObject::displayObjects, 
								  &DataObject::insertDisplayObject, 
								  &DataObject::removeDisplayObject>(
									  DataObject_py, "display_objects", "DisplayObjectList");

	auto CompoundObject_py = ovito_class<CompoundObject, DataObject>(m, 
			"A data collection is a dictionary-like container that can store an arbitrary number of data objects. "
			"OVITO knows various types of data objects, e.g."
			"\n\n"
			"   * :py:class:`~ovito.data.ParticleProperty` and :py:class:`~ovito.data.ParticleTypeProperty`\n"
			"   * :py:class:`~ovito.data.SimulationCell`\n"
			"   * :py:class:`~ovito.data.Bonds`\n"
			"   * :py:class:`~ovito.data.BondProperty` and :py:class:`~ovito.data.BondTypeProperty`\n"
			"   * :py:class:`~ovito.data.SurfaceMesh`\n"
			"   * :py:class:`~ovito.data.DislocationNetwork`\n"
			"\n\n"
			"Data collections hold the data that enters or leaves an :py:class:`~ovito.ObjectNode`'s modification pipeline. "
			"The *input* data collection of the pipeline can be accessed through the node's :py:attr:`~ovito.ObjectNode.source` attribute::"
			"\n\n"
			"   >>> node = import_file(...)\n"
			"   >>> print(node.source)\n"
			"   DataCollection(['Simulation cell', 'Position'])\n"
			"\n\n"
			"In this example the input data collection contains the original data that was read from the external file, consisting "
			"of the particle position property and a simulation cell."
			"\n\n"
			"The input data typically gets modified or extended by modifiers in the node's modification pipeline. To access the results "
			"of the modification pipeline, we need to call :py:meth:`ObjectNode.compute() <ovito.ObjectNode.compute>`, "
			"which returns the *output* data collection after evaluating the modifiers::"
			"\n\n"
			"   >>> node.modifiers.append(CommonNeighborAnalysisModifier())\n"
			"   >>> print(node.compute())\n"
			"   DataCollection(['Simulation cell', 'Position', 'Color', 'Structure Type'])\n"
			"\n"
			"The output data collection is cached by the :py:class:`~ovito.ObjectNode` and may "
			"subsequently be accessed through the :py:attr:`~ovito.ObjectNode.output` attribute::"
			"\n\n"
			"   >>> print(node.output)\n"
			"   DataCollection(['Simulation cell', 'Position', 'Color', 'Structure Type'])\n"
			"\n\n"
			"In our example, the :py:class:`~ovito.modifiers.CommonNeighborAnalysisModifier` in the modification pipeline "
			"has added additional particle properties to the :py:class:`!DataCollection`. "
			"Particle properties, which are instances of the :py:class:`ParticleProperty` class, are so-called :py:class:`data objects <ovito.data.DataObject>`. "
			"Likewise, the simulation cell (:py:class:`SimulationCell`) and bonds (:py:class:`Bonds`) are data objects, which "
			"can all be part of a data collection. "
			"\n\n"
			"The particle properties in a collection can be accessed through the :py:attr:`.particle_properties` dictionary view. "
			"Use its ``keys()`` method to find out which particle properties are contained in the collection::"
			"\n\n"
			"   >>> data = node.compute()\n"
			"   >>> list(data.particle_properties.keys())\n"
			"   ['Particle Identifier', 'Position', \n"
			"    'Potential Energy', 'Color', 'Structure Type']\n"
			"\n\n"
			"Specific particle properties in the collection can be accessed using the dictionary interface::"
			"\n\n"
			"   >>> data.particle_properties['Potential Energy']\n"
			"   <ParticleProperty at 0x11d01d60>\n"
			"\n\n"
			"Standard particle properties, however, can be directly accessed more conveniently via corresponding Python attributes, e.g.::"
			"\n\n"
			"   >>> data.particle_properties.potential_energy\n"
			"   <ParticleProperty at 0x11d01d60>\n"
			"   \n"
			"   >>> print(data.particle_properties.position.array)\n"
			"   [[ 0.          0.          0.        ]\n"
			"    [ 0.8397975   0.8397975   0.        ]\n"
			"    ...\n"
			"\n\n"
			"The :py:class:`~ovito.data.SimulationCell`, :py:class:`~ovito.data.Bonds`, and other data objects in the "
			"data collection can be accessed through its :py:attr:`.cell`, :py:attr:`.bonds`, :py:attr:`.surface`, and "
			":py:attr:`.dislocations` property::"
			"\n\n"
			"   >>> data.cell\n"
			"   <SimulationCellObject at 0x24338a0>\n\n"
			"   >>> data.cell.matrix\n"
			"   [[ 3.35918999  0.          0.          0.        ]\n"
			"    [ 0.          3.35918999  0.          0.        ]\n"
			"    [ 0.          0.          3.35918999  0.        ]]\n"
			"\n\n",
			// Python class name:
			"DataCollection")
		.def("add", &CompoundObject::addDataObject,
				"add(obj)"
				"\n\n"
				"Inserts a :py:class:`~ovito.data.DataObject` into the :py:class:`!DataCollection`. "
				"\n\n"
				"The method will do nothing if the data object is already part of the collection. "
				"A data object can be part of several data collections. ",
				py::arg("obj"))
		.def("remove", (void (CompoundObject::*)(DataObject*))&CompoundObject::removeDataObject,
				"remove(obj)"
				"\n\n"
				"Removes a :py:class:`~ovito.data.DataObject` from the :py:class:`!DataCollection`. "
				"\n\n"
				"The method will do nothing if the data object is not part of the collection. ",
				py::arg("obj"))
		.def("replace", &CompoundObject::replaceDataObject,
				"replace(old_obj, new_obj)"
				"\n\n"
				"Replaces a :py:class:`~ovito.data.DataObject` in the :py:class:`!DataCollection` with a different one. "
				"\n\n"
				"The method will do nothing if the data object to be replaced is not part of the collection. ",
				py::arg("old_obj"), py::arg("new_obj"))
		// This is needed by ObjectNode.compute():
		.def("set_data_objects", &CompoundObject::setDataObjects)
		.def_property_readonly("attribute_names", [](CompoundObject& obj) -> QStringList {
				return obj.attributes().keys();
			})
		.def("get_attribute", [](CompoundObject& obj, const QString& attrName) -> py::object {
				auto item = obj.attributes().find(attrName);
				if(item == obj.attributes().end())
					return py::none();
				else
					return py::cast(item.value());
			})
		.def("set_attribute", [](CompoundObject& obj, const QString& attrName, py::object value) {
				QVariantMap newAttrs = obj.attributes();
				if(value.is_none()) {
					newAttrs.remove(attrName);
				}
				else {
					if(PyLong_Check(value.ptr()))
						newAttrs.insert(attrName, QVariant::fromValue(PyLong_AsLong(value.ptr())));
					else if(PyFloat_Check(value.ptr()))
						newAttrs.insert(attrName, QVariant::fromValue(PyFloat_AsDouble(value.ptr())));
					else
						newAttrs.insert(attrName, QVariant::fromValue(value.cast<py::str>().cast<QString>()));
				}
				obj.setAttributes(newAttrs);
			})
	;
	expose_mutable_subobject_list<CompoundObject, 
								  DataObject, 
								  CompoundObject,
								  &CompoundObject::dataObjects, 
								  &CompoundObject::insertDataObject, 
								  &CompoundObject::removeDataObjectByIndex>(
									  CompoundObject_py, "objects", "DataCollectionObjectList");

	ovito_abstract_class<Modifier, RefTarget>(m,
			"This is the base class for all modifiers in OVITO.")
		.def_property("enabled", &Modifier::isEnabled, &Modifier::setEnabled,
				"Controls whether the modifier is applied to the input data. Modifiers which are not enabled "
				"are skipped even if they are part of a modification pipeline."
				"\n\n"
				":Default: ``True``\n")
		.def_property_readonly("status", &Modifier::status)
		//.def("modifierValidity", &Modifier::modifierValidity)
		.def_property_readonly("modifier_applications", [](Modifier& mod) -> py::list {
				py::list l;
				for(ModifierApplication* modApp : mod.modifierApplications())
					l.append(py::cast(modApp));
				return l;
			})
		//.def("isApplicableTo", &Modifier::isApplicableTo)
	;

	ovito_class<ModifierApplication, RefTarget>(m)
		.def(py::init<DataSet*, Modifier*>())
		.def_property_readonly("modifier", &ModifierApplication::modifier)
		//.def_property_readonly("pipelineObject", &ModifierApplication::pipelineObject)
		//.def_property_readonly("objectNodes", &ModifierApplication::objectNodes)
	;

	auto PipelineObject_py = ovito_class<PipelineObject, DataObject>(m)
		.def_property("source_object", &PipelineObject::sourceObject, &PipelineObject::setSourceObject)
		.def("insert_modifier", &PipelineObject::insertModifier)
	;
	expose_mutable_subobject_list<PipelineObject, 
								  ModifierApplication, 
								  PipelineObject,
								  &PipelineObject::modifierApplications, 
								  &PipelineObject::insertModifierApplication, 
								  &PipelineObject::removeModifierApplication>(
									  PipelineObject_py, "modifier_applications", "PipelineObjectModifierApplicationList");

	auto SceneNode_py = ovito_abstract_class<SceneNode, RefTarget>(m)
		.def_property("name", &SceneNode::nodeName, &SceneNode::setNodeName)
		.def_property("display_color", &SceneNode::displayColor, &SceneNode::setDisplayColor)
		.def_property_readonly("parent_node", &SceneNode::parentNode)
		.def_property_readonly("lookat_node", &SceneNode::lookatTargetNode)
		.def_property("transform_ctrl", &SceneNode::transformationController, &SceneNode::setTransformationController)
		.def_property_readonly("is_selected", &SceneNode::isSelected)
		.def("delete", &SceneNode::deleteNode)
		//.def("localBoundingBox", &SceneNode::localBoundingBox)
		//.def("worldBoundingBox", &SceneNode::worldBoundingBox)
	;
	expose_mutable_subobject_list<SceneNode, 
								  SceneNode, 
								  SceneNode,
								  &SceneNode::children, 
								  &SceneNode::insertChildNode, 
								  &SceneNode::removeChildNode>(
									  SceneNode_py, "children", "SceneNodeChildren");		
	

	ovito_class<ObjectNode, SceneNode>(m,
			"This class encapsulates a data source, a modification pipeline, and the output of the pipeline."
			"\n\n"
			"An :py:class:`!ObjectNode` is typically created by calling :py:func:`~ovito.io.import_file`. "
			"But you can also create an object node yourself, e.g., to :ref:`build a particle system from scratch <example_creating_particles_programmatically>`."
			"\n\n"
			"Each node has a data source associated with it, which generates or loads the input data of the "
			"modification pipeline. It is accessible through the node's :py:attr:`.source` attribute. "
			"For nodes creates by the :py:func:`~ovito.io.import_file` function, the data source is an instance "
			"of the :py:class:`~ovito.io.FileSource` class, which is responsible for loading the input data "
			"from the external file. Note that :py:class:`~ovito.io.FileSource` is derived from the "
			":py:class:`~ovito.data.DataCollection` base class. Thus, the :py:class:`~ovito.io.FileSource` "
			"also caches the data that it has loaded from the external file and allows you to access or even modify this data. "
			"\n\n"
			"The node's modification pipeline is accessible through the :py:attr:`.modifiers` attribute. "
			"This list is initially empty and you can populate it with new modifier instances (see the :py:mod:`ovito.modifiers` module)."
			"\n\n"
			"Once the modification pipeline is set up, you can request an evaluation of the pipeline, which means that the "
			"all modifiers in the pipeline are applied to the input data one after another. "
			"The output data of this computation is stored in the output cache of the :py:class:`!ObjectNode`, which "
			"is accessible through its :py:attr:`.output` attribute. This :py:class:`~ovito.data.DataCollection`, "
			"which holds the output data, is also the one that is directly returned by the :py:meth:`.compute` method. "
			"\n\n"
			"The following example creates a node by importing a simulation file and inserts a :py:class:`~ovito.modifiers.SliceModifier` to "
			"cut away some of the particles. It then prints the total number of particle in the input and in the output."
			"\n\n"
	        ".. literalinclude:: ../example_snippets/object_node_example.py"
			"\n\n"
			"An :py:class:`!ObjectNode` can be part of the current *scene*, which means that it appears in the viewports and in rendered images. "
			"By default a node is not part of the scene, but you can insert it into the scene with the :py:meth:`.add_to_scene` method. ",
			nullptr, py::dynamic_attr())
		.def_property("data_provider", &ObjectNode::dataProvider, &ObjectNode::setDataProvider)
		.def_property("source", &ObjectNode::sourceObject, &ObjectNode::setSourceObject,
				"The object that provides or generates the data that enters the node's modification pipeline. "
				"This typically is a :py:class:`~ovito.io.FileSource` instance if the node was created by a call to :py:func:`~ovito.io.import_file`.")
		// Required by ObjectNode.wait() and ObjectNode.compute():
		.def("eval_pipeline", [](ObjectNode* node, TimePoint time) {
			return node->evaluatePipelineImmediately(PipelineEvalRequest(time, false));
		})
		.def("wait_until_ready", [](ObjectNode* node, TimePoint time) {
			Future<PipelineFlowState> future = node->evaluatePipelineAsync(PipelineEvalRequest(time, false));
			return ScriptEngine::activeTaskManager().waitForTask(future);
		})
		// Required by ObjectNode.modifiers sequence:
		.def("apply_modifier", &ObjectNode::applyModifier)
	;

	ovito_class<SceneRoot, SceneNode>{m}
		//.def("getNodeByName", &SceneRoot::getNodeByName)
		//.def("makeNameUnique", &SceneRoot::makeNameUnique)
	;

	auto SelectionSet_py = ovito_class<SelectionSet, RefTarget>{m}
		//.def("boundingBox", &SelectionSet::boundingBox)
	;
	expose_mutable_subobject_list<SelectionSet, 
								  SceneNode, 
								  SelectionSet,
								  &SelectionSet::nodes, 
								  &SelectionSet::insert, 
								  &SelectionSet::removeByIndex>(
									  SelectionSet_py, "nodes", "SelectionSetNodes");

	ovito_class<PythonScriptModifier, Modifier>(m,
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"A modifier that executes a Python script function which computes the output of the modifier. "
			"\n\n"
			"This class makes it possible to implement new modifier types in Python which can participate in OVITO's "
			"data pipeline system and which may be used like OVITO's standard built-in modifiers. "
			"You can learn more about the usage of this class in the :ref:`writing_custom_modifiers` section. "
			"\n\n"
			"Example:"
			"\n\n"
			".. literalinclude:: ../example_snippets/python_script_modifier.py")

		.def_property("script", &PythonScriptModifier::script, &PythonScriptModifier::setScript,
				"The source code of the user-defined Python script, which is executed by the modifier and which defines the ``modify()`` function. "
				"Note that this property returns the source code entered by the user through the graphical user interface, not the callable Python function. "
				"\n\n"
				"If you want to set the modification script function from an already running Python script, you should set "
				"the :py:attr:`.function` property instead as demonstrated in the example above.")

		.def_property("function", &PythonScriptModifier::scriptFunction, &PythonScriptModifier::setScriptFunction,
				"The Python function to be called every time the modification pipeline is evaluated by the system."
				"\n\n"
				"The function must have a signature as shown in the example above. "
				"The *frame* parameter contains the current animation frame number at which the data pipeline "
				"is being evaluated. The :py:class:`~ovito.data.DataCollection` *input* holds the "
				"input data objects of the modifier, which were produced by the upstream part of the modification "
				"pipeline. *output* is the :py:class:`~ovito.data.DataCollection` where the modifier function "
				"should store the modified or newly generated data objects. This data objects in this collection flow down the "
				"modification pipeline and are eventually rendered in the viewports. "
				"\n\n"
				"By default the *output* data collection contains the same data objects as the *input* data collection. "
				"Thus, without further action, all data gets passed through the modifier unmodified. "
				"\n\n"
				":Default: ``None``\n")
	;

	ovito_class<TriMeshObject, DataObject>{m}
	;
}

};
