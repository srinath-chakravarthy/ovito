///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2015) Alexander Stukowski
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

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/particles/objects/SimulationCellObject.h>
#include <plugins/particles/objects/SurfaceMesh.h>
#include <plugins/crystalanalysis/objects/patterns/PatternCatalog.h>
#include <plugins/crystalanalysis/objects/dislocations/DislocationNetworkObject.h>
#include <plugins/crystalanalysis/objects/clusters/ClusterGraphObject.h>
#include <plugins/crystalanalysis/objects/partition_mesh/PartitionMesh.h>
#include <core/utilities/concurrent/TaskManager.h>
#include <core/scene/ObjectNode.h>
#include <core/scene/SelectionSet.h>
#include "CAExporter.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(CAExporter, FileExporter);

/******************************************************************************
* Selects the natural scene nodes to be exported by this exporter under 
* normal circumstances.
******************************************************************************/
void CAExporter::selectStandardOutputData()
{
	QVector<SceneNode*> nodes = dataset()->selection()->nodes();
	if(nodes.empty())
		throwException(tr("Please select an object to be exported first."));
	setOutputData(nodes);
}

/******************************************************************************
 * This is called once for every output file to be written and before
 * exportData() is called.
 ****************************************************************************/
bool CAExporter::openOutputFile(const QString& filePath, int numberOfFrames)
{
	OVITO_ASSERT(!_outputFile.isOpen());
	OVITO_ASSERT(!_outputStream);

	_outputFile.setFileName(filePath);
	_outputStream.reset(new CompressedTextWriter(_outputFile, dataset()));

	return true;
}

/******************************************************************************
 * This is called once for every output file written after exportData() has
 * been called.
 *****************************************************************************/
