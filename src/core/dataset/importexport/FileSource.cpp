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
#include <core/animation/AnimationSettings.h>
#include <core/utilities/io/ObjectLoadStream.h>
#include <core/utilities/io/ObjectSaveStream.h>
#include <core/utilities/io/FileManager.h>
#include <core/app/Application.h>
#include <core/viewport/Viewport.h>
#include <core/viewport/ViewportConfiguration.h>
#include <core/scene/ObjectNode.h>
#include <core/dataset/importexport/FileImporter.h>
#include <core/dataset/DataSetContainer.h>
#include <core/dataset/UndoStack.h>
#include "FileSource.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(DataIO)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(FileSource, CompoundObject);
DEFINE_FLAGS_REFERENCE_FIELD(FileSource, importer, "Importer", FileSourceImporter, PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_NO_UNDO);
DEFINE_PROPERTY_FIELD(FileSource, adjustAnimationIntervalEnabled, "AdjustAnimationIntervalEnabled");
DEFINE_FLAGS_PROPERTY_FIELD(FileSource, sourceUrl, "SourceUrl", PROPERTY_FIELD_NO_UNDO);
DEFINE_PROPERTY_FIELD(FileSource, playbackSpeedNumerator, "PlaybackSpeedNumerator");
DEFINE_PROPERTY_FIELD(FileSource, playbackSpeedDenominator, "PlaybackSpeedDenominator");
DEFINE_PROPERTY_FIELD(FileSource, playbackStartTime, "PlaybackStartTime");
SET_PROPERTY_FIELD_LABEL(FileSource, importer, "File Importer");
SET_PROPERTY_FIELD_LABEL(FileSource, adjustAnimationIntervalEnabled, "Adjust animation length to time series");
SET_PROPERTY_FIELD_LABEL(FileSource, sourceUrl, "Source location");
SET_PROPERTY_FIELD_LABEL(FileSource, playbackSpeedNumerator, "Playback rate numerator");
SET_PROPERTY_FIELD_LABEL(FileSource, playbackSpeedDenominator, "Playback rate denominator");
SET_PROPERTY_FIELD_LABEL(FileSource, playbackStartTime, "Playback start time");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(FileSource, playbackSpeedNumerator, IntegerParameterUnit, 1);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(FileSource, playbackSpeedDenominator, IntegerParameterUnit, 1);
SET_PROPERTY_FIELD_CHANGE_EVENT(FileSource, sourceUrl, ReferenceEvent::TitleChanged);

/******************************************************************************
* Constructs the object.
******************************************************************************/
FileSource::FileSource(DataSet* dataset) : CompoundObject(dataset),
	_adjustAnimationIntervalEnabled(true), _loadedFrameIndex(-1), _frameBeingLoaded(-1),
	_playbackSpeedNumerator(1), _playbackSpeedDenominator(1), _playbackStartTime(0), _isNewFile(false)
{
	INIT_PROPERTY_FIELD(importer);
	INIT_PROPERTY_FIELD(adjustAnimationIntervalEnabled);
	INIT_PROPERTY_FIELD(sourceUrl);
	INIT_PROPERTY_FIELD(playbackSpeedNumerator);
	INIT_PROPERTY_FIELD(playbackSpeedDenominator);
	INIT_PROPERTY_FIELD(playbackStartTime);

	connect(&_frameLoaderWatcher, &PromiseWatcher::finished, this, &FileSource::loadOperationFinished);
	connect(&_frameDiscoveryWatcher, &PromiseWatcher::finished, this, &FileSource::frameDiscoveryFinished);

	// Do not save a copy of the linked external data in state file by default.
	setSaveWithScene(false);
}

