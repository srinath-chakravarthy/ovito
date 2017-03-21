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
#include <core/dataset/importexport/FileImporter.h>
#include <core/dataset/importexport/FileExporter.h>
#include <core/dataset/importexport/FileSourceImporter.h>
#include <core/dataset/importexport/FileSource.h>
#include <core/dataset/importexport/AttributeFileExporter.h>
#include <core/utilities/io/FileManager.h>
#include <core/utilities/concurrent/TaskManager.h>
#include "PythonBinding.h"

namespace PyScript {

using namespace Ovito;

void defineIOSubmodule(py::module parentModule)
{
	py::module m = parentModule.def_submodule("IO");

	ovito_abstract_class<FileImporter, RefTarget>{m}
		// These are needed by ovito.io.import_file():
		.def("import_file", &FileImporter::importFile)
		.def_static("autodetect_format", (OORef<FileImporter> (*)(DataSet*, const QUrl&))&FileImporter::autodetectFileFormat)

		//.def_property_readonly("fileFilter", &FileImporter::fileFilter)
		//.def_property_readonly("fileFilterDescription", &FileImporter::fileFilterDescription)		
		//.def("isReplaceExistingPossible", &FileImporter::isReplaceExistingPossible)
		//.def("checkFileFormat", &FileImporter::checkFileFormat)
	;

	py::enum_<FileImporter::ImportMode>(m, "ImportMode")
		.value("AddToScene", FileImporter::AddToScene)
		.value("ReplaceSelected", FileImporter::ReplaceSelected)
		.value("ResetScene", FileImporter::ResetScene)
	;

	//py::class_<FileManager>(m, "FileManager", py::metaclass())
		//.def_property_readonly_static("instance", py::cpp_function(
		//	[](py::object /*self*/) { return Application::instance()->fileManager(); }, py::return_value_policy::reference))
		//.def("removeFromCache", &FileManager::removeFromCache)
		//.def("urlFromUserInput", &FileManager::urlFromUserInput)
	//;

	ovito_abstract_class<FileSourceImporter, FileImporter>{m}
		//.def("requestReload", &FileSourceImporter::requestReload)
		//.def("requestFramesUpdate", &FileSourceImporter::requestFramesUpdate)
	;

	ovito_abstract_class<FileExporter, RefTarget>(m)
		//.def_property_readonly("fileFilter", &FileExporter::fileFilter)
		//.def_property_readonly("fileFilterDescription", &FileExporter::fileFilterDescription)
		.def_property("output_filename", &FileExporter::outputFilename, &FileExporter::setOutputFilename)
		.def_property("multiple_frames", &FileExporter::exportAnimation, &FileExporter::setExportAnimation)
		.def_property("use_wildcard_filename", &FileExporter::useWildcardFilename, &FileExporter::setUseWildcardFilename)
		.def_property("wildcard_filename", &FileExporter::wildcardFilename, &FileExporter::setWildcardFilename)
		.def_property("start_frame", &FileExporter::startFrame, &FileExporter::setStartFrame)
		.def_property("end_frame", &FileExporter::endFrame, &FileExporter::setEndFrame)
		.def_property("every_nth_frame", &FileExporter::everyNthFrame, &FileExporter::setEveryNthFrame)
		// Required by ovito.io.export_file():
		.def("set_node", [](FileExporter& exporter, SceneNode* node) { exporter.setOutputData({ node }); })
		.def("export_nodes", &FileExporter::exportNodes)
		.def("select_standard_output_data", &FileExporter::selectStandardOutputData)		
	;

	ovito_class<AttributeFileExporter, FileExporter>(m)
		.def_property("columns", &AttributeFileExporter::attributesToExport, &AttributeFileExporter::setAttributesToExport)
	;

	ovito_class<FileSource, CompoundObject>(m, 
			":Base class: :py:class:`ovito.data.DataCollection`\n\n"
			"This object serves as a data source for modification pipelines and is responsible for reading the input data from one or more external files."
			"\n\n"
			"You normally do not create an instance of this class yourself. "
			"The :py:func:`ovito.io.import_file` function does it for you and assigns the file source to the :py:attr:`~ovito.ObjectNode.source` "
			"attribute of the returned :py:class:`~ovito.ObjectNode`. "
			"This file source loads data from the external file given by the :py:attr:`.source_path` attribute. The :py:class:`~ovito.ObjectNode` "
			"then takes this data and feeds it into its modification pipeline."
			"\n\n"
			"You typically don't set the :py:attr:`.source_path` attribute directly. "
			"Instead, use the :py:meth:`FileSource.load` method to load a different input file and hook it into an existing modification pipeline:"
			"\n\n"
			".. literalinclude:: ../example_snippets/file_source_load_method.py\n"
			"\n"
			"File sources are also used by certain modifiers to load a reference configuration, e.g. by the :py:class:`~ovito.modifiers.CalculateDisplacementsModifier`, "
			"whose :py:attr:`~ovito.modifiers.CalculateDisplacementsModifier.reference` attribute also contains a :py:class:`!FileSource`.\n"
			"\n\n"
			"**Data access**"
			"\n\n"
		    "The :py:class:`!FileSource` class is derived from the :py:class:`~ovito.data.DataCollection` base class. "
		    "This means the file source also stores the data loaded from the external file, and you can access this data through the :py:class:`~ovito.data.DataCollection` base class interface. "
			"Note that the cached data represents the outcome of the most recent successful loading operation and may change every time a new simulation frame is "
			"loaded (see :py:attr:`.loaded_frame`)."
			"\n\n"
			".. literalinclude:: ../example_snippets/file_source_data_access.py\n"
			)
		.def_property_readonly("importer", &FileSource::importer)
		.def_property_readonly("source_path", &FileSource::sourceUrl)
		.def("set_source", &FileSource::setSource)
		.def_property_readonly("num_frames", &FileSource::numberOfFrames,
				"The total number of frames the imported file or file sequence contains (read-only).")
		.def_property_readonly("loaded_frame", &FileSource::loadedFrameIndex,
				"The zero-based frame index that is currently loaded into memory by the :py:class:`!FileSource` (read-only). "
				"\n\n"
				"The content of this frame is accessible through the inherited :py:class:`~ovito.data.DataCollection` interface.")
		.def_property("adjust_animation_interval", &FileSource::adjustAnimationIntervalEnabled, &FileSource::setAdjustAnimationIntervalEnabled,
				"A flag that controls whether the animation length in OVITO is automatically adjusted to match the number of frames in the "
				"loaded file or file sequence."
				"\n\n"
				"The current length of the animation in OVITO is managed by the global :py:class:`~ovito.anim.AnimationSettings` object. The number of frames in the external file "
				"or file sequence is indicated by the :py:attr:`.num_frames` attribute of this :py:class:`!FileSource`. If :py:attr:`.adjust_animation_interval` "
				"is ``True``, then the animation length will be automatically adjusted to match the number of frames provided by the :py:class:`!FileSource`. "
				"\n\n"
				"In some situations it makes sense to turn this option off, for example, if you import several data files into "
				"OVITO simultaneously, but their frame counts do not match. "
				"\n\n"
				":Default: ``True``\n")
		//.def("refreshFromSource", &FileSource::refreshFromSource)
		//.def("updateFrames", &FileSource::updateFrames)
		//.def("animationTimeToInputFrame", &FileSource::animationTimeToInputFrame)
		//.def("inputFrameToAnimationTime", &FileSource::inputFrameToAnimationTime)
	;
}

};
