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
#include <core/scene/ObjectNode.h>
#include <core/scene/SceneRoot.h>
#include <core/scene/objects/DataObject.h>
#include <core/scene/SelectionSet.h>
#include <core/dataset/DataSetContainer.h>
#include <core/animation/AnimationSettings.h>
#include <core/viewport/ViewportConfiguration.h>
#include <core/utilities/io/FileManager.h>
#include <core/app/Application.h>
#include "FileSourceImporter.h"
#include "FileSource.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(DataIO)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(FileSourceImporter, FileImporter);

/******************************************************************************
* Sends a request to the FileSource owning this importer to reload
* the input file.
******************************************************************************/
void FileSourceImporter::requestReload(int frame)
{
	// Retrieve the FileSource that owns this importer by looking it up in the list of dependents.
	for(RefMaker* refmaker : dependents()) {
		if(FileSource* fileSource = dynamic_object_cast<FileSource>(refmaker)) {
			try {
				fileSource->refreshFromSource(frame);
			}
			catch(const Exception& ex) {
				ex.reportError();
			}
		}
	}
}

/******************************************************************************
* Sends a request to the FileSource owning this importer to refresh the
* animation frame sequence.
******************************************************************************/
void FileSourceImporter::requestFramesUpdate()
{
	// Retrieve the FileSource that owns this importer by looking it up in the list of dependents.
	for(RefMaker* refmaker : dependents()) {
		if(FileSource* fileSource = dynamic_object_cast<FileSource>(refmaker)) {
			try {
				// If wildcard pattern search has been disabled, replace
				// wildcard pattern URL with an actual filename.
				if(!autoGenerateWildcardPattern()) {
					QFileInfo fileInfo(fileSource->sourceUrl().path());
					if(fileInfo.fileName().contains('*') || fileInfo.fileName().contains('?')) {
						if(fileSource->loadedFrameIndex() >= 0 && fileSource->loadedFrameIndex() < fileSource->frames().size()) {
							QUrl currentUrl = fileSource->frames()[fileSource->loadedFrameIndex()].sourceFile;
							if(currentUrl != fileSource->sourceUrl()) {
								fileSource->setSource(currentUrl, this, true);
								continue;
							}
						}
					}
				}

				// Scan input source for animation frames.
				fileSource->updateFrames();
			}
			catch(const Exception& ex) {
				ex.reportError();
			}
		}
	}
}

/******************************************************************************
* Determines if the option to replace the currently selected object
* with the new file is available.
******************************************************************************/
bool FileSourceImporter::isReplaceExistingPossible(const QUrl& sourceUrl)
{
	// Look for an existing FileSource in the scene whose
	// data source we can replace with the new file.
	for(SceneNode* node : dataset()->selection()->nodes()) {
		if(ObjectNode* objNode = dynamic_object_cast<ObjectNode>(node)) {
			if(dynamic_object_cast<FileSource>(objNode->sourceObject()))
				return true;
		}
	}
	return false;
}

/******************************************************************************
* Imports the given file into the scene.
* Return true if the file has been imported.
* Return false if the import has been aborted by the user.
* Throws an exception when the import has failed.
******************************************************************************/
bool FileSourceImporter::importFile(const QUrl& sourceUrl, ImportMode importMode, bool autodetectFileSequences)
{
	OORef<FileSource> existingFileSource;
	ObjectNode* existingNode = nullptr;

	if(importMode == ReplaceSelected) {
		// Look for an existing FileSource in the scene whose
		// data source we can replace with the newly imported file.
		for(SceneNode* node : dataset()->selection()->nodes()) {
			if(ObjectNode* objNode = dynamic_object_cast<ObjectNode>(node)) {
				existingFileSource = dynamic_object_cast<FileSource>(objNode->sourceObject());
				if(existingFileSource) {
					existingNode = objNode;
					break;
				}
			}
		}
	}
	else if(importMode == ResetScene) {
		dataset()->clearScene();
		if(!dataset()->undoStack().isRecording())
			dataset()->undoStack().clear();
		dataset()->setFilePath(QString());
	}
	else {
		if(dataset()->sceneRoot()->children().empty())
			importMode = ResetScene;
	}

	UndoableTransaction transaction(dataset()->undoStack(), tr("Import '%1'").arg(QFileInfo(sourceUrl.path()).fileName()));

	// Do not create any animation keys during import.
	AnimationSuspender animSuspender(this);

	OORef<FileSource> fileSource;

	// Create the object that will insert the imported data into the scene.
	if(existingFileSource == nullptr) {
		fileSource = new FileSource(dataset());

		// When adding the imported data to an existing scene,
		// do not auto-adjust animation interval.
		if(importMode == AddToScene)
			fileSource->setAdjustAnimationIntervalEnabled(false);
	}
	else {
		fileSource = existingFileSource;
	}

	// Set the input location and importer.
	if(!fileSource->setSource(sourceUrl, this, autodetectFileSequences)) {
		return false;
	}

	// Create a new object node in the scene for the linked data.
	SceneRoot* scene = dataset()->sceneRoot();
	OORef<ObjectNode> node;
	if(existingNode == nullptr) {
		{
			UndoSuspender unsoSuspender(this);	// Do not create undo records for this part.

			// Add object to scene.
			node = new ObjectNode(dataset());
			node->setDataProvider(fileSource);

			// Let the importer subclass customize the node.
			prepareSceneNode(node, fileSource);
		}

		// Insert node into scene.
		scene->addChildNode(node);
	}
	else node = existingNode;

	// Select import node.
	dataset()->selection()->setNode(node);

	if(importMode != ReplaceSelected) {
		// Adjust viewports to completely show the newly imported object.
		// This needs to be done after the data has been completely loaded.
		PromiseWatcher* watcher = new PromiseWatcher(this);
		connect(watcher, &PromiseWatcher::finished, dataset()->viewportConfig(), &ViewportConfiguration::zoomToSelectionExtents);
		connect(watcher, &PromiseWatcher::finished, watcher, &QObject::deleteLater);	// Self-destruct watcher object when it's no longer needed.
		watcher->setFuture(dataset()->makeSceneReady());
	}

	transaction.commit();
	return true;
}