/******************************************************************************
* Sets the source location for importing data.
******************************************************************************/
bool FileSource::setSource(QUrl sourceUrl, FileSourceImporter* importer, bool autodetectFileSequences)
{
	// Make file path absolute.
	if(sourceUrl.isLocalFile()) {
		QFileInfo fileInfo(sourceUrl.toLocalFile());
		if(fileInfo.isRelative())
			sourceUrl = QUrl::fromLocalFile(fileInfo.absoluteFilePath());
	}

	if(this->sourceUrl() == sourceUrl && this->importer() == importer)
		return true;

	QFileInfo fileInfo(sourceUrl.path());
	_originallySelectedFilename = fileInfo.fileName();

	if(importer) {
		// If URL is not already a wildcard pattern, generate a default pattern by
		// replacing last sequence of numbers in the filename with a wildcard character.
		if(autodetectFileSequences && importer->autoGenerateWildcardPattern() && !_originallySelectedFilename.contains('*') && !_originallySelectedFilename.contains('?')) {
			int startIndex, endIndex;
			for(endIndex = _originallySelectedFilename.length() - 1; endIndex >= 0; endIndex--)
				if(_originallySelectedFilename.at(endIndex).isNumber()) break;
			if(endIndex >= 0) {
				for(startIndex = endIndex-1; startIndex >= 0; startIndex--)
					if(!_originallySelectedFilename.at(startIndex).isNumber()) break;
				QString wildcardPattern = _originallySelectedFilename.left(startIndex+1) + '*' + _originallySelectedFilename.mid(endIndex+1);
				fileInfo.setFile(fileInfo.dir(), wildcardPattern);
				sourceUrl.setPath(fileInfo.filePath());
				OVITO_ASSERT(sourceUrl.isValid());
			}
		}

		if(this->sourceUrl() == sourceUrl && this->importer() == importer)
			return true;
	}

	// Make the import process reversible.
	UndoableTransaction transaction(dataset()->undoStack(), tr("Set input file"));

	// Make the call to setSource() undoable.
	class SetSourceOperation : public UndoableOperation {
	public:
		SetSourceOperation(FileSource* obj) : _obj(obj), _oldUrl(obj->sourceUrl()), _oldImporter(obj->importer()) {}
		virtual void undo() override {
			QUrl url = _obj->sourceUrl();
			OORef<FileSourceImporter> importer = _obj->importer();
			_obj->setSource(_oldUrl, _oldImporter, false);
			_oldUrl = url;
			_oldImporter = importer;
		}
	private:
		QUrl _oldUrl;
		OORef<FileSourceImporter> _oldImporter;
		OORef<FileSource> _obj;
	};
	dataset()->undoStack().pushIfRecording<SetSourceOperation>(this);

	_sourceUrl = sourceUrl;
	_importer = importer;

	// Cancel any old load operation in progress.
	cancelLoadOperation();

	// Set flag which indicates that the file being loaded is a newly selected one.
	_isNewFile = true;

	// Trigger a reload of the current frame.
	_loadedFrameIndex = -1;
	_frames.clear();

	// Scan the input source for animation frames.
	updateFrames();

	transaction.commit();

	notifyDependents(ReferenceEvent::TitleChanged);

	return true;
}

/******************************************************************************
* Scans the input source for animation frames and updates the internal list of frames.
******************************************************************************/
void FileSource::updateFrames()
{
	// Stop any running frame discovery task.
	_frameDiscoveryWatcher.cancel();

	if(!importer()) {
		_frames.clear();
		_loadedFrameIndex = -1;
		notifyDependents(ReferenceEvent::TargetChanged);
		return;
	}

	_frameDiscoveryFuture = importer()->discoverFrames(sourceUrl());
	_frameDiscoveryWatcher.setFuture(_frameDiscoveryFuture);
}

/******************************************************************************
* Cancels the current load operation if there is any in progress.
******************************************************************************/
void FileSource::cancelLoadOperation()
{
	if(_frameBeingLoaded != -1) {
		try {
			// This will suppress any pending notification events.
			_frameLoaderWatcher.unsetPromise();
			OVITO_ASSERT(_activeFrameLoader);
			_activeFrameLoader->cancel();
			_activeFrameLoader->waitForFinished();
		} catch(...) {}
		_frameBeingLoaded = -1;
		notifyDependents(ReferenceEvent::PendingStateChanged);
	}
}

/******************************************************************************
* Given an animation time, computes the input frame index to be shown at that time.
******************************************************************************/
int FileSource::animationTimeToInputFrame(TimePoint time) const
{
	int animFrame = dataset()->animationSettings()->timeToFrame(time);
	return (animFrame - playbackStartTime()) *
			std::max(1, playbackSpeedNumerator()) /
			std::max(1, playbackSpeedDenominator());
}

/******************************************************************************
* Given an input frame index, returns the animation time at which it is shown.
******************************************************************************/
TimePoint FileSource::inputFrameToAnimationTime(int frame) const
{
	int animFrame = frame *
			std::max(1, playbackSpeedDenominator()) /
			std::max(1, playbackSpeedNumerator()) +
			playbackStartTime();
	return dataset()->animationSettings()->frameToTime(animFrame);
}

/******************************************************************************
* Asks the object for the result of the geometry pipeline at the given time.
******************************************************************************/
PipelineFlowState FileSource::evaluateImmediately(const PipelineEvalRequest& request)
{
	return requestFrame(animationTimeToInputFrame(request.time()));
}

