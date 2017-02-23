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

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include "PartitionMesh.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(PartitionMesh, DataObject);
DEFINE_PROPERTY_FIELD(PartitionMesh, spaceFillingRegion, "SpaceFillingRegion");

/******************************************************************************
* Constructs an empty surface mesh object.
******************************************************************************/
PartitionMesh::PartitionMesh(DataSet* dataset, PartitionMeshData* mesh) : DataObjectWithSharedStorage(dataset, mesh ? mesh : new PartitionMeshData()),
		_spaceFillingRegion(0)
{
	INIT_PROPERTY_FIELD(spaceFillingRegion);
}

/******************************************************************************
* Creates a copy of this object.
******************************************************************************/
OORef<RefTarget> PartitionMesh::clone(bool deepCopy, CloneHelper& cloneHelper)
{
	// Let the base class create an instance of this class.
	OORef<PartitionMesh> clone = static_object_cast<PartitionMesh>(DataObjectWithSharedStorage<PartitionMeshData>::clone(deepCopy, cloneHelper));

	// Copy internal data.
	clone->_cuttingPlanes = this->_cuttingPlanes;

	return clone;
}

/******************************************************************************
* Fairs a closed triangle mesh.
******************************************************************************/
bool PartitionMesh::smoothMesh(PartitionMeshData& mesh, const SimulationCell& cell, int numIterations, PromiseBase& promise, FloatType k_PB, FloatType lambda)
{
	// This is the implementation of the mesh smoothing algorithm:
	//
	// Gabriel Taubin
	// A Signal Processing Approach To Fair Surface Design
	// In SIGGRAPH 95 Conference Proceedings, pages 351-358 (1995)

	FloatType mu = FloatType(1) / (k_PB - FloatType(1)/lambda);
	promise.setProgressMaximum(numIterations);

	for(int iteration = 0; iteration < numIterations; iteration++) {
		if(!promise.setProgressValue(iteration)) 
			return false;
		smoothMeshIteration(mesh, lambda, cell);
		smoothMeshIteration(mesh, mu, cell);
	}

	return !promise.isCanceled();
}

/******************************************************************************
* Performs one iteration of the smoothing algorithm.
******************************************************************************/
void PartitionMesh::smoothMeshIteration(PartitionMeshData& mesh, FloatType prefactor, const SimulationCell& cell)
{
	const AffineTransformation absoluteToReduced = cell.inverseMatrix();
	const AffineTransformation reducedToAbsolute = cell.matrix();

	// Compute displacement for each vertex.
	std::vector<Vector3> displacements(mesh.vertexCount());
	parallelFor(mesh.vertexCount(), [&mesh, &displacements, prefactor, cell, absoluteToReduced](int index) {
		PartitionMeshData::Vertex* vertex = mesh.vertex(index);
		Vector3 d = Vector3::Zero();

		// Count the number of edges incident on this vertex which are triple lines.
		int numTripleLines = 0;
		PartitionMeshData::Edge* tripleLines[2] = { nullptr, nullptr };
		for(PartitionMeshData::Edge* edge = vertex->edges(); edge != nullptr; edge = edge->nextVertexEdge()) {
			// Check if it is a triple line:
			if(edge->oppositeEdge()->nextManifoldEdge->oppositeEdge()->nextManifoldEdge == edge) {
				continue;	// No, it's not.
			}
			if(numTripleLines == 0) {
				tripleLines[0] = edge;
				numTripleLines++;
			}
			else {
				// Check if we have visited this triple line edge before to avoid double counting.
				PartitionMeshData::Edge* iterEdge = edge;
				do {
					if(iterEdge == tripleLines[0] || iterEdge == tripleLines[1]) {
						break;
					}
					iterEdge = iterEdge->oppositeEdge()->nextManifoldEdge;
				}
				while(iterEdge != edge);
				if(iterEdge == edge) {
					if(numTripleLines < 2) tripleLines[numTripleLines] = edge;
					numTripleLines++;
				}
			}
		}

		if(numTripleLines == 0) {
			PartitionMeshData::Edge* currentEdge = vertex->edges();
			OVITO_ASSERT(currentEdge != nullptr);
			int numManifoldEdges = 0;
			do {
				OVITO_ASSERT(currentEdge != nullptr && currentEdge->face() != nullptr);
				d += cell.wrapVector(currentEdge->vertex2()->pos() - vertex->pos());
				numManifoldEdges++;
				currentEdge = currentEdge->prevFaceEdge()->oppositeEdge();
			}
			while(currentEdge != vertex->edges());
			d *= (prefactor / numManifoldEdges);
		}
		else if(numTripleLines == 2) {
			d += cell.wrapVector(tripleLines[0]->vertex2()->pos() - vertex->pos());
			d += cell.wrapVector(tripleLines[1]->vertex2()->pos() - vertex->pos());
			d *= (prefactor / 2);
		}

		displacements[index] = d;
	});

	// Apply computed displacements.
	auto d = displacements.cbegin();
	for(PartitionMeshData::Vertex* vertex : mesh.vertices())
		vertex->pos() += (*d++);
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
