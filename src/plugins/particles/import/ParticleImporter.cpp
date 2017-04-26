///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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

#include <plugins/particles/Particles.h>
#include <core/utilities/io/FileManager.h>
#include <core/utilities/concurrent/Future.h>
#include <core/app/Application.h>
#include <core/dataset/DataSetContainer.h>
#include <core/dataset/importexport/FileSource.h>
#include "ParticleImporter.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(ParticleImporter, FileSourceImporter);
DEFINE_PROPERTY_FIELD(ParticleImporter, isMultiTimestepFile, "IsMultiTimestepFile");
SET_PROPERTY_FIELD_LABEL(ParticleImporter, isMultiTimestepFile, "File contains time series");

/******************************************************************************
* Scans the given external path (which may be a directory and a wild-card pattern,
* or a single file containing multiple frames) to find all available animation frames.
******************************************************************************/
Future<QVector<FileSourceImporter::Frame>> ParticleImporter::discoverFrames(const QUrl& sourceUrl)
{
	if(shouldScanFileForTimesteps(sourceUrl)) {
		return dataset()->container()->taskManager().execAsync(
				std::bind(&ParticleImporter::discoverFramesInFile, this, sourceUrl, std::placeholders::_1));
	}
	else {
		return FileSourceImporter::discoverFrames(sourceUrl);
	}
}

/******************************************************************************
* Scans the input file for simulation timesteps.
******************************************************************************/
QVector<FileSourceImporter::Frame> ParticleImporter::discoverFramesInFile(const QUrl sourceUrl, PromiseBase& promise)
{
	QVector<FileSourceImporter::Frame> result;

	// Check if filename is a wildcard pattern.
	// If yes, find all matching files and scan each one of them.
	QFileInfo fileInfo(sourceUrl.path());
	if(fileInfo.fileName().contains('*') || fileInfo.fileName().contains('?')) {
		auto findFilesFuture = FileSourceImporter::findWildcardMatches(sourceUrl, dataset()->container());
		if(!promise.waitForSubTask(findFilesFuture))
			return result;
		for(auto item : findFilesFuture.result()) {
			result += discoverFramesInFile(item.sourceFile, promise);
		}
		return result;
	}

	promise.setProgressText(tr("Scanning file %1").arg(sourceUrl.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));

	// Fetch file.
	Future<QString> fetchFileFuture = Application::instance()->fileManager()->fetchUrl(*dataset()->container(), sourceUrl);
	if(!promise.waitForSubTask(fetchFileFuture))
		return result;

	// Open file.
	QFile file(fetchFileFuture.result());
	CompressedTextReader stream(file, sourceUrl.path());

	// Scan file.
	try {
		scanFileForTimesteps(promise, result, sourceUrl, stream);
	}
	catch(const Exception&) {
		// Silently ignore parsing and I/O errors if at least two frames have been read.
		// Keep all frames read up to where the error occurred.
		if(result.size() <= 1)
			throw;
		else
			result.pop_back();		// Remove last discovered frame because it may be corrupted or only partially written.
	}

	return result;
}

/******************************************************************************
* Scans the given input file to find all contained simulation frames.
******************************************************************************/
void ParticleImporter::scanFileForTimesteps(PromiseBase& promise, QVector<FileSourceImporter::Frame>& frames, const QUrl& sourceUrl, CompressedTextReader& stream)
{
	// By default, register a single frame.
	QFileInfo fileInfo(stream.filename());
	frames.push_back({ sourceUrl, 0, 0, fileInfo.lastModified(), fileInfo.fileName() });
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void ParticleImporter::propertyChanged(const PropertyFieldDescriptor& field)
{
	if(field == PROPERTY_FIELD(isMultiTimestepFile)) {
		// Automatically rescan input file for animation frames when this option has been activated.
		requestFramesUpdate();
	}
	FileSourceImporter::propertyChanged(field);
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
