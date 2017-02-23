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
#include <core/scene/ObjectNode.h>
#include <core/scene/SelectionSet.h>
#include <core/animation/AnimationSettings.h>
#include <core/utilities/concurrent/TaskManager.h>
#include "AttributeFileExporter.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(DataIO)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(AttributeFileExporter, FileExporter);
DEFINE_PROPERTY_FIELD(AttributeFileExporter, attributesToExport, "AttributesToExport");

/******************************************************************************
* Constructs a new instance of the class.
******************************************************************************/
AttributeFileExporter::AttributeFileExporter(DataSet* dataset) : FileExporter(dataset)
{
	INIT_PROPERTY_FIELD(attributesToExport);
}

/******************************************************************************
* Selects the natural scene nodes to be exported by this exporter under 
* normal circumstances.
******************************************************************************/
void AttributeFileExporter::selectStandardOutputData()
{
	QVector<SceneNode*> nodes = dataset()->selection()->nodes();
	if(nodes.empty())
		throwException(tr("Please select an object to be exported first."));
	setOutputData(nodes);
}

/******************************************************************************
 * This is called once for every output file to be written and before
 * exportData() is called.
 *****************************************************************************/
bool AttributeFileExporter::openOutputFile(const QString& filePath, int numberOfFrames)
{
	OVITO_ASSERT(!_outputFile.isOpen());
	OVITO_ASSERT(!_outputStream);

	_outputFile.setFileName(filePath);
	_outputStream.reset(new CompressedTextWriter(_outputFile, dataset()));

	textStream() << "#";
	for(const QString& attrName : attributesToExport()) {
		textStream() << " \"" << attrName << "\"";
	}
	textStream() << "\n";

	return true;
}

/******************************************************************************
 * This is called once for every output file written after exportData()
 * has been called.
 *****************************************************************************/
void AttributeFileExporter::closeOutputFile(bool exportCompleted)
{
	_outputStream.reset();
	if(_outputFile.isOpen())
		_outputFile.close();

	if(!exportCompleted)
		_outputFile.remove();
}

/******************************************************************************
* Loads the user-defined default values of this object's parameter fields from the
* application's settings store.
 *****************************************************************************/
void AttributeFileExporter::loadUserDefaults()
{
	// This exporter is typically used to export attributes as functions of time.
	setExportAnimation(true);

	FileExporter::loadUserDefaults();

	// Restore last output column mapping.
	QSettings settings;
	settings.beginGroup("exporter/attributes/");
	setAttributesToExport(settings.value("attrlist", QVariant::fromValue(QStringList())).toStringList());
	settings.endGroup();
}

/******************************************************************************
* Evaluates the pipeline of an ObjectNode and returns the computed attributes.
******************************************************************************/
bool AttributeFileExporter::getAttributes(SceneNode* sceneNode, TimePoint time, QVariantMap& attributes, TaskManager& taskManager)
{
	ObjectNode* objectNode = dynamic_object_cast<ObjectNode>(sceneNode);
	if(!objectNode)
		throwException(tr("The scene node to be exported is not an object node."));

	// Evaluate pipeline of object node.
	auto evalFuture = objectNode->evaluatePipelineAsync(PipelineEvalRequest(time, false));
	if(!taskManager.waitForTask(evalFuture))
		return false;

	const PipelineFlowState& state = evalFuture.result();
	if(state.isEmpty())
		throwException(tr("The object to be exported does not contain any data."));

	attributes = state.attributes();
	attributes.insert(QStringLiteral("Frame"), sceneNode->dataset()->animationSettings()->timeToFrame(time));

	return true;
}

/******************************************************************************
 * Exports a single animation frame to the current output file.
 *****************************************************************************/
bool AttributeFileExporter::exportFrame(int frameNumber, TimePoint time, const QString& filePath, TaskManager& taskManager)
{
	if(!FileExporter::exportFrame(frameNumber, time, filePath, taskManager))
		return false;

	// Export the first scene node from the selection set.
	if(outputData().empty())
		throwException(tr("The selection set to be exported is empty."));

	QVariantMap attrMap;
	if(!getAttributes(outputData().front(), time, attrMap, taskManager))
		return false;

	for(const QString& attrName : attributesToExport()) {
		if(!attrMap.contains(attrName))
			throwException(tr("The global attribute '%1' to be exported is not available at animation frame %2.").arg(attrName).arg(frameNumber));

		textStream() << attrMap.value(attrName).toString() << " ";
	}
	textStream() << "\n";

	return true;
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
