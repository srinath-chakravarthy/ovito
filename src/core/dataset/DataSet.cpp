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
#include <core/dataset/DataSet.h>
#include <core/dataset/DataSetContainer.h>
#include <core/viewport/Viewport.h>
#include <core/viewport/ViewportConfiguration.h>
#include <core/animation/AnimationSettings.h>
#include <core/scene/SceneRoot.h>
#include <core/scene/SelectionSet.h>
#include <core/rendering/RenderSettings.h>
#include <core/rendering/FrameBuffer.h>
#include <core/rendering/SceneRenderer.h>
#include <core/app/Application.h>
#ifdef OVITO_VIDEO_OUTPUT_SUPPORT
	#include <core/utilities/io/video/VideoEncoder.h>
#endif

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(DataSet, RefTarget);
DEFINE_FLAGS_REFERENCE_FIELD(DataSet, viewportConfig, "ViewportConfiguration", ViewportConfiguration, PROPERTY_FIELD_NO_CHANGE_MESSAGE|PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_REFERENCE_FIELD(DataSet, animationSettings, "AnimationSettings", AnimationSettings, PROPERTY_FIELD_NO_CHANGE_MESSAGE|PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_REFERENCE_FIELD(DataSet, sceneRoot, "SceneRoot", SceneRoot, PROPERTY_FIELD_NO_CHANGE_MESSAGE|PROPERTY_FIELD_ALWAYS_DEEP_COPY);
DEFINE_FLAGS_REFERENCE_FIELD(DataSet, selection, "CurrentSelection", SelectionSet, PROPERTY_FIELD_NO_CHANGE_MESSAGE|PROPERTY_FIELD_ALWAYS_DEEP_COPY);
DEFINE_FLAGS_REFERENCE_FIELD(DataSet, renderSettings, "RenderSettings", RenderSettings, PROPERTY_FIELD_NO_CHANGE_MESSAGE|PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_VECTOR_REFERENCE_FIELD(DataSet, globalObjects, "GlobalObjects", RefTarget, PROPERTY_FIELD_ALWAYS_CLONE|PROPERTY_FIELD_ALWAYS_DEEP_COPY);
SET_PROPERTY_FIELD_LABEL(DataSet, viewportConfig, "Viewport Configuration");
SET_PROPERTY_FIELD_LABEL(DataSet, animationSettings, "Animation Settings");
SET_PROPERTY_FIELD_LABEL(DataSet, sceneRoot, "Scene");
SET_PROPERTY_FIELD_LABEL(DataSet, selection, "Selection");
SET_PROPERTY_FIELD_LABEL(DataSet, renderSettings, "Render Settings");
SET_PROPERTY_FIELD_LABEL(DataSet, globalObjects, "Global objects");

/******************************************************************************
* Constructor.
******************************************************************************/
DataSet::DataSet(DataSet* self) : RefTarget(this), _unitsManager(this)
{
	INIT_PROPERTY_FIELD(viewportConfig);
	INIT_PROPERTY_FIELD(animationSettings);
	INIT_PROPERTY_FIELD(sceneRoot);
	INIT_PROPERTY_FIELD(selection);
	INIT_PROPERTY_FIELD(renderSettings);
	INIT_PROPERTY_FIELD(globalObjects);

	_viewportConfig = createDefaultViewportConfiguration();
	_animationSettings = new AnimationSettings(this);
	_sceneRoot = new SceneRoot(this);
	_selection = new SelectionSet(this);
	_renderSettings = new RenderSettings(this);
}

/******************************************************************************
* Destructor.
******************************************************************************/
DataSet::~DataSet()
{
	if(_sceneReadyRequest) {
		_sceneReadyRequest->cancel();
		_sceneReadyRequest->setFinished();
		_sceneReadyRequest.reset();
	}
}

/******************************************************************************
* Returns a viewport configuration that is used as template for new scenes.
******************************************************************************/
OORef<ViewportConfiguration> DataSet::createDefaultViewportConfiguration()
{
	UndoSuspender noUndo(undoStack());

	OORef<ViewportConfiguration> defaultViewportConfig = new ViewportConfiguration(this);

	OORef<Viewport> topView = new Viewport(this);
	topView->setViewType(Viewport::VIEW_TOP);
	defaultViewportConfig->addViewport(topView);

	OORef<Viewport> frontView = new Viewport(this);
	frontView->setViewType(Viewport::VIEW_FRONT);
	defaultViewportConfig->addViewport(frontView);

	OORef<Viewport> leftView = new Viewport(this);
	leftView->setViewType(Viewport::VIEW_LEFT);
	defaultViewportConfig->addViewport(leftView);

	OORef<Viewport> perspectiveView = new Viewport(this);
	perspectiveView->setViewType(Viewport::VIEW_PERSPECTIVE);
	perspectiveView->setCameraTransformation(ViewportSettings::getSettings().coordinateSystemOrientation() * AffineTransformation::lookAlong({90, -120, 100}, {-90, 120, -100}, {0,0,1}).inverse());
	defaultViewportConfig->addViewport(perspectiveView);

	defaultViewportConfig->setActiveViewport(perspectiveView);
	defaultViewportConfig->setMaximizedViewport(nullptr);

	return defaultViewportConfig;
}

/******************************************************************************
* Is called when a RefTarget referenced by this object has generated an event.
******************************************************************************/
bool DataSet::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	OVITO_ASSERT_MSG(!QCoreApplication::instance() || QThread::currentThread() == QCoreApplication::instance()->thread(), "DataSet::referenceEvent", "Reference events may only be processed in the main thread.");

	if(event->type() == ReferenceEvent::TargetChanged || event->type() == ReferenceEvent::PendingStateChanged) {

		// Update the viewports whenever something has changed in the current data set.
		if(source == sceneRoot() || source == selection() || source == renderSettings()) {
			// Do not automatically update while in the process of jumping to a new animation frame.
			if(!animationSettings()->isTimeChanging())
				viewportConfig()->updateViewports();

			if(source == sceneRoot() && event->type() == ReferenceEvent::PendingStateChanged) {
				// Serve requests waiting for scene to become ready.
				if(_sceneReadyRequest) {
					Application::instance()->runOnceLater(this, [this]() {
						if(_sceneReadyRequest) {
							if(_sceneReadyRequest->isCanceled() || isSceneReady(animationSettings()->time())) {
								_sceneReadyRequest->setFinished();
								_sceneReadyRequest.reset();
							}
						}
					});
				}
			}
		}
	}
	return RefTarget::referenceEvent(source, event);
}

/******************************************************************************
* Is called when the value of a reference field of this RefMaker changes.
******************************************************************************/
void DataSet::referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget)
{
	if(field == PROPERTY_FIELD(viewportConfig)) {
		Q_EMIT viewportConfigReplaced(viewportConfig());
	}
	else if(field == PROPERTY_FIELD(animationSettings)) {
		// Stop animation playback when animation settings are being replaced.
		if(AnimationSettings* oldAnimSettings = static_object_cast<AnimationSettings>(oldTarget))
			oldAnimSettings->stopAnimationPlayback();

		Q_EMIT animationSettingsReplaced(animationSettings());
	}
	else if(field == PROPERTY_FIELD(renderSettings)) {
		Q_EMIT renderSettingsReplaced(renderSettings());
	}
	else if(field == PROPERTY_FIELD(selection)) {
		Q_EMIT selectionSetReplaced(selection());
	}

	// Install a signal/slot connection that updates the viewports every time the animation time has changed.
	if(field == PROPERTY_FIELD(viewportConfig) || field == PROPERTY_FIELD(animationSettings)) {
		disconnect(_updateViewportOnTimeChangeConnection);
		if(animationSettings() && viewportConfig()) {
			_updateViewportOnTimeChangeConnection = connect(animationSettings(), &AnimationSettings::timeChangeComplete, viewportConfig(), &ViewportConfiguration::updateViewports);
			viewportConfig()->updateViewports();
		}
	}

	RefTarget::referenceReplaced(field, oldTarget, newTarget);
}

/******************************************************************************
* Returns the container to which this dataset belongs.
******************************************************************************/
DataSetContainer* DataSet::container() const
{
	for(RefMaker* refmaker : dependents()) {
		if(DataSetContainer* c = dynamic_object_cast<DataSetContainer>(refmaker)) {
			return c;
		}
	}
	OVITO_ASSERT_MSG(false, "DataSet::container()", "DataSet is not in a DataSetContainer.");
	return nullptr;
}

/******************************************************************************
* Deletes all nodes from the scene.
******************************************************************************/
void DataSet::clearScene()
{
	while(!sceneRoot()->children().empty())
		sceneRoot()->children().back()->deleteNode();
}

/******************************************************************************
* Rescales the animation keys of all controllers in the scene.
******************************************************************************/
void DataSet::rescaleTime(const TimeInterval& oldAnimationInterval, const TimeInterval& newAnimationInterval)
{
	// Iterate over all controllers in the scene.
	for(RefTarget* reftarget : getAllDependencies()) {
		if(Controller* ctrl = dynamic_object_cast<Controller>(reftarget)) {
			ctrl->rescaleTime(oldAnimationInterval, newAnimationInterval);
		}
	}
}

/******************************************************************************
* Checks all scene nodes if their data pipeline is fully evaluated at the
* given animation time.
******************************************************************************/
bool DataSet::isSceneReady(TimePoint time) const
{
	OVITO_ASSERT_MSG(QThread::currentThread() == QCoreApplication::instance()->thread(), "DataSet::isSceneReady", "This function may only be called from the main thread.");
	OVITO_CHECK_OBJECT_POINTER(sceneRoot());

	PipelineEvalRequest pipelineRequest(time, true); // Request display objects to be ready as well.

	// Iterate over all object nodes and make an attempt to request results from their data pipelines.
	// The scene is ready if none of the results has status 'pending'.
	bool isReady = sceneRoot()->visitObjectNodes([&pipelineRequest](ObjectNode* node) {
		return (node->evaluatePipelineImmediately(pipelineRequest).status().type() != PipelineStatus::Pending);
	});

	return isReady;
}

/******************************************************************************
* This function blocks until the scene has become ready.
******************************************************************************/
Future<void> DataSet::makeSceneReady(const QString& message)
{
	// Perform a first quick check if scene is already ready.
	if(isSceneReady(animationSettings()->time())) {
		if(!_sceneReadyRequest)
			return Future<void>::createImmediate(message);
		else {
			_sceneReadyRequest->setFinished();
			Future<void> future(_sceneReadyRequest);
			_sceneReadyRequest.reset();
			return future;
		}
	}

	// Re-use existing request.
	if(_sceneReadyRequest) {
		if(!_sceneReadyRequest->isCanceled())
			return Future<void>(_sceneReadyRequest);
		else {
			_sceneReadyRequest->setFinished();
			_sceneReadyRequest.reset();
		}
	}

	// If not ready yet, create a future.
	Future<void> future = Future<void>::createWithPromise();
	_sceneReadyRequest = future.promise();
	_sceneReadyRequest->setStarted();
	_sceneReadyRequest->setProgressText(message);
	return future;
}

/******************************************************************************
* This is the high-level rendering function, which invokes the renderer to generate one or more
* output images of the scene. All rendering parameters are specified in the RenderSettings object.
******************************************************************************/
bool DataSet::renderScene(RenderSettings* settings, Viewport* viewport, FrameBuffer* frameBuffer, TaskManager& taskManager)
{
	OVITO_CHECK_OBJECT_POINTER(settings);
	OVITO_CHECK_OBJECT_POINTER(viewport);
	OVITO_ASSERT(frameBuffer);

	// Get the selected scene renderer.
	SceneRenderer* renderer = settings->renderer();
	if(!renderer) throwException(tr("No rendering engine has been selected."));

	SynchronousTask renderTask(taskManager);
	renderTask.setProgressText(tr("Initializing renderer"));
	try {

		// Resize output frame buffer.
		if(frameBuffer->size() != QSize(settings->outputImageWidth(), settings->outputImageHeight())) {
			frameBuffer->setSize(QSize(settings->outputImageWidth(), settings->outputImageHeight()));
			frameBuffer->clear();
		}

		// Don't update viewports while rendering.
		ViewportSuspender noVPUpdates(this);

		// Initialize the renderer.
		if(renderer->startRender(this, settings)) {

			VideoEncoder* videoEncoder = nullptr;
#ifdef OVITO_VIDEO_OUTPUT_SUPPORT
			QScopedPointer<VideoEncoder> videoEncoderPtr;
			// Initialize video encoder.
			if(settings->saveToFile() && settings->imageInfo().isMovie()) {

				if(settings->imageFilename().isEmpty())
					throwException(tr("Cannot save rendered images to movie file. Output filename has not been specified."));

				videoEncoderPtr.reset(new VideoEncoder());
				videoEncoder = videoEncoderPtr.data();
				videoEncoder->openFile(settings->imageFilename(), settings->outputImageWidth(), settings->outputImageHeight(), animationSettings()->framesPerSecond());
			}
#endif

			if(settings->renderingRangeType() == RenderSettings::CURRENT_FRAME) {
				// Render a single frame.
				TimePoint renderTime = animationSettings()->time();
				int frameNumber = animationSettings()->timeToFrame(renderTime);
				renderTask.setProgressText(QString());
				if(!renderFrame(renderTime, frameNumber, settings, renderer, viewport, frameBuffer, videoEncoder, taskManager))
					renderTask.cancel();
			}
			else if(settings->renderingRangeType() == RenderSettings::ANIMATION_INTERVAL || settings->renderingRangeType() == RenderSettings::CUSTOM_INTERVAL) {
				// Render an animation interval.
				TimePoint renderTime;
				int firstFrameNumber, numberOfFrames;
				if(settings->renderingRangeType() == RenderSettings::ANIMATION_INTERVAL) {
					renderTime = animationSettings()->animationInterval().start();
					firstFrameNumber = animationSettings()->timeToFrame(animationSettings()->animationInterval().start());
					numberOfFrames = (animationSettings()->timeToFrame(animationSettings()->animationInterval().end()) - firstFrameNumber + 1);
				}
				else {
					firstFrameNumber = settings->customRangeStart();
					renderTime = animationSettings()->frameToTime(firstFrameNumber);
					numberOfFrames = (settings->customRangeEnd() - firstFrameNumber + 1);
				}
				numberOfFrames = (numberOfFrames + settings->everyNthFrame() - 1) / settings->everyNthFrame();
				if(numberOfFrames < 1)
					throwException(tr("Invalid rendering range: Frame %1 to %2").arg(settings->customRangeStart()).arg(settings->customRangeEnd()));
				renderTask.setProgressMaximum(numberOfFrames);

				// Render frames, one by one.
				for(int frameIndex = 0; frameIndex < numberOfFrames; frameIndex++) {
					int frameNumber = firstFrameNumber + frameIndex * settings->everyNthFrame() + settings->fileNumberBase();

					renderTask.setProgressValue(frameIndex);
					renderTask.setProgressText(tr("Rendering animation (frame %1 of %2)").arg(frameIndex+1).arg(numberOfFrames));

					if(!renderFrame(renderTime, frameNumber, settings, renderer, viewport, frameBuffer, videoEncoder, taskManager))
						renderTask.cancel();
					if(renderTask.isCanceled())
						break;

					// Go to next animation frame.
					renderTime += animationSettings()->ticksPerFrame() * settings->everyNthFrame();
				}
			}

#ifdef OVITO_VIDEO_OUTPUT_SUPPORT
			// Finalize movie file.
			if(videoEncoder)
				videoEncoder->closeFile();
#endif
		}

		// Shutdown renderer.
		renderer->endRender();
	}
	catch(Exception& ex) {
		// Shutdown renderer.
		renderer->endRender();
		// Provide a context for this error.
		if(ex.context() == nullptr) ex.setContext(this);
		throw;
	}

	return !renderTask.isCanceled();
}

/******************************************************************************
* Renders a single frame and saves the output file.
******************************************************************************/
bool DataSet::renderFrame(TimePoint renderTime, int frameNumber, RenderSettings* settings, SceneRenderer* renderer, Viewport* viewport,
		FrameBuffer* frameBuffer, VideoEncoder* videoEncoder, TaskManager& taskManager)
{
	// Determine output filename for this frame.
	QString imageFilename;
	if(settings->saveToFile() && !videoEncoder) {
		imageFilename = settings->imageFilename();
		if(imageFilename.isEmpty())
			throwException(tr("Cannot save rendered image to file, because no output filename has been specified."));

		if(settings->renderingRangeType() != RenderSettings::CURRENT_FRAME) {
			// Append frame number to file name if rendering an animation.
			QFileInfo fileInfo(imageFilename);
			imageFilename = fileInfo.path() + QChar('/') + fileInfo.baseName() + QString("%1.").arg(frameNumber, 4, 10, QChar('0')) + fileInfo.completeSuffix();

			// Check for existing image file and skip.
			if(settings->skipExistingImages() && QFileInfo(imageFilename).isFile())
				return true;
		}
	}

	// Jump to animation frame.
	animationSettings()->setTime(renderTime);

	// Wait until the scene is ready.
	Future<void> sceneReadyFuture = makeSceneReady(tr("Preparing frame %1").arg(frameNumber));
	if(!taskManager.waitForTask(sceneReadyFuture))
		return false;

	// Request scene bounding box.
	Box3 boundingBox = renderer->sceneBoundingBox(renderTime);

	// Setup projection.
	ViewProjectionParameters projParams = viewport->projectionParameters(renderTime, settings->outputImageAspectRatio(), boundingBox);

	// Render one frame.
	frameBuffer->clear();
	try {
		renderer->beginFrame(renderTime, projParams, viewport);
		if(!renderer->renderFrame(frameBuffer, SceneRenderer::NonStereoscopic, taskManager)) {
			renderer->endFrame(false);
			return false;
		}
		renderer->endFrame(true);
	}
	catch(...) {
		renderer->endFrame(false);
		throw;
	}

	// Apply viewport overlays.
	for(ViewportOverlay* overlay : viewport->overlays()) {
		{
			QPainter painter(&frameBuffer->image());
			overlay->render(viewport, painter, projParams, settings);
		}
		frameBuffer->update();
	}

	// Save rendered image to disk.
	if(settings->saveToFile()) {
		if(!videoEncoder) {
			OVITO_ASSERT(!imageFilename.isEmpty());
			if(!frameBuffer->image().save(imageFilename, settings->imageInfo().format()))
				throwException(tr("Failed to save rendered image to output file '%1'.").arg(imageFilename));
		}
		else {
#ifdef OVITO_VIDEO_OUTPUT_SUPPORT
			videoEncoder->writeFrame(frameBuffer->image());
#endif
		}
	}

	return true;
}

/******************************************************************************
* Saves the dataset to the given file.
******************************************************************************/
void DataSet::saveToFile(const QString& filePath)
{
	QFile fileStream(filePath);
    if(!fileStream.open(QIODevice::WriteOnly))
    	throwException(tr("Failed to open output file '%1' for writing.").arg(filePath));

	QDataStream dataStream(&fileStream);
	ObjectSaveStream stream(dataStream);
	stream.saveObject(this);
	stream.close();

	if(fileStream.error() != QFile::NoError)
		throwException(tr("Failed to write output file '%1'.").arg(filePath));
	fileStream.close();
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