/******************************************************************************
* Requests a frame of the input file sequence.
******************************************************************************/
PipelineFlowState FileSource::requestFrame(int frame)
{
	// Handle out-of-range cases.
	if(frame < 0) frame = 0;
	else if(frame >= numberOfFrames()) frame = numberOfFrames() - 1;

	// Determine validity interval of the returned state.
	TimeInterval interval = TimeInterval::infinite();
	if(frame > 0)
		interval.setStart(inputFrameToAnimationTime(frame));
	if(frame < numberOfFrames() - 1)
		interval.setEnd(std::max(inputFrameToAnimationTime(frame+1)-1, inputFrameToAnimationTime(frame)));

	// Prepare the attribute map that will be passed to the modification pipeline
	// along with the data objects.
	QVariantMap attrs = attributes();
	attrs.insert(QStringLiteral("SourceFrame"), QVariant::fromValue(frame));

	bool oldLoadingTaskWasCanceled = false;
	if(_frameBeingLoaded != -1) {
		if(_frameBeingLoaded == frame) {
			// The requested frame is already being loaded at the moment.
			// Indicate to the caller that the result is pending.
			return PipelineFlowState(PipelineStatus::Pending, dataObjects(), interval, attrs);
		}
		else {
			// Another frame than the requested one is already being loaded.
			// Cancel pending loading operation first.
			try {
				// This will suppress any pending notification events.
				_frameLoaderWatcher.unsetPromise();
				OVITO_ASSERT(_activeFrameLoader);
				_activeFrameLoader->cancel();
				_activeFrameLoader->waitForFinished();
			} catch(...) {}
			_frameBeingLoaded = -1;
			// Inform previous caller that the existing loading operation has been canceled.
			oldLoadingTaskWasCanceled = true;
		}
	}

	if(frame >= 0 && loadedFrameIndex() == frame) {
		if(oldLoadingTaskWasCanceled) {
			setStatus(PipelineStatus::Success);
			notifyDependents(ReferenceEvent::PendingStateChanged);
		}

		// The requested frame has already been loaded and is available immediately.
		return PipelineFlowState(status(), dataObjects(), interval, attrs);
	}
	else {
		// The requested frame needs to be loaded first. Start background loading task.
		if(frame < 0 || frame >= numberOfFrames() || !importer()) {
			if(oldLoadingTaskWasCanceled)
				notifyDependents(ReferenceEvent::PendingStateChanged);

			// Check if we are still discovering the input frames.
			if(_frameDiscoveryFuture.isValid() && numberOfFrames() == 0) {
				// Indicate to the caller that the result is pending.
				setStatus(PipelineStatus::Pending);
				return PipelineFlowState(PipelineStatus::Pending, dataObjects(), interval, attrs);
			}

			setStatus(PipelineStatus(PipelineStatus::Error, tr("The source location is empty or has not been set (no files found).")));
			_loadedFrameIndex = -1;
			return PipelineFlowState(status(), dataObjects(), interval);
		}
		_frameBeingLoaded = frame;
		_activeFrameLoader = importer()->createFrameLoader(frames()[frame], _isNewFile);
		_isNewFile = false;
		_frameLoaderWatcher.setPromise(_activeFrameLoader);
		dataset()->container()->taskManager().runTaskAsync(_activeFrameLoader);
		setStatus(PipelineStatus::Pending);
		if(oldLoadingTaskWasCanceled)
			notifyDependents(ReferenceEvent::PendingStateChanged);

		// Indicate to the caller that the result is pending.
		return PipelineFlowState(PipelineStatus::Pending, dataObjects(), interval, attrs);
	}
}

/******************************************************************************
* This is called when the background loading operation has finished.
******************************************************************************/
void FileSource::loadOperationFinished()
{
	OVITO_ASSERT(_frameBeingLoaded != -1);
	OVITO_ASSERT(_activeFrameLoader);
	bool wasCanceled = _activeFrameLoader->isCanceled();
	int loadedFrame = _frameBeingLoaded;
	_loadedFrameIndex = _frameBeingLoaded;
	_frameBeingLoaded = -1;
	PipelineStatus newStatus = status();

	_frameLoaderWatcher.unsetPromise();
	std::shared_ptr<FileSourceImporter::FrameLoader> frameLoader = std::move(_activeFrameLoader);
	OVITO_ASSERT(!_activeFrameLoader);

	if(!wasCanceled) {
		try {
			// Check for exceptions thrown by the frame loader.
			frameLoader->waitForFinished();
			// Adopt the data loaded by the frame loader.
			frameLoader->handOver(this);
			newStatus = frameLoader->status();
			if(frames().count() > 1) {
				newStatus.setText(tr("Loaded frame %1 of %2\n").arg(loadedFrame+1).arg(frames().count()) + newStatus.text());
			}
		}
		catch(Exception& ex) {
			// Provide a context for this error.
			ex.setContext(dataset());
			// Transfer exception message to evaluation status.
			newStatus = PipelineStatus(PipelineStatus::Error, ex.messages().join(QChar('\n')));
			ex.reportError();
		}
	}
	else {
		newStatus = PipelineStatus(PipelineStatus::Error, tr("Load operation has been canceled by the user."));
	}

	if(_loadedFrameIndex == loadedFrame) {
		// Set the new object status.
		setStatus(newStatus);

		// Notify dependents that the evaluation request was completed.
		notifyDependents(ReferenceEvent::PendingStateChanged);
		notifyDependents(ReferenceEvent::TitleChanged);
	}
}