/******************************************************************************
* Returns the list of files that match the given wildcard pattern.
******************************************************************************/
Future<QVector<FileSourceImporter::Frame>> FileSourceImporter::findWildcardMatches(const QUrl& sourceUrl, DataSetContainer* datasetContainer)
{
	QVector<Frame> frames;

	// Determine whether the filename contains wildcard characters.
	QFileInfo fileInfo(sourceUrl.path());
	QString pattern = fileInfo.fileName();
	if(pattern.contains('*') == false && pattern.contains('?') == false) {

		// It's not a wildcard pattern. Register just a single frame.
		frames.push_back(Frame(sourceUrl, 0, 0, fileInfo.lastModified(), fileInfo.fileName()));

	}
	else {

		QDir directory;
		bool isLocalPath = false;

		// Scan the directory for files matching the wildcard pattern.
		QStringList entries;
		if(sourceUrl.isLocalFile()) {

			isLocalPath = true;
			directory = QFileInfo(sourceUrl.toLocalFile()).dir();
			for(const QString& filename : directory.entryList(QDir::Files|QDir::NoDotAndDotDot, QDir::Name)) {
				if(matchesWildcardPattern(pattern, filename))
					entries << filename;
			}

		}
		else {

			directory = fileInfo.dir();
			QUrl directoryUrl = sourceUrl;
			directoryUrl.setPath(fileInfo.path());

			try {
				// Retrieve list of files in remote directory.
				Future<QStringList> fileListFuture = Application::instance()->fileManager()->listDirectoryContents(directoryUrl);
				if(!datasetContainer->taskManager().waitForTask(fileListFuture))
					return Future<QVector<Frame>>::createCanceled();

				// Filter file names.
				for(const QString& filename : fileListFuture.result()) {
					if(matchesWildcardPattern(pattern, filename))
						entries << filename;
				}
			}
			catch(Exception& ex) {
				if(ex.context() == nullptr) ex.setContext(datasetContainer);
				throw;
			}
		}

		// Sorted the files.
		// A file called "abc9.xyz" must come before a file named "abc10.xyz", which is not
		// the default lexicographic ordering.
		QMap<QString, QString> sortedFilenames;
		for(const QString& oldName : entries) {
			// Generate a new name from the original filename that yields the correct ordering.
			QString newName;
			QString number;
			for(int index = 0; index < oldName.length(); index++) {
				QChar c = oldName[index];
				if(!c.isDigit()) {
					if(!number.isEmpty()) {
						newName.append(number.rightJustified(10, '0'));
						number.clear();
					}
					newName.append(c);
				}
				else number.append(c);
			}
			if(!number.isEmpty()) newName.append(number.rightJustified(10, '0'));
			sortedFilenames[newName] = oldName;
		}

		// Generate final list of frames.
		for(const auto& iter : sortedFilenames) {
			QFileInfo fileInfo(directory, iter);
			QUrl url = sourceUrl;
			if(isLocalPath)
				url = QUrl::fromLocalFile(fileInfo.filePath());
			else
				url.setPath(fileInfo.filePath());
			frames.push_back(Frame(
				url, 0, 0,
				isLocalPath ? fileInfo.lastModified() : QDateTime(),
				iter));
		}
	}

	return Future<QVector<Frame>>::createImmediate(frames);
}

/******************************************************************************
* Checks if a filename matches to the given wildcard pattern.
******************************************************************************/
bool FileSourceImporter::matchesWildcardPattern(const QString& pattern, const QString& filename)
{
	QString::const_iterator p = pattern.constBegin();
	QString::const_iterator f = filename.constBegin();
	while(p != pattern.constEnd() && f != filename.constEnd()) {
		if(*p == QChar('*')) {
			if(!f->isDigit())
				return false;
			do { ++f; }
			while(f != filename.constEnd() && f->isDigit());
			++p;
			continue;
		}
		else if(*p != *f)
			return false;
		++p;
		++f;
	}
	return p == pattern.constEnd() && f == filename.constEnd();
}

/******************************************************************************
* Writes an animation frame information record to a binary output stream.
******************************************************************************/
SaveStream& operator<<(SaveStream& stream, const FileSourceImporter::Frame& frame)
{
	stream.beginChunk(0x02);
	stream << frame.sourceFile << frame.byteOffset << frame.lineNumber << frame.lastModificationTime << frame.label;
	stream.endChunk();
	return stream;
}

/******************************************************************************
* Reads an animation frame information record from a binary input stream.
******************************************************************************/
LoadStream& operator>>(LoadStream& stream, FileSourceImporter::Frame& frame)
{
	int version = stream.expectChunkRange(0, 2);
	stream >> frame.sourceFile >> frame.byteOffset >> frame.lineNumber >> frame.lastModificationTime;
	if(version >= 2)	// For backward compatibility with OVITO 2.4.2
		stream >> frame.label;
	stream.closeChunk();
	return stream;
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
