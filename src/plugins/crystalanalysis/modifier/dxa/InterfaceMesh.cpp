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
#include "DislocationAnalysisModifier.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/******************************************************************************
* Classifies each tetrahedron of the tessellation as being either good or bad.
******************************************************************************/
bool InterfaceMesh::classifyTetrahedra(FutureInterfaceBase& progress)
{
	_numGoodTetrahedra = 0;
	_isCompletelyGood = true;

	progress.setProgressRange(tessellation().number_of_tetrahedra());
	int progressCounter = 0;
	for(DelaunayTessellation::CellIterator cell = tessellation().begin_cells(); cell != tessellation().end_cells(); ++cell) {

		// Update progress indicator.
		if(!progress.setProgressValueIntermittent(progressCounter++))
			return false;

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
	}
	return true;
}

/******************************************************************************
* Creates the mesh facets separating good and bad tetrahedra.
******************************************************************************/
bool InterfaceMesh::createMesh(FutureInterfaceBase& progress)
{
	progress.beginProgressSubSteps(2);
	progress.setProgressRange(_numGoodTetrahedra);

	// Stores pointers to the mesh facets generated for a good
	// tetrahedron of the Delaunay tessellation.
	struct Tetrahedron {
		// Pointers to the mesh facets associated with the four faces of the tetrahedron.
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

		// Update progress indicator.
		if(!progress.setProgressValueIntermittent(cell->info().index))
			return false;

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
				continue;	// It's also a good tet. That means we don't create an interface facet here.

			// Create the three vertices of the face or use existing output vertices.
			std::array<Vertex*, 3> facetVertices;
			Point3 vertexPositions[3];
			int vertexIndices[3];
			for(int v = 0; v < 3; v++) {
				DelaunayTessellation::VertexHandle vertex = cell->vertex(DelaunayTessellation::cellFacetVertexIndex(f, v));
				int vertexIndex = vertex->point().index();
				OVITO_ASSERT(vertexIndex >= 0 && vertexIndex < vertexMap.size());
				if(vertexMap[vertexIndex] == nullptr)
					vertexMap[vertexIndex] = facetVertices[2 - v] = createVertex(structureAnalysis().positions()->getPoint3(vertexIndex));
				else
					facetVertices[2-v] = vertexMap[vertexIndex];
				vertexPositions[2-v] = (Point3)vertex->point();
				vertexIndices[2-v] = vertexIndex;
			}

			// Create a new triangle facet.
			Face* face = tet.meshFacets[f] = createFace(facetVertices.begin(), facetVertices.end());

			// Transfer cluster vectors from tessellation edges to mesh edges.
			Edge* edge = face->edges();
			for(int i = 0; i < 3; i++, edge = edge->nextFaceEdge()) {
				OVITO_ASSERT(edge->vertex1() == facetVertices[i]);
				OVITO_ASSERT(edge->vertex2() == facetVertices[(i+1)%3]);
				edge->physicalVector = vertexPositions[(i+1)%3] - vertexPositions[i];
				OVITO_ASSERT(!structureAnalysis().cell().isWrappedVector(edge->physicalVector));

				ElasticMapping::TessellationEdge* tessEdge = elasticMapping().findEdge(vertexIndices[i], vertexIndices[(i+1)%3]);
				OVITO_ASSERT(tessEdge != nullptr);

				edge->clusterVector = tessEdge->clusterVector;
				edge->clusterTransition = tessEdge->clusterTransition;
			}
		}

		std::sort(vertexIndices.begin(), vertexIndices.end());
		tetrahedraList.push_back(tetrahedra.insert(std::make_pair(vertexIndices, tet)).first);
	}

	// Link half-edges with opposite half-edges.
	progress.nextProgressSubStep();
	progress.setProgressRange(tetrahedra.size());
	int progressCounter = 0;

	for(auto tetIter = tetrahedra.cbegin(); tetIter != tetrahedra.cend(); ++tetIter) {

		// Update progress indicator.
		if(!progress.setProgressValueIntermittent(progressCounter))
			return false;

		const Tetrahedron& tet = tetIter->second;

		for(int f = 0; f < 4; f++) {
			Face* facet = tet.meshFacets[f];
			if(facet == nullptr) continue;

			Edge* edge = facet->edges();
			for(int e = 0; e < 3; e++, edge = edge->nextFaceEdge()) {
				OVITO_CHECK_POINTER(edge);
				if(edge->oppositeEdge() != nullptr) continue;
				int vertexIndex1 = DelaunayTessellation::cellFacetVertexIndex(f, 2-e);
				int vertexIndex2 = DelaunayTessellation::cellFacetVertexIndex(f, (4-e)%3);
				DelaunayTessellation::FacetCirculator circulator_start = tessellation().incident_facets(tet.cell, vertexIndex1, vertexIndex2, tet.cell, f);
				DelaunayTessellation::FacetCirculator circulator = circulator_start;
				OVITO_ASSERT(circulator->first == tet.cell);
				OVITO_ASSERT(circulator->second == f);
				--circulator;
				OVITO_ASSERT(circulator != circulator_start);
				do {
					// Look for the first open cell while going around the edge.
					if(circulator->first->info().flag == false)
						break;
					--circulator;
				}
				while(circulator != circulator_start);
				OVITO_ASSERT(circulator != circulator_start);

				// Get the adjacent cell, which must be solid.
				std::pair<DelaunayTessellation::CellHandle,int> mirrorFacet = tessellation().mirrorFacet(circulator);
				OVITO_ASSERT(mirrorFacet.first->info().flag == true);
				Face* oppositeFace = nullptr;
				// If the cell is a ghost cell, find the corresponding real cell.
				if(mirrorFacet.first->info().isGhost) {
					OVITO_ASSERT(mirrorFacet.first->info().index == -1);
					std::array<int,4> cellVerts;
					for(size_t i = 0; i < 4; i++) {
						cellVerts[i] = mirrorFacet.first->vertex(i)->point().index();
						OVITO_ASSERT(cellVerts[i] != -1);
					}
					std::array<int,3> faceVerts;
					for(size_t i = 0; i < 3; i++) {
						faceVerts[i] = cellVerts[DelaunayTessellation::cellFacetVertexIndex(mirrorFacet.second, i)];
						OVITO_ASSERT(faceVerts[i] != -1);
					}
					std::sort(cellVerts.begin(), cellVerts.end());
					const Tetrahedron& realTet = tetrahedra[cellVerts];
					for(int fi = 0; fi < 4; fi++) {
						if(realTet.meshFacets[fi] == nullptr) continue;
						std::array<int,3> faceVerts2;
						for(size_t i = 0; i < 3; i++) {
							faceVerts2[i] = realTet.cell->vertex(DelaunayTessellation::cellFacetVertexIndex(fi, i))->point().index();
							OVITO_ASSERT(faceVerts2[i] != -1);
						}
						if(std::is_permutation(faceVerts.begin(), faceVerts.end(), faceVerts2.begin())) {
							oppositeFace = realTet.meshFacets[fi];
							break;
						}
					}
				}
				else {
					const Tetrahedron& mirrorTet = tetrahedraList[mirrorFacet.first->info().index]->second;
					oppositeFace = mirrorTet.meshFacets[mirrorFacet.second];
				}
				if(oppositeFace == nullptr)
					throw Exception(DislocationAnalysisModifier::tr("Cannot construct interface mesh for this input dataset. Opposite cell face not found."));
				OVITO_ASSERT(oppositeFace != facet);
				Edge* oppositeEdge = oppositeFace->edges();
				do {
					OVITO_CHECK_POINTER(oppositeEdge);
					if(oppositeEdge->vertex1() == edge->vertex2()) {
						edge->linkToOppositeEdge(oppositeEdge);
						OVITO_ASSERT(edge->physicalVector.equals(-oppositeEdge->physicalVector));
						OVITO_ASSERT(edge->clusterTransition == oppositeEdge->clusterTransition->reverse);
						OVITO_ASSERT(edge->clusterTransition->reverse == oppositeEdge->clusterTransition);
						OVITO_ASSERT(edge->clusterVector.equals(-oppositeEdge->clusterTransition->transform(oppositeEdge->clusterVector)));
						break;
					}
					oppositeEdge = oppositeEdge->nextFaceEdge();
				}
				while(oppositeEdge != oppositeFace->edges());
				if(edge->oppositeEdge() == nullptr)
					throw Exception(DislocationAnalysisModifier::tr("Cannot construct interface mesh for this input dataset. Opposite half-edge not found."));
			}
		}
	}

	// Make sure each vertex is only part of a single manifold.
	duplicateSharedVertices();

	qDebug() << "Number of interface mesh faces:" << faceCount();

	// Validate constructed mesh.
