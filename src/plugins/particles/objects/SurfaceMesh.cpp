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
#include <core/utilities/concurrent/ParallelFor.h>
#include "SurfaceMesh.h"

namespace Ovito { namespace Particles {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(SurfaceMesh, DataObject);
DEFINE_PROPERTY_FIELD(SurfaceMesh, isCompletelySolid, "IsCompletelySolid");

/******************************************************************************
* Constructs an empty surface mesh object.
******************************************************************************/
SurfaceMesh::SurfaceMesh(DataSet* dataset, HalfEdgeMesh<>* mesh) : DataObjectWithSharedStorage(dataset, mesh ? mesh : new HalfEdgeMesh<>()),
		_isCompletelySolid(false)
{
	INIT_PROPERTY_FIELD(isCompletelySolid);
}

/******************************************************************************
* Creates a copy of this object.
******************************************************************************/
OORef<RefTarget> SurfaceMesh::clone(bool deepCopy, CloneHelper& cloneHelper)
{
	// Let the base class create an instance of this class.
	OORef<SurfaceMesh> clone = static_object_cast<SurfaceMesh>(DataObjectWithSharedStorage<HalfEdgeMesh<>>::clone(deepCopy, cloneHelper));

	// Copy internal data.
	clone->_cuttingPlanes = this->_cuttingPlanes;

	return clone;
}

/******************************************************************************
* Fairs a closed triangle mesh.
******************************************************************************/
bool SurfaceMesh::smoothMesh(HalfEdgeMesh<>& mesh, const SimulationCell& cell, int numIterations, PromiseBase& promise, FloatType k_PB, FloatType lambda)
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
void SurfaceMesh::smoothMeshIteration(HalfEdgeMesh<>& mesh, FloatType prefactor, const SimulationCell& cell)
{
	const AffineTransformation absoluteToReduced = cell.inverseMatrix();
	const AffineTransformation reducedToAbsolute = cell.matrix();

	// Compute displacement for each vertex.
	std::vector<Vector3> displacements(mesh.vertexCount());
	parallelFor(mesh.vertexCount(), [&mesh, &displacements, prefactor, cell, absoluteToReduced](int index) {
		HalfEdgeMesh<>::Vertex* vertex = mesh.vertex(index);
		Vector3 d = Vector3::Zero();

		// Go in positive direction around vertex, facet by facet.
		HalfEdgeMesh<>::Edge* currentEdge = vertex->edges();
		if(currentEdge != nullptr) {
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

		displacements[index] = d;
	});

	// Apply computed displacements.
	auto d = displacements.cbegin();
	for(HalfEdgeMesh<>::Vertex* vertex : mesh.vertices())
		vertex->pos() += *d++;
}

}	// End of namespace
}	// End of namespace
