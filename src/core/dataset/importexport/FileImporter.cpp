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

#include <core/Core.h>
#include <core/plugins/PluginManager.h>
#include <core/utilities/io/FileManager.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/DataSetContainer.h>
#include <core/dataset/importexport/FileSourceImporter.h>
#include <core/app/Application.h>
#include "FileImporter.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(DataIO)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(FileImporter, RefTarget);

/******************************************************************************
* Return the list of available import services.
******************************************************************************/
QVector<OvitoObjectType*> FileImporter::availableImporters()
{
	return PluginManager::instance().listClasses(FileImporter::OOType);
}

/******************************************************************************
* Tries to detect the format of the given file.
******************************************************************************/
OORef<FileImporter> FileImporter::autodetectFileFormat(DataSet* dataset, const QUrl& url)
{
	if(!url.isValid())
		dataset->throwException(tr("Invalid path or URL."));

	try {
		DataSetContainer* container = dataset->container();
		OVITO_ASSERT(container != nullptr);

		// Resolve filename if it contains a wildcard.
		Future<QVector<FileSourceImporter::Frame>> framesFuture = FileSourceImporter::findWildcardMatches(url, container);
		if(!container->taskManager().waitForTask(framesFuture))
			dataset->throwException(tr("Operation has been canceled by the user."));
		QVector<FileSourceImporter::Frame> frames = framesFuture.result();
		if(frames.empty())
			dataset->throwException(tr("There are no files in the directory matching the filename pattern."));

		// Download file so we can determine its format.
		Future<QString> fetchFileFuture = Application::instance()->fileManager()->fetchUrl(*container, frames.front().sourceFile);
		if(!container->taskManager().waitForTask(fetchFileFuture))
			dataset->throwException(tr("Operation has been canceled by the user."));

		// Detect file format.
		return autodetectFileFormat(dataset, fetchFileFuture.result(), frames.front().sourceFile.path());
	}
	catch(Exception& ex) {
		// Provide a context object for any errors that occur during file inspection.
		ex.setContext(dataset);
		throw;
	}
}

/******************************************************************************
* Tries to detect the format of the given file.
******************************************************************************/
OORef<FileImporter> FileImporter::autodetectFileFormat(DataSet* dataset, const QString& localFile, const QUrl& sourceLocation)
{
	UndoSuspender noUnder(dataset);
	for(const OvitoObjectType* importerType : availableImporters()) {
		try {
			OORef<FileImporter> importer = static_object_cast<FileImporter>(importerType->createInstance(dataset));
			QFile file(localFile);
			if(importer && importer->checkFileFormat(file, sourceLocation)) {
				return importer;
			}
		}
		catch(const Exception&) {
			// Ignore errors that occur during file format detection.
		}
	}
	return nullptr;
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
