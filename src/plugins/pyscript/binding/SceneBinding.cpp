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
			"   * :py:class:`~ovito.data.SurfaceMesh`\n"
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
			"   >>> node.modifiers.append(CommonNeighborAnalysisModifier(adaptive_mode=True))\n"
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
			"Particle properties, which are instances of the :py:class:`ParticleProperty` class, are so-called :py:class:`~ovito.data.DataObject`s. "
			"Likewise, the simulation cell (:py:class:`SimulationCell`) and bonds (:py:class:`Bonds`) are data objects, which "
			"can all be part of a data collection. "
			"\n\n"
			"Individual data objects in a collection can be accessed via key strings. Use the :py:meth:`.keys` method to "
			"find out which data objects are in the collection::"
			"\n\n"
			"   >>> data = node.compute()\n"
			"   >>> data.keys()\n"
			"   ['Simulation cell', 'Particle identifiers', 'Particle positions', \n"
			"    'Potential Energy', 'Particle colors', 'Structure types']\n"
			"\n\n"
			"Specific data objects in the collection can be accessed using the dictionary interface::"
			"\n\n"
			"   >>> data['Potential Energy']\n"
			"   <ParticleProperty at 0x11d01d60>\n"
			"\n\n"
			"More conveniently, however, standard particle properties, the simulation cell, and the bonds list can be directly accessed as object attributes, e.g.::"
			"\n\n"
			"   >>> data.potential_energy\n"
			"   <ParticleProperty at 0x11d01d60>\n"
			"   \n"
			"   >>> print(data.position.array)\n"
			"   [[ 0.          0.          0.        ]\n"
			"    [ 0.8397975   0.8397975   0.        ]\n"
			"    ...\n\n"
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
				"Inserts a :py:class:`~ovito.data.DataObject` into the :py:class:`!DataCollection`. "
				"\n\n"
				"The method will do nothing if the data object is already part of the collection. "
				"A data object can be part of several data collections. ",
				args("self", "obj"))
		.def("remove", &CompoundObject::removeDataObject,
				"Removes a :py:class:`~ovito.data.DataObject` from the :py:class:`!DataCollection`. "
				"\n\n"
				"The method will do nothing if the data object is not part of the collection. ",
				args("self", "obj"))
		.def("setDataObjects", &CompoundObject::setDataObjects)
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
		.add_property("displayColor", make_function(&SceneNode::displayColor, return_value_policy<copy_const_reference>()), &SceneNode::setDisplayColor)
		.add_property("parentNode", make_function(&SceneNode::parentNode, return_value_policy<ovito_object_reference>()))
		.add_property("children", make_function(&SceneNode::children, return_internal_reference<>()))
		.add_property("lookatTargetNode", make_function(&SceneNode::lookatTargetNode, return_value_policy<ovito_object_reference>()))
		.add_property("isSelected", &SceneNode::isSelected)
		.def("delete", &SceneNode::deleteNode)
		.def("addChild", &SceneNode::addChild)
		.def("insertChild", &SceneNode::insertChild)
		.def("removeChild", &SceneNode::removeChild)
		.def("localBoundingBox", &SceneNode::localBoundingBox)
		.def("worldBoundingBox", make_function(&SceneNode::worldBoundingBox, return_value_policy<copy_const_reference>()))
	;

	ovito_class<ObjectNode, SceneNode>(
			"Manages a data source, a modification pipeline, and the output of the pipeline."
			"\n\n"
			"An :py:class:`!ObjectNode` is created when a new object is inserted into the scene, for example, "
			"as a result of calling py:func:`~ovito.io.import_file`."
			"The node maintains a modification pipeline, which allows applying modifiers to the input data. "
			"The output of the modification pipeline is displayed by the :py:class:`!ObjectNode` in the three-dimensional scene "
			"shown by the viewport."
			"\n\n"
			"The data entering the modification pipeline is provided by the node's :py:attr:`ObjectNode.source` object,"
			"which can be a :py:class:`~ovito.io.FileSource` or a :py:class:`~ovito.data.DataCollection`, for example. "
			"The node's modification pipeline can be accessed through the :py:attr:`ObjectNode.modifiers` attribute. "
			"An evaluation of the modification pipeline can be requested by calling the :py:meth:`ObjectNode.compute` method. "
			"Finally, the cached output of the pipeline can be accessed through the the node's :py:attr:`ObjectNode.output` attribute. "
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
}

OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(PyScriptScene);

};