/******************************************************************************
* This is called when the background frame discovery task has finished.
******************************************************************************/
void FileSource::frameDiscoveryFinished()
{
	if(_frameDiscoveryFuture.isValid() && !_frameDiscoveryFuture.isCanceled()) {
		try {
			QVector<FileSourceImporter::Frame> newFrames = _frameDiscoveryFuture.result();

			// Reload current frame if file has changed.
			if(_loadedFrameIndex >= 0) {
				if(_loadedFrameIndex >= newFrames.size() || _loadedFrameIndex >= _frames.size() || newFrames[_loadedFrameIndex] != _frames[_loadedFrameIndex])
					_loadedFrameIndex = -1;
			}

			_frames = newFrames;
		}
		catch(Exception& ex) {
			// Provide a context for this error.
			ex.setContext(dataset());
			ex.reportError();
		}

		// Jump to the right frame to show the originally selected file.
		int jumpToFrame = -1;
		if(_isNewFile) {
			for(int frameIndex = 0; frameIndex < _frames.size(); frameIndex++) {
				QFileInfo fileInfo(_frames[frameIndex].sourceFile.path());
				if(fileInfo.fileName() == _originallySelectedFilename) {
					jumpToFrame = frameIndex;
					break;
				}
			}
		}

		// Adjust the animation length number to match the number of frames in the input data source.
		adjustAnimationInterval(jumpToFrame);
	}

	// Reset everything.
	_frameDiscoveryWatcher.unsetPromise();
	_frameDiscoveryFuture.reset();

	// Notify dependents that the evaluation request was completed.
	notifyDependents(ReferenceEvent::TargetChanged);
	notifyDependents(ReferenceEvent::PendingStateChanged);
}

/******************************************************************************
* This will reload an animation frame.
******************************************************************************/
void FileSource::refreshFromSource(int frameIndex)
{
	if(!importer())
		return;

	// Remove external file from local file cache so that it will be fetched from the
	// remote server again.
	if(frameIndex >= 0 && frameIndex < frames().size())
		Application::instance()->fileManager()->removeFromCache(frames()[frameIndex].sourceFile);

	if(frameIndex == loadedFrameIndex() || frameIndex == -1) {
		_loadedFrameIndex = -1;
		notifyDependents(ReferenceEvent::TargetChanged);
	}
}

/******************************************************************************
* Saves the status returned by the parser object and generates a
* ReferenceEvent::ObjectStatusChanged event.
******************************************************************************/
void FileSource::setStatus(const PipelineStatus& status)
{
	if(status == _importStatus) return;
	_importStatus = status;
	notifyDependents(ReferenceEvent::ObjectStatusChanged);
}

/******************************************************************************
* Adjusts the animation interval of the current data set to the number of
* frames reported by the file parser.
******************************************************************************/
void FileSource::adjustAnimationInterval(int gotoFrameIndex)
{
	if(!adjustAnimationIntervalEnabled())
		return;

	AnimationSettings* animSettings = dataset()->animationSettings();

	int numFrames = std::max(1, numberOfFrames());
	TimeInterval interval(inputFrameToAnimationTime(0), inputFrameToAnimationTime(std::max(numberOfFrames()-1,0)));
	animSettings->setAnimationInterval(interval);
	if(gotoFrameIndex >= 0 && gotoFrameIndex < numberOfFrames()) {
		animSettings->setTime(inputFrameToAnimationTime(gotoFrameIndex));
	}
	else if(animSettings->time() > interval.end())
		animSettings->setTime(interval.end());
	else if(animSettings->time() < interval.start())
		animSettings->setTime(interval.start());

	animSettings->clearNamedFrames();
	for(int animFrame = animSettings->timeToFrame(interval.start()); animFrame <= animSettings->timeToFrame(interval.end()); animFrame++) {
		int inputFrame = animationTimeToInputFrame(animSettings->frameToTime(animFrame));
		if(inputFrame >= 0 && inputFrame < _frames.size() && !_frames[inputFrame].label.isEmpty())
			animSettings->assignFrameName(animFrame, _frames[inputFrame].label);
	}
}

