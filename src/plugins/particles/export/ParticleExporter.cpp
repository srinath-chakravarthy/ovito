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
#include <plugins/particles/objects/ParticlePropertyObject.h>
#include <core/utilities/concurrent/TaskManager.h>
#include <core/scene/ObjectNode.h>
#include <core/scene/SelectionSet.h>
#include "ParticleExporter.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Export)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(ParticleExporter, FileExporter);

/******************************************************************************
* Constructs a new instance of the class.
******************************************************************************/
ParticleExporter::ParticleExporter(DataSet* dataset) : FileExporter(dataset)
{
}

/******************************************************************************
* Selects the natural scene nodes to be exported by this exporter under 
* normal circumstances.
******************************************************************************/
void ParticleExporter::selectStandardOutputData()
{
	QVector<SceneNode*> nodes = dataset()->selection()->nodes();
	if(nodes.empty())
		throwException(tr("Please select an object to be exported first."));
	setOutputData(nodes);
}

/******************************************************************************
* Evaluates the pipeline of an ObjectNode and makes sure that the data to be
* exported contains particles and throws an exception if not.
******************************************************************************/
bool ParticleExporter::getParticleData(SceneNode* sceneNode, TimePoint time, PipelineFlowState& state, TaskManager& taskManager)
{
	ObjectNode* objectNode = dynamic_object_cast<ObjectNode>(sceneNode);
	if(!objectNode)
		throwException(tr("The scene node to be exported is not an object node."));

	// Evaluate pipeline of object node.
	auto evalFuture = objectNode->evaluatePipelineAsync(PipelineEvalRequest(time, false));
	if(!taskManager.waitForTask(evalFuture))
		return false;
	state = evalFuture.result();
	if(state.isEmpty())
		throwException(tr("The object to be exported does not contain any data."));

	ParticlePropertyObject* posProperty = ParticlePropertyObject::findInState(state, ParticleProperty::PositionProperty);
	if(!posProperty)
		throwException(tr("The selected scene object does not contain any particles that can be exported."));

	// Verify data, make sure array length is consistent for all particle properties.
	for(DataObject* obj : state.objects()) {
		if(ParticlePropertyObject* p = dynamic_object_cast<ParticlePropertyObject>(obj)) {
			if(p->size() != posProperty->size())
				throwException(tr("Data produced by modification pipeline is invalid. Array size is not the same for all particle properties."));
		}
	}

	return true;
}

/******************************************************************************
 * This is called once for every output file to be written and before
 * exportFrame() is called.
 *****************************************************************************/
bool ParticleExporter::openOutputFile(const QString& filePath, int numberOfFrames)
{
	OVITO_ASSERT(!_outputFile.isOpen());
	OVITO_ASSERT(!_outputStream);

	_outputFile.setFileName(filePath);
	_outputStream.reset(new CompressedTextWriter(_outputFile, dataset()));

	return true;
}

/******************************************************************************
 * This is called once for every output file written after exportFrame()
 * has been called.
 *****************************************************************************/
void ParticleExporter::closeOutputFile(bool exportCompleted)
{
	_outputStream.reset();
	if(_outputFile.isOpen())
		_outputFile.close();

	if(!exportCompleted)
		_outputFile.remove();
}

/******************************************************************************
 * Exports a single animation frame to the current output file.
 *****************************************************************************/
bool ParticleExporter::exportFrame(int frameNumber, TimePoint time, const QString& filePath, TaskManager& taskManager)
{
	if(!FileExporter::exportFrame(frameNumber, time, filePath, taskManager))
		return false;

	// Export the first scene node from the selection set.
	if(!outputData().empty())
		return exportObject(outputData().front(), frameNumber, time, filePath, taskManager);
	else
		throwException(tr("The selection set to be exported is empty."));

	return true;
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
