///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
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
#include <core/scene/SceneRoot.h>
#include <core/animation/AnimationSettings.h>
#include <core/utilities/concurrent/Task.h>
#include <core/utilities/concurrent/TaskManager.h>
#include <core/rendering/RenderSettings.h>
#include <core/viewport/Viewport.h>
#include <core/viewport/ViewportConfiguration.h>
#include "POVRayExporter.h"

namespace Ovito { namespace POVRay {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(POVRayExporter, FileExporter);

/******************************************************************************
* Constructs a new instance of the class.
******************************************************************************/
POVRayExporter::POVRayExporter(DataSet* dataset) : FileExporter(dataset)
{
}

/******************************************************************************
* Selects the natural scene nodes to be exported by this exporter under 
* normal circumstances.
******************************************************************************/
void POVRayExporter::selectStandardOutputData()
{
	// Export the whole scene (i.e. its root node) by default.
	QVector<SceneNode*> nodes;
	nodes.push_back(dataset()->sceneRoot());
	setOutputData(nodes);
}

/******************************************************************************
 * This is called once for every output file to be written and before
 * exportData() is called.
 *****************************************************************************/
bool POVRayExporter::openOutputFile(const QString& filePath, int numberOfFrames)
{
	OVITO_ASSERT(!_outputFile.isOpen());
	OVITO_ASSERT(!_renderer);

	_outputFile.setFileName(filePath);
	if(!_outputFile.open(QIODevice::WriteOnly | QIODevice::Text))
		throwException(tr("Failed to open output file '%1' for writing: %2").arg(filePath).arg(_outputFile.errorString()));

	// Take the already-existing POV-Ray renderer from the dataset if there is one.
	// Otherwise, create a temporary POV-Ray renderer for streaming the scene objects to the output file.
	POVRayRenderer* sceneRenderer = dynamic_object_cast<POVRayRenderer>(dataset()->renderSettings()->renderer());
	if(sceneRenderer)
		_renderer.reset(sceneRenderer);
	else
		_renderer.reset(new POVRayRenderer(dataset()));

	_renderer->setScriptOutputDevice(&_outputFile);
	if(!_renderer->startRender(dataset(), dataset()->renderSettings()))
		return false;

	return true;
}

/******************************************************************************
 * This is called once for every output file written after exportData()
 * has been called.
 *****************************************************************************/
void POVRayExporter::closeOutputFile(bool exportCompleted)
{
	if(_renderer) {
		_renderer->endRender();
		_renderer.reset();
	}
	if(_outputFile.isOpen())
		_outputFile.close();

	if(!exportCompleted)
		_outputFile.remove();
}

/******************************************************************************
 * Exports a single animation frame to the current output file.
 *****************************************************************************/
bool POVRayExporter::exportFrame(int frameNumber, TimePoint time, const QString& filePath, TaskManager& taskManager)
{
	if(!FileExporter::exportFrame(frameNumber, time, filePath, taskManager))
		return false;

	// Wait until the scene is ready.
	Future<void> sceneReadyFuture = dataset()->makeSceneReady();
	if(!taskManager.waitForTask(sceneReadyFuture))
		return false;

	Viewport* vp = dataset()->viewportConfig()->activeViewport();
	if(!vp) throwException(tr("POV-Ray exporter requires an active viewport."));

	SynchronousTask exportTask(taskManager);
	exportTask.setProgressText(tr("Writing data to POV-Ray file"));

	OVITO_ASSERT(_renderer);
	Box3 boundingBox = _renderer->sceneBoundingBox(time);
	ViewProjectionParameters projParams = vp->projectionParameters(time, _renderer->renderSettings()->outputImageAspectRatio(), boundingBox);
	try {
		_renderer->_exportTask = &exportTask;
		_renderer->beginFrame(time, projParams, vp);
		for(SceneNode* node : outputData()) {
			_renderer->renderNode(node);
			if(exportTask.isCanceled()) break;
		}
		_renderer->endFrame(!exportTask.isCanceled());
	}
	catch(...) {
		_renderer->endFrame(false);
		throw;
	}

	return !exportTask.isCanceled();
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