/******************************************************************************
* Saves the class' contents to the given stream.
******************************************************************************/
void FileSource::saveToStream(ObjectSaveStream& stream)
{
	CompoundObject::saveToStream(stream);
	stream.beginChunk(0x02);
	stream << _frames;
	if(saveWithScene())
		stream << _loadedFrameIndex;
	else
		stream << -1;

	// Store the relative path to the external file (in addition to the absolute path, which is automatically saved).
	QUrl relativePath = sourceUrl();
	if(relativePath.isLocalFile() && !relativePath.isRelative()) {
		// Extract relative portion of path (only if both the scene file path and the external file path are absolute).
		QFileDevice* fileDevice = qobject_cast<QFileDevice*>(stream.dataStream().device());
		if(fileDevice) {
			QFileInfo sceneFile(fileDevice->fileName());
			if(sceneFile.isAbsolute()) {
				QFileInfo externalFile(relativePath.toLocalFile());
				// Currently this only works for files in the same directory.
				if(externalFile.path() == sceneFile.path()) {
					relativePath = QUrl::fromLocalFile(externalFile.fileName());
				}
			}
		}
	}
	stream << relativePath;

	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void FileSource::loadFromStream(ObjectLoadStream& stream)
{
	CompoundObject::loadFromStream(stream);
	int version = stream.expectChunkRange(0x00, 0x02);
	stream >> _frames;
	stream >> _loadedFrameIndex;
	if(version >= 2) {	// For backward compatibility with OVITO 2.6.2
		QUrl relativePath;
		stream >> relativePath;

		// If the absolute path no longer exists, replace it with the relative one resolved against
		// the scene file's path.
		if(sourceUrl().isLocalFile() && relativePath.isLocalFile()) {
			QFileInfo relativeFileInfo(relativePath.toLocalFile());
			if(relativeFileInfo.isAbsolute() == false) {
				QFileDevice* fileDevice = qobject_cast<QFileDevice*>(stream.dataStream().device());
				if(fileDevice) {
					QFileInfo sceneFile(fileDevice->fileName());
					if(sceneFile.isAbsolute()) {
						_sourceUrl = QUrl::fromLocalFile(QFileInfo(sceneFile.dir(), relativeFileInfo.filePath()).absoluteFilePath());

						// Also update the paths stored in the frame records.
						for(FileSourceImporter::Frame& frame : _frames) {
							if(frame.sourceFile.isLocalFile()) {
								QFileInfo frameFile(frame.sourceFile.toLocalFile());
								frame.sourceFile = QUrl::fromLocalFile(QFileInfo(sceneFile.dir(), frameFile.fileName()).absoluteFilePath());
							}
						}
					}
				}
			}
		}
	}
	stream.closeChunk();
}

/******************************************************************************
* Returns the title of this object.
******************************************************************************/
QString FileSource::objectTitle()
{
	QString filename;
	int frameIndex = loadedFrameIndex();
	if(frameIndex >= 0) {
		filename = QFileInfo(frames()[frameIndex].sourceFile.path()).fileName();
	}
	else if(!sourceUrl().isEmpty()) {
		filename = QFileInfo(sourceUrl().path()).fileName();
	}
	if(importer())
		return QString("%2 [%1]").arg(importer()->objectTitle()).arg(filename);
	return CompoundObject::objectTitle();
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void FileSource::propertyChanged(const PropertyFieldDescriptor& field)
{
	if(field == PROPERTY_FIELD(adjustAnimationIntervalEnabled) ||
			field == PROPERTY_FIELD(playbackSpeedNumerator) ||
			field == PROPERTY_FIELD(playbackSpeedDenominator) ||
			field == PROPERTY_FIELD(playbackStartTime)) {
		adjustAnimationInterval();
	}
	CompoundObject::propertyChanged(field);
}

/******************************************************************************
* Sends an event to all dependents of this RefTarget.
******************************************************************************/
void FileSource::notifyDependents(ReferenceEvent& event)
{
	if(event.type() == ReferenceEvent::PendingStateChanged)
		_evaluationRequestHelper.serveRequests(this);

	DataObject::notifyDependents(event);
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