#ifdef OVITO_DEBUG
	for(Vertex* vertex : vertices()) {
		int edgeCount = 0;
		for(Edge* edge = vertex->edges(); edge != nullptr; edge = edge->nextVertexEdge()) {
			OVITO_ASSERT(edge->oppositeEdge()->oppositeEdge() == edge);
			OVITO_ASSERT(edge->physicalVector.equals(-edge->oppositeEdge()->physicalVector));
			OVITO_ASSERT(edge->clusterTransition == edge->oppositeEdge()->clusterTransition->reverse);
			OVITO_ASSERT(edge->clusterTransition->reverse == edge->oppositeEdge()->clusterTransition);
			OVITO_ASSERT(edge->clusterVector.equals(-edge->oppositeEdge()->clusterTransition->transform(edge->oppositeEdge()->clusterVector)));
			OVITO_ASSERT(edge->nextFaceEdge()->prevFaceEdge() == edge);
			OVITO_ASSERT(edge->prevFaceEdge()->nextFaceEdge() == edge);
			OVITO_ASSERT(edge->nextFaceEdge()->nextFaceEdge() == edge->prevFaceEdge());
			OVITO_ASSERT(edge->prevFaceEdge()->prevFaceEdge() == edge->nextFaceEdge());
			edgeCount++;
		}
		OVITO_ASSERT(edgeCount == vertex->numEdges());
		OVITO_ASSERT(edgeCount >= 3);

		Edge* edge = vertex->edges();
		do {
			OVITO_ASSERT(edgeCount > 0);
			Edge* nextEdge = edge->oppositeEdge()->nextFaceEdge();
			OVITO_ASSERT(nextEdge->prevFaceEdge()->oppositeEdge() == edge);
			edge = nextEdge;
			edgeCount--;
		}
		while(edge != vertex->edges());
		OVITO_ASSERT(edgeCount == 0);
	}
#endif

	progress.endProgressSubSteps();
	return true;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
