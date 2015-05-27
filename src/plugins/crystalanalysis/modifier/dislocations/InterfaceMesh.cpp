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
#include "InterfaceMesh.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/******************************************************************************
* Classifies each tetrahedron of the tessellation as being either good or bad.
******************************************************************************/
bool InterfaceMesh::classifyTetrahedra(FutureInterfaceBase& progress)
{
	_numGoodTetrahedra = 0;
	_isCompletelyGood = true;

	for(DelaunayTessellation::CellIterator cell = tessellation().begin_cells(); cell != tessellation().end_cells(); ++cell) {
		// Determine whether the tetrahedron belongs to the good or the bad crystal region.
		bool isGood = elasticMapping().isElasticMappingCompatible(cell);

		cell->info().flag = isGood;
		if(isGood && !cell->info().isGhost) {
			cell->info().index = _numGoodTetrahedra++;
		}
		else {
			if(!cell->info().isGhost) _isCompletelyGood = false;
			cell->info().index = -1;
		}

		if(progress.isCanceled())
			return false;
	}
	return true;
}

/******************************************************************************
* Creates the mesh facets separating good and bad tetrahedra.
******************************************************************************/
bool InterfaceMesh::createSeparatingFacets(FutureInterfaceBase& progress)
{
	progress.setProgressValue(0);
	progress.setProgressRange(_numGoodTetrahedra);

	// Stores pointers to the mesh facets generated for a good
	// tetrahedron of the Delaunay tessellation.
	struct Tetrahedron {
		/// Pointers to the mesh facets associated with the four faces of the tetrahedron.
		std::array<Face*, 4> meshFacets;
		DelaunayTessellation::CellHandle cell;
	};

	std::map<std::array<int,4>, Tetrahedron> tetrahedra;
	std::vector<std::map<std::array<int,4>, Tetrahedron>::const_iterator> tetrahedraList;
	tetrahedraList.reserve(_numGoodTetrahedra);

	// Create the triangular mesh facets separating solid and open tetrahedra.
	std::vector<HalfEdgeMesh::Vertex*> vertexMap(structureAnalysis().atomCount(), nullptr);
	for(DelaunayTessellation::CellIterator cell = tessellation().begin_cells(); cell != tessellation().end_cells(); ++cell) {

		// Start with the primary images of the good tetrahedra.
		if(cell->info().index == -1)
			continue;
		OVITO_ASSERT(cell->info().flag);

		if((cell->info().index % 1024) == 0)
			progress.setProgressValue(cell->info().index);
		if(progress.isCanceled()) return false;

		Tetrahedron tet;
		tet.cell = cell;
		std::array<int,4> vertexIndices;
		for(size_t i = 0; i < 4; i++) {
			vertexIndices[i] = cell->vertex(i)->point().index();
		}

		// Iterate over the four faces of the tetrahedron cell.
		for(int f = 0; f < 4; f++) {
			tet.meshFacets[f] = nullptr;

			// Test if the adjacent tetrahedron belongs to the bad region.
			DelaunayTessellation::CellHandle adjacentCell = tessellation().mirrorCell(cell, f);
			if(adjacentCell->info().flag)
				continue;	// It's also a good tet.

			// Create the three vertices of the face or use existing output vertices.
			std::array<Vertex*, 3> facetVertices;
			for(int v = 0; v < 3; v++) {
				DelaunayTessellation::VertexHandle vertex = cell->vertex(DelaunayTessellation::cellFacetVertexIndex(f, v));
				int vertexIndex = vertex->point().index();
				OVITO_ASSERT(vertexIndex >= 0 && vertexIndex < vertexMap.size());
				if(vertexMap[vertexIndex] == nullptr)
					vertexMap[vertexIndex] = facetVertices[2 - v] = createVertex(structureAnalysis().positions()->getPoint3(vertexIndex));
				else
					facetVertices[2-v] = vertexMap[vertexIndex];
			}

			// Create a new triangle facet.
			tet.meshFacets[f] = createFace(facetVertices.begin(), facetVertices.end());
		}

		std::sort(vertexIndices.begin(), vertexIndices.end());
		tetrahedraList.push_back(tetrahedra.insert(std::make_pair(vertexIndices, tet)).first);
	}

	return true;
}

/******************************************************************************
* Links half-edges to opposite half-edges together.
******************************************************************************/
bool InterfaceMesh::linkHalfEdges(FutureInterfaceBase& progress)
{
	return true;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
