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
#include <core/scene/pipeline/Modifier.h>
#include <core/scene/pipeline/ModifierApplication.h>
#include <core/scene/pipeline/PipelineObject.h>
#include <core/utilities/concurrent/ProgressDisplay.h>
#include <plugins/pyscript/extensions/PythonScriptModifier.h>
#include "PythonBinding.h"

namespace PyScript {

using namespace boost::python;
using namespace Ovito;

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(ObjectNode_waitUntilReady_overloads, waitUntilReady, 2, 3);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(DataObject_waitUntilReady_overloads, waitUntilReady, 2, 3);

BOOST_PYTHON_MODULE(PyScriptScene)
{
	docstring_options docoptions(true, false);

	{
		scope s = class_<PipelineStatus>("PipelineStatus", init<optional<PipelineStatus::StatusType, const QString&>>())
			.add_property("type", &PipelineStatus::type)
			.add_property("text", make_function(&PipelineStatus::text, return_value_policy<copy_const_reference>()))
			.def(self == PipelineStatus())
			.def(self != PipelineStatus())
		;

		enum_<PipelineStatus::StatusType>("Type")
			.value("Success", PipelineStatus::Success)
			.value("Warning", PipelineStatus::Warning)
			.value("Error", PipelineStatus::Error)
			.value("Pending", PipelineStatus::Pending)
		;
	}

	class_<PipelineFlowState>("PipelineFlowState", init<>())
		.def(init<DataObject*, TimeInterval>())
		.def(init<const PipelineStatus&, const QVector<DataObject*>&, const TimeInterval&>())
		.def("clear", &PipelineFlowState::clear)
		.def("addObject", &PipelineFlowState::addObject)
		.def("replaceObject", &PipelineFlowState::replaceObject)
		.def("removeObject", &PipelineFlowState::removeObject)
		.def("cloneObjectsIfNeeded", &PipelineFlowState::cloneObjectsIfNeeded)
		.add_property("isEmpty", &PipelineFlowState::isEmpty)
		.add_property("status", make_function(&PipelineFlowState::status, return_internal_reference<>()), &PipelineFlowState::setStatus)
		.add_property("objects", make_function(&PipelineFlowState::objects, return_internal_reference<>()))
	;

	ovito_abstract_class<DataObject, RefTarget>(
			"Abstract base class for all data objects."
			"\n\n"
			"Some data objects are associated with a :py:class:`~ovito.vis.Display` object, which is responsible for "
			"displaying the data in the viewports and in rendered images. "
			"The :py:attr:`.display` attribute provides access to the attached display object and "
			"allows controlling the visual appearance of the data.")
		.def("objectValidity", &DataObject::objectValidity)
		.def("evaluate", &DataObject::evaluate)
		.def("addDisplayObject", &DataObject::addDisplayObject)
		.def("setDisplayObject", &DataObject::setDisplayObject)
		.add_property("status", &DataObject::status)
		.add_property("displayObjects", make_function(&DataObject::displayObjects, return_internal_reference<>()))
		.add_property("saveWithScene", &DataObject::saveWithScene, &DataObject::setSaveWithScene)
		.def("waitUntilReady", &DataObject::waitUntilReady, DataObject_waitUntilReady_overloads())
	;
	register_ptr_to_python<VersionedOORef<DataObject>>();

	ovito_class<CompoundObject, DataObject>(
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
		.add_property("objects", make_function(&CompoundObject::dataObjects, return_internal_reference<>()))
		.def("add", &CompoundObject::addDataObject,
				"add(obj)"
				"\n\n"
				"Inserts a :py:class:`~ovito.data.DataObject` into the :py:class:`!DataCollection`. "
				"\n\n"
				"The method will do nothing if the data object is already part of the collection. "
				"A data object can be part of several data collections. ",
				args("self", "obj"))
		.def("remove", &CompoundObject::removeDataObject,
				"remove(obj)"
				"\n\n"
				"Removes a :py:class:`~ovito.data.DataObject` from the :py:class:`!DataCollection`. "
				"\n\n"
				"The method will do nothing if the data object is not part of the collection. ",
				(arg("obj")))
		.def("replace", &CompoundObject::replaceDataObject,
				"replace(old_obj, new_obj)"
				"\n\n"
				"Replaces a :py:class:`~ovito.data.DataObject` in the :py:class:`!DataCollection` with a different one. "
				"\n\n"
				"The method will do nothing if the data object to be replaced is not part of the collection. ",
				(arg("old_obj"), arg("new_obj")))
		.def("setDataObjects", &CompoundObject::setDataObjects)
		.add_property("attributes", static_cast<dict (*)(CompoundObject&)>([](CompoundObject& obj) {
				dict a;
				for(auto entry = obj.attributes().constBegin(); entry != obj.attributes().constEnd(); ++entry) {
					switch(static_cast<QMetaType::Type>(entry.value().type())) {
						case QMetaType::Bool: a[entry.key()] = entry.value().toBool(); break;
						case QMetaType::Int: a[entry.key()] = entry.value().toInt(); break;
						case QMetaType::UInt: a[entry.key()] = entry.value().toUInt(); break;
						case QMetaType::Double: a[entry.key()] = entry.value().toDouble(); break;
						case QMetaType::Float: a[entry.key()] = entry.value().toFloat(); break;
						case QMetaType::QString: a[entry.key()] = entry.value().toString(); break;
						default: continue;
					}
				}
				return a;
			}),
			"Returns a dictionary object containing additional metadata, which describes the contents of the :py:class:`!DataCollection`. "
			"\n\n"
			"If the particle dataset has been loaded from a LAMMPS dump file, for example, which contains the simulation timestep number, this "
			"timestep information can be retrieved from the attributes dictionary as follows::"
			"\n\n"
			"   >>> data_collection = ovito.dataset.selected_node.compute()\n"
			"   >>> data_collection.attributes['Timestep']\n"
			"   140000\n"
			"\n\n"
			"For *extended XYZ* files, the attributes dictionary contains all key/value pairs found in the comment line of the input file.\n")
	;

	ovito_abstract_class<Modifier, RefTarget>(
			"This is the base class for all modifiers in OVITO.")
		.add_property("enabled", &Modifier::isEnabled, &Modifier::setEnabled,
				"Controls whether the modifier is applied to the input data. Modifiers which are not enabled "
				"are skipped even if they are part of a modification pipeline."
				"\n\n"
				":Default: ``True``\n")
		.add_property("status", &Modifier::status)
		.def("modifierValidity", &Modifier::modifierValidity)
		.def("modifierApplications", &Modifier::modifierApplications)
		.def("isApplicableTo", &Modifier::isApplicableTo)
	;

	ovito_class<ModifierApplication, RefTarget>()
		.def(init<DataSet*, Modifier*>())
		.add_property("modifier", make_function(&ModifierApplication::modifier, return_value_policy<ovito_object_reference>()))
		.add_property("pipelineObject", make_function(&ModifierApplication::pipelineObject, return_value_policy<ovito_object_reference>()))
		.add_property("objectNodes", &ModifierApplication::objectNodes)
	;

	ovito_class<PipelineObject, DataObject>()
		.add_property("source_object", make_function(&PipelineObject::sourceObject, return_value_policy<ovito_object_reference>()), &PipelineObject::setSourceObject)
		.add_property("modifierApplications", make_function(&PipelineObject::modifierApplications, return_internal_reference<>()))
		.def("insertModifier", make_function(&PipelineObject::insertModifier, return_value_policy<ovito_object_reference>()))
		.def("insertModifierApplication", &PipelineObject::insertModifierApplication)
		.def("removeModifier", &PipelineObject::removeModifier)
	;

	ovito_abstract_class<SceneNode, RefTarget>()
		.add_property("name", make_function(&SceneNode::name, return_value_policy<copy_const_reference>()), &SceneNode::setName)
		.add_property("display_color", make_function(&SceneNode::displayColor, return_value_policy<copy_const_reference>()), &SceneNode::setDisplayColor)
		.add_property("parent_node", make_function(&SceneNode::parentNode, return_value_policy<ovito_object_reference>()))
		.add_property("children", make_function(&SceneNode::children, return_internal_reference<>()))
		.add_property("lookat_node", make_function(&SceneNode::lookatTargetNode, return_value_policy<ovito_object_reference>()))
		.add_property("transform_ctrl", make_function(&SceneNode::transformationController, return_value_policy<ovito_object_reference>()), &SceneNode::setTransformationController)
		.add_property("is_selected", &SceneNode::isSelected)
		.def("delete", &SceneNode::deleteNode)
		.def("addChild", &SceneNode::addChild)
		.def("insertChild", &SceneNode::insertChild)
		.def("removeChild", &SceneNode::removeChild)
		.def("localBoundingBox", &SceneNode::localBoundingBox)
		.def("worldBoundingBox", make_function(&SceneNode::worldBoundingBox, return_value_policy<copy_const_reference>()))
	;

	ovito_class<ObjectNode, SceneNode>(
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
			"By default a node is not part of the scene, but you can insert it into the scene with the :py:meth:`.add_to_scene` method. "
			)
		.add_property("data_provider", make_function(&ObjectNode::dataProvider, return_value_policy<ovito_object_reference>()), &ObjectNode::setDataProvider)
		.add_property("source", make_function(&ObjectNode::sourceObject, return_value_policy<ovito_object_reference>()), &ObjectNode::setSourceObject,
				"The object that provides or generates the data that enters the node's modification pipeline. "
				"This typically is a :py:class:`~ovito.io.FileSource` instance if the node was created by a call to :py:func:`~ovito.io.import_file`.")
		.add_property("displayObjects", make_function(&ObjectNode::displayObjects, return_internal_reference<>()))
		.def("evalPipeline", make_function(&ObjectNode::evalPipeline, return_value_policy<copy_const_reference>()))
		.def("applyModifier", &ObjectNode::applyModifier)
		.def("waitUntilReady", &ObjectNode::waitUntilReady, ObjectNode_waitUntilReady_overloads())
	;

	ovito_class<SceneRoot, SceneNode>()
		.def("getNodeByName", make_function(&SceneRoot::getNodeByName, return_value_policy<ovito_object_reference>()))
		.def("makeNameUnique", &SceneRoot::makeNameUnique)
	;

	ovito_class<SelectionSet, RefTarget>()
		.add_property("size", &SelectionSet::size)
		.add_property("empty", &SelectionSet::empty)
		.add_property("front", make_function(&SelectionSet::front, return_value_policy<ovito_object_reference>()))
		.add_property("nodes", make_function(&SelectionSet::nodes, return_internal_reference<>()))
		.def("contains", &SelectionSet::contains)
		.def("push_back", &SelectionSet::push_back)
		.def("clear", &SelectionSet::clear)
		.def("remove", &SelectionSet::remove)
		.def("boundingBox", &SelectionSet::boundingBox)
		.def("setNode", &SelectionSet::setNode)
	;

	ovito_class<PythonScriptModifier, Modifier>(
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

		.add_property("script", make_function(&PythonScriptModifier::script, return_value_policy<copy_const_reference>()), &PythonScriptModifier::setScript,
				"The source code of the user-defined Python script, which is executed by the modifier and which defines the ``modify()`` function. "
				"Note that this property returns the source code entered by the user through the graphical user interface, not the callable Python function. "
				"\n\n"
				"If you want to set the modification script function from an already running Python script, you should set "
				"the :py:attr:`.function` property instead as demonstrated in the example above.")

		.add_property("function", &PythonScriptModifier::scriptFunction, &PythonScriptModifier::setScriptFunction,
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
}

OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(PyScriptScene);

};