void CAExporter::closeOutputFile(bool exportCompleted)
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
bool CAExporter::exportFrame(int frameNumber, TimePoint time, const QString& filePath, TaskManager& taskManager)
{
	if(!FileExporter::exportFrame(frameNumber, time, filePath, taskManager))
		return false;

	// Export the first scene node from the selection set.
	if(outputData().empty())
		throwException(tr("The selection set to be exported is empty."));

	ObjectNode* objectNode = dynamic_object_cast<ObjectNode>(outputData().front());
	if(!objectNode)
		throwException(tr("The scene node to be exported is not an object node."));

	// Evaluate pipeline of object node.
	auto evalFuture = objectNode->evaluatePipelineAsync(PipelineEvalRequest(time, false));
	if(!taskManager.waitForTask(evalFuture))
		return false;
	const PipelineFlowState& state = evalFuture.result();
	if(state.isEmpty())
		throwException(tr("The object to be exported does not contain any data."));

	// Get simulation cell info.
	SimulationCellObject* simulationCell = state.findObject<SimulationCellObject>();
	if(!simulationCell)
		throwException(tr("Dataset to be exported contains no simulation cell. Cannot write CA file."));

	// Get dislocation lines.
	DislocationNetworkObject* dislocationObj = state.findObject<DislocationNetworkObject>();

	// Get defect/surface mesh.
	SurfaceMesh* defectMesh = meshExportEnabled() ? state.findObject<SurfaceMesh>() : nullptr;

	// Get partition mesh.
	PartitionMesh* partitionMesh = meshExportEnabled() ? state.findObject<PartitionMesh>() : nullptr;

	if(!dislocationObj && !defectMesh && !partitionMesh)
		throwException(tr("Dataset to be exported contains no dislocation lines nor a surface mesh. Cannot write CA file."));

	// Get cluster graph.
	ClusterGraphObject* clusterGraph = state.findObject<ClusterGraphObject>();
	if(dislocationObj && !clusterGraph)
		throwException(tr("Dataset to be exported contains no cluster graph. Cannot write CA file."));

	// Get pattern catalog.
	PatternCatalog* patternCatalog = state.findObject<PatternCatalog>();
	if(dislocationObj && !patternCatalog)
		throwException(tr("Dataset to be exported contains no structure pattern catalog. Cannot write CA file."));

	// Write file header.
	textStream() << "CA_FILE_VERSION 6\n";
	textStream() << "CA_LIB_VERSION 0.0.0\n";

	if(patternCatalog) {
		// Write list of structure types.
		textStream() << "STRUCTURE_TYPES " << (patternCatalog->patterns().size() - 1) << "\n";
		for(StructurePattern* s : patternCatalog->patterns()) {
			if(s->id() == 0) continue;
			textStream() << "STRUCTURE_TYPE " << s->id() << "\n";
			textStream() << "NAME " << s->shortName() << "\n";
			textStream() << "FULL_NAME " << s->longName() << "\n";
			textStream() << "COLOR " << s->color().r() << " " << s->color().g() << " " << s->color().b() << "\n";
			if(s->structureType() == StructurePattern::Lattice) textStream() << "TYPE LATTICE\n";
			else if(s->structureType() == StructurePattern::Interface) textStream() << "TYPE INTERFACE\n";
			else if(s->structureType() == StructurePattern::PointDefect) textStream() << "TYPE POINTDEFECT\n";
			textStream() << "BURGERS_VECTOR_FAMILIES " << s->burgersVectorFamilies().size() << "\n";
			int bvfId = 0;
			for(BurgersVectorFamily* bvf : s->burgersVectorFamilies()) {
				textStream() << "BURGERS_VECTOR_FAMILY ID " << bvfId << "\n" << bvf->name() << "\n";
				textStream() << bvf->burgersVector().x() << " " << bvf->burgersVector().y() << " " << bvf->burgersVector().z() << "\n";
				textStream() << bvf->color().r() << " " << bvf->color().g() << " " << bvf->color().b() << "\n";
				bvfId++;
			}
			textStream() << "END_STRUCTURE_TYPE\n";
		}
	}

	// Write simulation cell geometry.
	AffineTransformation cell = simulationCell->cellMatrix();
	textStream() << "SIMULATION_CELL_ORIGIN " << cell.column(3).x() << " " << cell.column(3).y() << " " << cell.column(3).z() << "\n";
	textStream() << "SIMULATION_CELL_MATRIX" << "\n"
			<< cell.column(0).x() << " " << cell.column(1).x() << " " << cell.column(2).x() << "\n"
			<< cell.column(0).y() << " " << cell.column(1).y() << " " << cell.column(2).y() << "\n"
			<< cell.column(0).z() << " " << cell.column(1).z() << " " << cell.column(2).z() << "\n";
	textStream() << "PBC_FLAGS "
			<< (int)simulationCell->pbcFlags()[0] << " "
			<< (int)simulationCell->pbcFlags()[1] << " "
			<< (int)simulationCell->pbcFlags()[2] << "\n";

	// Write list of clusters.
	if(clusterGraph) {
		textStream() << "CLUSTERS " << (clusterGraph->storage()->clusters().size() - 1) << "\n";
		for(Cluster* cluster : clusterGraph->storage()->clusters()) {
			if(cluster->id == 0) continue;
			OVITO_ASSERT(clusterGraph->storage()->clusters()[cluster->id] == cluster);
			textStream() << "CLUSTER " << cluster->id << "\n";
			textStream() << "CLUSTER_STRUCTURE " << cluster->structure << "\n";
			textStream() << "CLUSTER_ORIENTATION\n";
			for(size_t row = 0; row < 3; row++)
				textStream() << cluster->orientation(row,0) << " " << cluster->orientation(row,1) << " " << cluster->orientation(row,2) << "\n";
			textStream() << "CLUSTER_COLOR " << cluster->color.r() << " " << cluster->color.g() << " " << cluster->color.b() << "\n";
			textStream() << "CLUSTER_SIZE " << cluster->atomCount << "\n";
			textStream() << "END_CLUSTER\n";
		}

		// Count cluster transitions.
		size_t numClusterTransitions = 0;
		for(ClusterTransition* t : clusterGraph->storage()->clusterTransitions()) {
			if(!t->isSelfTransition())
				numClusterTransitions++;
		}

		// Serialize cluster transitions.
		textStream() << "CLUSTER_TRANSITIONS " << numClusterTransitions << "\n";
		for(ClusterTransition* t : clusterGraph->storage()->clusterTransitions()) {
			if(t->isSelfTransition()) continue;
			textStream() << "TRANSITION " << (t->cluster1->id - 1) << " " << (t->cluster2->id - 1) << "\n";
			textStream() << t->tm.column(0).x() << " " << t->tm.column(1).x() << " " << t->tm.column(2).x() << " "
					<< t->tm.column(0).y() << " " << t->tm.column(1).y() << " " << t->tm.column(2).y() << " "
					<< t->tm.column(0).z() << " " << t->tm.column(1).z() << " " << t->tm.column(2).z() << "\n";
		}
	}

	if(dislocationObj) {
		// Write list of dislocation segments.
		textStream() << "DISLOCATIONS " << dislocationObj->segments().size() << "\n";
		for(DislocationSegment* segment : dislocationObj->segments()) {

			// Make sure consecutive identifiers have been assigned to segments.
			OVITO_ASSERT(segment->id >= 0 && segment->id < dislocationObj->segments().size());
			OVITO_ASSERT(dislocationObj->segments()[segment->id] == segment);

			textStream() << segment->id << "\n";
			textStream() << segment->burgersVector.localVec().x() << " " << segment->burgersVector.localVec().y() << " " << segment->burgersVector.localVec().z() << "\n";
			textStream() << segment->burgersVector.cluster()->id << "\n";

			// Write polyline.
			textStream() << segment->line.size() << "\n";
			if(segment->coreSize.empty()) {
				for(const Point3& p : segment->line) {
					textStream() << p.x() << " " << p.y() << " " << p.z() << "\n";
				}
			}
			else {
				OVITO_ASSERT(segment->coreSize.size() == segment->line.size());
				auto cs = segment->coreSize.cbegin();
				for(const Point3& p : segment->line) {
					textStream() << p.x() << " " << p.y() << " " << p.z() << " " << (*cs++) << "\n";
				}
			}
		}

		// Write dislocation connectivity information.
		textStream() << "DISLOCATION_JUNCTIONS\n";
		int index = 0;
		for(DislocationSegment* segment : dislocationObj->segments()) {
			OVITO_ASSERT(segment->forwardNode().junctionRing->segment->id < dislocationObj->segments().size());
			OVITO_ASSERT(segment->backwardNode().junctionRing->segment->id < dislocationObj->segments().size());

			for(int nodeIndex = 0; nodeIndex < 2; nodeIndex++) {
				DislocationNode* otherNode = segment->nodes[nodeIndex]->junctionRing;
				textStream() << (int)otherNode->isForwardNode() << " " << otherNode->segment->id << "\n";
			}
			index++;
		}
	}

	if(defectMesh) {
		// Serialize list of vertices.
		textStream() << "DEFECT_MESH_VERTICES " << defectMesh->storage()->vertices().size() << "\n";
		for(HalfEdgeMesh<>::Vertex* vertex : defectMesh->storage()->vertices()) {

			// Make sure indices have been assigned to vertices.
			OVITO_ASSERT(vertex->index() >= 0 && vertex->index() < defectMesh->storage()->vertices().size());

			textStream() << vertex->pos().x() << " " << vertex->pos().y() << " " << vertex->pos().z() << "\n";
		}

		// Serialize list of facets.
		textStream() << "DEFECT_MESH_FACETS " << defectMesh->storage()->faces().size() << "\n";
		for(HalfEdgeMesh<>::Face* facet : defectMesh->storage()->faces()) {
			HalfEdgeMesh<>::Edge* e = facet->edges();
			do {
				textStream() << e->vertex1()->index() << " ";
				e = e->nextFaceEdge();
			}
			while(e != facet->edges());
			textStream() << "\n";
		}

		// Serialize facet adjacency information.
		for(HalfEdgeMesh<>::Face* facet : defectMesh->storage()->faces()) {
			HalfEdgeMesh<>::Edge* e = facet->edges();
			do {
				textStream() << e->oppositeEdge()->face()->index() << " ";
				e = e->nextFaceEdge();
			}
			while(e != facet->edges());
			textStream() << "\n";
		}
	}

	if(partitionMesh) {
		// Serialize list of vertices.
		textStream() << "PARTITION_MESH_VERTICES " << partitionMesh->storage()->vertices().size() << "\n";
		for(PartitionMeshData::Vertex* vertex : partitionMesh->storage()->vertices()) {

			// Make sure indices have been assigned to vertices.
			OVITO_ASSERT(vertex->index() >= 0 && vertex->index() < partitionMesh->storage()->vertices().size());

			textStream() << vertex->pos().x() << " " << vertex->pos().y() << " " << vertex->pos().z() << "\n";
		}

		// Serialize list of facets.
		textStream() << "PARTITION_MESH_FACETS " << partitionMesh->storage()->faces().size() << "\n";
		for(PartitionMeshData::Face* facet : partitionMesh->storage()->faces()) {
			textStream() << facet->region << " ";
			PartitionMeshData::Edge* e = facet->edges();
			do {
				textStream() << e->vertex1()->index() << " ";
				e = e->nextFaceEdge();
			}
			while(e != facet->edges());
			textStream() << "\n";
		}

		// Serialize facet adjacency information.
		for(PartitionMeshData::Face* facet : partitionMesh->storage()->faces()) {
			OVITO_ASSERT(facet->oppositeFace != nullptr);
			textStream() << facet->oppositeFace->index() << " ";
			PartitionMeshData::Edge* e = facet->edges();
			do {
				OVITO_ASSERT(facet->oppositeFace->findEdge(e->nextManifoldEdge->vertex1(), e->nextManifoldEdge->vertex2()) != nullptr);
				textStream() << e->oppositeEdge()->face()->index() << " " << e->nextManifoldEdge->vertex1()->index() << " " << e->nextManifoldEdge->vertex2()->index() << " ";
				e = e->nextFaceEdge();
			}
			while(e != facet->edges());
			textStream() << "\n";
		}
	}

	return true;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
