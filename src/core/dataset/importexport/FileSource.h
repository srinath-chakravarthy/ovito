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

#pragma once


#include <core/Core.h>
#include <core/scene/objects/CompoundObject.h>
#include <core/scene/pipeline/AsyncPipelineEvaluationHelper.h>
#include <core/utilities/concurrent/Promise.h>
#include <core/utilities/concurrent/PromiseWatcher.h>
#include "FileSourceImporter.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(DataIO)

/**
 * \brief A place holder object that feeds data read from an external file into the scene.
 *
 * This class is used in conjunction with a FileSourceImporter class.
 */
class OVITO_CORE_EXPORT FileSource : public CompoundObject
{
public:

	/// Constructs an empty file source which is not referring to an external file.
	Q_INVOKABLE FileSource(DataSet* dataset);
	
	/// \brief Sets the source location for importing data.
	/// \param sourceUrl The new source location.
	/// \param importer The importer object that will parse the input file.
	/// \param autodetectFileSequences Enables the automatic detection of file sequences.
	/// \return false if the operation has been canceled by the user.
	bool setSource(QUrl sourceUrl, FileSourceImporter* importer, bool autodetectFileSequences);

	/// \brief This reloads the input data from the external file.
	/// \param frameIndex The animation frame to reload from the external file.
	Q_INVOKABLE void refreshFromSource(int frameIndex = -1);

	/// \brief Returns the status returned by the file parser on its last invocation.
	virtual PipelineStatus status() const override { return _importStatus; }

	/// \brief Scans the input source for animation frames and updates the internal list of frames.
	Q_INVOKABLE void updateFrames();

	/// \brief Returns the number of animation frames that can be loaded from the data source.
	int numberOfFrames() const { return _frames.size(); }

	/// \brief Returns the index of the animation frame loaded last from the input file.
	int loadedFrameIndex() const { return _loadedFrameIndex; }

	/// \brief Returns the list of animation frames in the input file(s).
	const QVector<FileSourceImporter::Frame>& frames() const { return _frames; }

	/// \brief Given an animation time, computes the input frame index to be shown at that time.
	Q_INVOKABLE int animationTimeToInputFrame(TimePoint time) const;

	/// \brief Given an input frame index, returns the animation time at which it is shown.
	Q_INVOKABLE TimePoint inputFrameToAnimationTime(int frame) const;

	/// \brief Adjusts the animation interval of the current data set to the number of frames in the data source.
	void adjustAnimationInterval(int gotoFrameIndex = -1);

	/// \brief Requests a frame of the input file sequence.
	PipelineFlowState requestFrame(int frameIndex);

	/// \brief Asks the object for the results of the data pipeline.
	virtual PipelineFlowState evaluateImmediately(const PipelineEvalRequest& request) override;

	/// Asks the object for the complete results of the data pipeline.
	virtual Future<PipelineFlowState> evaluateAsync(const PipelineEvalRequest& request) override {
		return _evaluationRequestHelper.createRequest(this, request);
	}

	/// Returns the title of this object.
	virtual QString objectTitle() override;

	/// Sends an event to all dependents of this RefTarget.
	virtual void notifyDependents(ReferenceEvent& event) override;

	/// \brief Sends an event to all dependents of this RefTarget.
	/// \param eventType The event type passed to the ReferenceEvent constructor.
	inline void notifyDependents(ReferenceEvent::Type eventType) {
		DataObject::notifyDependents(eventType);
	}

	/// This method is called after the reference counter of this object has reached zero
	/// and before the object is being deleted.
	virtual void aboutToBeDeleted() override {
		cancelLoadOperation();
		CompoundObject::aboutToBeDeleted();
	}

protected Q_SLOTS:

	/// \brief This is called when the background loading operation has finished.
	void loadOperationFinished();

	/// \brief This is called when the background frame discovery task has finished.
	void frameDiscoveryFinished();

protected:

	/// \brief Saves the status returned by the parser object and generates a ReferenceEvent::ObjectStatusChanged event.
	void setStatus(const PipelineStatus& status);

	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// \brief Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream) override;

	/// \brief Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

	/// \brief Cancels the current load operation if there is any in progress.
	void cancelLoadOperation();

private:

	/// The associated importer object that is responsible for parsing the input file.
	DECLARE_REFERENCE_FIELD(FileSourceImporter, importer);

	/// Controls whether the scene's animation interval is adjusted to the number of frames found in the input file.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, adjustAnimationIntervalEnabled, setAdjustAnimationIntervalEnabled);

	/// The source file (may include a wild-card pattern).
	DECLARE_PROPERTY_FIELD(QUrl, sourceUrl);

	/// Controls the mapping of input file frames to animation frames (i.e. the numerator of the playback rate for the file sequence).
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, playbackSpeedNumerator, setPlaybackSpeedNumerator);

	/// Controls the mapping of input file frames to animation frames (i.e. the denominator of the playback rate for the file sequence).
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, playbackSpeedDenominator, setPlaybackSpeedDenominator);

	/// Specifies the starting animation frame to which the first frame of the file sequence is mapped.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, playbackStartTime, setPlaybackStartTime);

	/// Stores the list of frames of the time series.
	QVector<FileSourceImporter::Frame> _frames;

	/// The index of the animation frame loaded last from the input file.
	int _loadedFrameIndex;

	/// The index of the animation frame currently being loaded.
	int _frameBeingLoaded;

	/// Flag indicating that the file being loaded has been newly selected by the user.
	/// If not, then the file being loaded is just another frame from the existing sequence.
	bool _isNewFile;

	/// The file that was originally selected by the user when importing the input file.
	QString _originallySelectedFilename;

	/// The asynchronous file loading task started by requestFrame().
	std::shared_ptr<FileSourceImporter::FrameLoader> _activeFrameLoader;

	/// The watcher object that is used to monitor the background operation.
	PromiseWatcher _frameLoaderWatcher;

	/// The active Future that provides the discovered input frames.
	Future<QVector<FileSourceImporter::Frame>> _frameDiscoveryFuture;

	/// The watcher object that is used to monitor the background operation.
	PromiseWatcher _frameDiscoveryWatcher;

	/// The status returned by the parser during its last call.
	PipelineStatus _importStatus;

	/// Manages pending asynchronous pipeline requests.
	AsyncPipelineEvaluationHelper _evaluationRequestHelper;

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("ClassNameAlias", "LinkedFileObject");	// This for backward compatibility with files written by Ovito 2.4 and older.
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace


