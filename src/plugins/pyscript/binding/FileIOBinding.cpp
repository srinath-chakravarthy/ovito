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
#include <core/utilities/io/FileManager.h>
#include <core/utilities/concurrent/ProgressDisplay.h>
#include "PythonBinding.h"

namespace PyScript {

using namespace boost::python;
using namespace Ovito;

BOOST_PYTHON_MODULE(PyScriptFileIO)
{
	docstring_options docoptions(true, false);

	class_<QUrl>("QUrl", init<>())
		.def(init<const QString&>())
		.def("clear", &QUrl::clear)
		.add_property("errorString", &QUrl::errorString)
		.add_property("isEmpty", &QUrl::isEmpty)
		.add_property("isLocalFile", &QUrl::isLocalFile)
		.add_property("isValid", &QUrl::isValid)
        .def("__str__", lambda_address([](const QUrl& url) { return url.toString(QUrl::PreferLocalFile); }))
		.def(self == other<QUrl>())
		.def(self != other<QUrl>())
	;

	// Install automatic Python string to QUrl conversion.
	auto convertible_QUrl = [](PyObject* obj_ptr) -> void* {
		// Check if Python object can be converted to target type.
		extract<QString> ex(obj_ptr);
		if(!ex.check()) return nullptr;
		return obj_ptr;
	};
	auto construct_QUrl = [](PyObject* obj_ptr, converter::rvalue_from_python_stage1_data* data) {
		QString value = extract<QString>(obj_ptr);
		void* storage = ((converter::rvalue_from_python_storage<QUrl>*)data)->storage.bytes;
		new (storage) QUrl(FileManager::instance().urlFromUserInput(value));
		data->convertible = storage;
	};
	converter::registry::push_back(convertible_QUrl, construct_QUrl, type_id<QUrl>());

	ovito_abstract_class<FileImporter, RefTarget>()
		.add_property("fileFilter", &FileImporter::fileFilter)
		.add_property("fileFilterDescription", &FileImporter::fileFilterDescription)
		.def("importFile", &FileImporter::importFile)
		.def("isReplaceExistingPossible", &FileImporter::isReplaceExistingPossible)
		.def("checkFileFormat", &FileImporter::checkFileFormat)
		.def("autodetectFileFormat", (OORef<FileImporter> (*)(DataSet*, const QUrl&))&FileImporter::autodetectFileFormat)
		.staticmethod("autodetectFileFormat")
	;

	enum_<FileImporter::ImportMode>("ImportMode")
		.value("AddToScene", FileImporter::AddToScene)
		.value("ReplaceSelected", FileImporter::ReplaceSelected)
		.value("ResetScene", FileImporter::ResetScene)
	;

	class_<FileManager, boost::noncopyable>("FileManager", no_init)
		.add_static_property("instance", make_function(&FileManager::instance, return_value_policy<reference_existing_object>()))
		.def("removeFromCache", &FileManager::removeFromCache)
		.def("urlFromUserInput", &FileManager::urlFromUserInput)
	;

	ovito_abstract_class<FileSourceImporter, FileImporter>()
		.def("requestReload", &FileSourceImporter::requestReload)
		.def("requestFramesUpdate", &FileSourceImporter::requestFramesUpdate)
	;

	ovito_abstract_class<FileExporter, RefTarget>()
		.add_property("fileFilter", &FileExporter::fileFilter)
		.add_property("fileFilterDescription", &FileExporter::fileFilterDescription)
		.add_property("output_filename", make_function(&FileExporter::outputFilename, return_value_policy<copy_const_reference>()), &FileExporter::setOutputFilename)
		.add_property("multiple_frames", &FileExporter::exportAnimation, &FileExporter::setExportAnimation)
		.add_property("use_wildcard_filename", &FileExporter::useWildcardFilename, &FileExporter::setUseWildcardFilename)
		.add_property("wildcard_filename", make_function(&FileExporter::wildcardFilename, return_value_policy<copy_const_reference>()), &FileExporter::setWildcardFilename)
		.add_property("start_frame", &FileExporter::startFrame, &FileExporter::setStartFrame)
		.add_property("end_frame", &FileExporter::endFrame, &FileExporter::setEndFrame)
		.add_property("every_nth_frame", &FileExporter::everyNthFrame, &FileExporter::setEveryNthFrame)
		.def("setOutputData", &FileExporter::setOutputData)
		.def("exportNodes", &FileExporter::exportNodes)
	;

	ovito_class<FileSource, CompoundObject>(
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
			"**Example**"
			"\n\n"
			"The following script receives a list of simulation files on the command line. It loads them one by one and performs a common neighbor analysis "
			"to determine the number of face-centered cubic atoms in each structure:"
			"\n\n"
			".. literalinclude:: ../example_snippets/cna_process_file_set.py\n"
			"\n"
			"**Data access**"
			"\n\n"
		    "The :py:class:`!FileSource` class is derived from the :py:class:`~ovito.data.DataCollection` base class. "
		    "This means the file source also stores the data loaded from the external file, and you can access this data through the :py:class:`~ovito.data.DataCollection` interface. "
			"Note that the cached data represents the outcome of the most recent successful loading operation and may change every time a new simulation frame is "
			"loaded (see :py:attr:`.loaded_frame`)."
			"\n\n"
			".. literalinclude:: ../example_snippets/file_source_data_access.py\n"
			)
		.add_property("importer", make_function(&FileSource::importer, return_value_policy<ovito_object_reference>()))
		.add_property("source_path", make_function(&FileSource::sourceUrl, return_value_policy<copy_const_reference>()))
		.add_property("num_frames", &FileSource::numberOfFrames,
				"The total number of frames the imported file or file sequence contains (read-only).")
		.add_property("loaded_frame", &FileSource::loadedFrameIndex,
				"The zero-based frame index that is currently loaded into memory by the :py:class:`!FileSource` (read-only). "
				"\n\n"
				"The content of this frame is accessible through the inherited :py:class:`~ovito.data.DataCollection` interface.")
		.add_property("adjust_animation_interval", &FileSource::adjustAnimationIntervalEnabled, &FileSource::setAdjustAnimationIntervalEnabled,
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
		.def("refreshFromSource", &FileSource::refreshFromSource)
		.def("updateFrames", &FileSource::updateFrames)
		.def("animationTimeToInputFrame", &FileSource::animationTimeToInputFrame)
		.def("inputFrameToAnimationTime", &FileSource::inputFrameToAnimationTime)
		.def("adjustAnimationInterval", &FileSource::adjustAnimationInterval)
		.def("setSource", &FileSource::setSource)
	;
}

OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(PyScriptFileIO);

};
