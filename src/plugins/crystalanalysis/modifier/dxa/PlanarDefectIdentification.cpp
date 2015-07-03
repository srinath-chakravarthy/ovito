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
#include "PlanarDefectIdentification.h"
#include "DislocationAnalysisEngine.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/******************************************************************************
* Extracts the planar defects.
******************************************************************************/
bool PlanarDefectIdentification::extractPlanarDefects(int crystalStructure, FutureInterfaceBase& progress)
{
	// Build path from each planar defect atom to the closest crystal cluster.
	bool done;
	do {
		done = true;

		for(size_t atomIndex = 0; atomIndex < _incidentEdges.size(); atomIndex++) {
			Cluster* cluster = elasticMapping().clusterOfVertex(atomIndex);
			OVITO_ASSERT(cluster != nullptr);

			if(cluster->structure == crystalStructure || cluster->id == 0)
				continue;

			ElasticMapping::TessellationEdge* bestEdge = _incidentEdges[atomIndex];
			Cluster* bestSourceCluster = nullptr;
			int bestDepth = 0;
			Vector3 bestVector = Vector3::Zero();
			ClusterTransition* t = clusterGraph().createSelfTransition(cluster);
			for(ElasticMapping::TessellationEdge* edge = bestEdge; edge != nullptr; edge = _incidentEdges[edge->vertex1]) {
				bestSourceCluster = edge->clusterTransition->cluster1;
				bestDepth++;
				t = clusterGraph().concatenateClusterTransitions(edge->clusterTransition, t);
				bestVector += t->transform(edge->clusterVector);
			}
			OVITO_ASSERT(bestSourceCluster == nullptr || bestSourceCluster->structure == crystalStructure);
			for(ElasticMapping::TessellationEdge* edge = elasticMapping().vertexEdges(atomIndex); edge != nullptr; edge = edge->next) {
				if(!edge->hasClusterVector()) continue;
				if(edge->reverse == bestEdge) continue;
				Cluster* sourceCluster = edge->clusterTransition->cluster2;
				OVITO_ASSERT(sourceCluster->id > 0);
				OVITO_ASSERT(edge->vertex2 != atomIndex);
				ClusterTransition* t = edge->clusterTransition->reverse;
				Vector3 vector = -edge->clusterVector;
				int depth = 1;
				for(ElasticMapping::TessellationEdge* edge2 = _incidentEdges[edge->vertex2]; edge2 != nullptr; edge2 = _incidentEdges[edge2->vertex1]) {
					if(edge2->vertex1 == atomIndex)
						break;
					OVITO_ASSERT(edge2 != edge && edge2 != edge->reverse);
					sourceCluster = edge2->clusterTransition->cluster1;
					depth++;
					t = clusterGraph().concatenateClusterTransitions(edge2->clusterTransition, t);
					vector += t->transform(edge2->clusterVector);
				}
				if(sourceCluster->structure != crystalStructure) {
					continue;
				}
				if(bestSourceCluster != nullptr) {
					if(depth > bestDepth)
						continue;
					else if(depth == bestDepth) {
						if(structureAnalysis().atomCluster(edge->vertex2)->structure != crystalStructure)
							continue;
						else {

							// Check if it is a complete lattice vector.
							Cluster* cluster = edge->clusterTransition->cluster2;
							Vector3 d = edge->clusterTransition->transform(edge->clusterVector);
							Vector3 rd = StructureAnalysis::latticeStructure(cluster->structure).primitiveCellInverse * d;
							bool isStackingFault = false;
							for(int dim = 0; dim < 3; dim++) {
								if(std::abs(rd[dim] - floor(rd[dim]+0.5f)) > CA_LATTICE_VECTOR_EPSILON) {
									isStackingFault = true;
									break;
								}
							}
							if(isStackingFault)
								continue;
							if(edge->vertex2 > bestEdge->vertex1)
								continue;
						}
					}
				}
				bestSourceCluster = sourceCluster;
				bestEdge = edge->reverse;
				bestDepth = depth;
				bestVector = vector;
				done = false;
				_incidentEdges[atomIndex] = bestEdge;
				OVITO_ASSERT(bestEdge->vertex2 == atomIndex);

#if 0
				Vector3 bestVector2 = Vector3::Zero();
				t = clusterGraph().createSelfTransition(cluster);
				for(ElasticMapping::TessellationEdge* edge = bestEdge; edge != nullptr; edge = _incidentEdges[edge->vertex1]) {
					t = clusterGraph().concatenateClusterTransitions(edge->clusterTransition, t);
					bestVector2 += t->transform(edge->clusterVector);
				}
				OVITO_ASSERT(bestVector2.equals(bestVector));
#endif
			}
		}
		qDebug() << "done = " << done;
	}
	while(!done);


#if 1
	QFile file("mapping.dump");
	file.open(QIODevice::WriteOnly);
	QTextStream stream(&file);

	AffineTransformation simCell = structureAnalysis().cell().matrix();
	FloatType xlo = simCell.translation().x();
	FloatType ylo = simCell.translation().y();
	FloatType zlo = simCell.translation().z();
	FloatType xhi = simCell.column(0).x() + xlo;
	FloatType yhi = simCell.column(1).y() + ylo;
	FloatType zhi = simCell.column(2).z() + zlo;
	FloatType xy = simCell.column(1).x();
	FloatType xz = simCell.column(2).x();
	FloatType yz = simCell.column(2).y();
	xlo += std::min((FloatType)0, std::min(xy, std::min(xz, xy+xz)));
	xhi += std::max((FloatType)0, std::max(xy, std::max(xz, xy+xz)));
	ylo += std::min((FloatType)0, yz);
	yhi += std::max((FloatType)0, yz);

	stream << "ITEM: TIMESTEP\n";
	stream << 0 << '\n';
	stream << "ITEM: NUMBER OF ATOMS\n";
	stream <<  _incidentEdges.size() << '\n';
	stream << "ITEM: BOX BOUNDS xy xz yz";
	stream << (structureAnalysis().cell().pbcFlags()[0] ? " pp" : " ff");
	stream << (structureAnalysis().cell().pbcFlags()[1] ? " pp" : " ff");
	stream << (structureAnalysis().cell().pbcFlags()[2] ? " pp" : " ff");
	stream << '\n';
	stream << xlo << ' ' << xhi << ' ' << xy << '\n';
	stream << ylo << ' ' << yhi << ' ' << xz << '\n';
	stream << zlo << ' ' << zhi << ' ' << yz << '\n';
	stream << "ITEM: ATOMS x y z Displacement.X Displacement.Y Displacement.Z" << '\n';
	for(size_t atomIndex = 0; atomIndex < _incidentEdges.size(); atomIndex++) {
		Point3 p = structureAnalysis().positions()->getPoint3(atomIndex);
		stream << p.x() << ' ' << p.y() << ' ' << p.z() << ' ';
		Vector3 d = Vector3::Zero();
		for(ElasticMapping::TessellationEdge* edge = _incidentEdges[atomIndex]; edge != nullptr; edge = _incidentEdges[edge->vertex1]) {
			d += structureAnalysis().cell().wrapVector(structureAnalysis().positions()->getPoint3(edge->vertex1) - structureAnalysis().positions()->getPoint3(edge->vertex2));
		}
		stream << d.x() << ' ' << d.y() << ' ' << d.z() << '\n';
	}
#endif

	std::map<DelaunayTessellation::VertexHandle,int> meshVertexMap;

	auto determineAnchorVertex = [this, crystalStructure](DelaunayTessellation::CellHandle cell) {
		int indices[4];
		for(int v = 0; v < 4; v++) {
			indices[v] = cell->vertex(v)->point().index();
		}

		int faultAnchor = -1;
		for(int v = 0; v < 4; v++) {
			if(elasticMapping().clusterOfVertex(indices[v])->structure != crystalStructure && indices[v] > faultAnchor) faultAnchor = indices[v];
		}

		if(faultAnchor != -1) {
			FloatType largestZ = 0;
			int anchor = faultAnchor;
			for(int v = 0; v < 4; v++) {
				if(indices[v] == faultAnchor) continue;
				ElasticMapping::TessellationEdge* edge = elasticMapping().findEdge(faultAnchor, indices[v]);
				OVITO_ASSERT(edge != nullptr && edge->hasClusterVector());
				if(edge->clusterVector.z() > largestZ + CA_LATTICE_VECTOR_EPSILON) {
					anchor = indices[v];
					largestZ = edge->clusterVector.z();
				}
				else if(edge->clusterVector.z() > largestZ - CA_LATTICE_VECTOR_EPSILON && indices[v] > faultAnchor) {
					anchor = indices[v];
					largestZ = edge->clusterVector.z();
				}
			}
			return anchor;
		}
		else {
			int anchor = -1;
			for(int v = 0; v < 4; v++) {
				int idx = cell->vertex(v)->point().index();
				if(idx > anchor) anchor = idx;
			}
			return anchor;
		}
	};

	// Iterate over the good tetrahedra of the tessellation.
	for(DelaunayTessellation::CellIterator cell1 = tessellation().begin_cells(); cell1 != tessellation().end_cells(); ++cell1) {

		// Skip bad tetrahedra and ghost tetrahedra.
		if(cell1->info().index == -1) continue;

		// Iterate over the four faces of the tetrahedron cell.
		for(int f = 0; f < 4; f++) {

			// Get the adjacent tetrahedron.
			auto mirrorCell = tessellation().dt().tds().mirror_facet(DelaunayTessellation::DT::Triangulation_data_structure::Facet(cell1, f));
			DelaunayTessellation::CellHandle cell2 = mirrorCell.first;

			// Check if it is also a good tet. If not, skip it.
			if(!cell2->info().flag) continue;

			// We will visit this pair of cells twice, therefore we have to skip it in half of the cases.
			// Use the indices of the two vertices not shared by the two tets as a criterion.
			OVITO_ASSERT(cell1->vertex(f) != cell2->vertex(mirrorCell.second));
			if(cell1->vertex(f)->point().index() > cell2->vertex(mirrorCell.second)->point().index())
				continue;

#if 0
			int fv[3];
			for(int v = 0; v < 3; v++)
				fv[v] = cell1->vertex(DelaunayTessellation::cellFacetVertexIndex(f, v))->point().index();
			std::sort(std::begin(fv), std::end(fv));
			bool print = false;
			if(fv[0] == 83-1 && fv[1] == 87-1 && fv[2] == 88-1) {
				print = true;
			}
#endif

			// Determine the anchor vertex for each of the two cells.
			int anchor1 = determineAnchorVertex(cell1);
			int anchor2 = determineAnchorVertex(cell2);

			// Determine the lattice coordinates of the first cell's anchor vertex.
			Vector3 coord1 = Vector3::Zero();
			ClusterTransition* transition1 = clusterGraph().createSelfTransition(elasticMapping().clusterOfVertex(anchor1));
			for(ElasticMapping::TessellationEdge* edge = _incidentEdges[anchor1]; edge != nullptr; edge = _incidentEdges[edge->vertex1]) {
				OVITO_ASSERT(edge->hasClusterVector());
				coord1 = edge->clusterTransition->reverseTransform(coord1) + edge->clusterVector;
				transition1 = clusterGraph().concatenateClusterTransitions(edge->clusterTransition, transition1);
			}

			// Determine the lattice coordinates and orientation of the second cell's anchor vertex.
			Vector3 coord2 = Vector3::Zero();
			ClusterTransition* transition2 = clusterGraph().createSelfTransition(elasticMapping().clusterOfVertex(anchor2));
			for(ElasticMapping::TessellationEdge* edge = _incidentEdges[anchor2]; edge != nullptr; edge = _incidentEdges[edge->vertex1]) {
				OVITO_ASSERT(edge->hasClusterVector());
				coord2 = edge->clusterTransition->reverseTransform(coord2) + edge->clusterVector;
				transition2 = clusterGraph().concatenateClusterTransitions(edge->clusterTransition, transition2);
			}

			// Take a vertex that is shared by the two tetrahedra.
			int sharedVertex = cell1->vertex((f + 1) % 4)->point().index();

			// Connect paths at the shared vertex.
			if(sharedVertex != anchor1) {
				ElasticMapping::TessellationEdge* edge = elasticMapping().findEdge(anchor1, sharedVertex);
				OVITO_ASSERT(edge != nullptr);
				OVITO_ASSERT(edge->hasClusterVector());
				coord1 += transition1->reverseTransform(edge->clusterVector);
				transition1 = clusterGraph().concatenateClusterTransitions(transition1, edge->clusterTransition);
			}
			if(sharedVertex != anchor2) {
				ElasticMapping::TessellationEdge* edge = elasticMapping().findEdge(anchor2, sharedVertex);
				OVITO_ASSERT(edge != nullptr);
				OVITO_ASSERT(edge->hasClusterVector());
				coord2 += transition2->reverseTransform(edge->clusterVector);
				transition2 = clusterGraph().concatenateClusterTransitions(transition2, edge->clusterTransition);
			}

			// Compute the misorientation.
			ClusterTransition* fullTransition = clusterGraph().concatenateClusterTransitions(transition1, transition2->reverse);
			OVITO_ASSERT(fullTransition->cluster1->structure == crystalStructure);
			OVITO_ASSERT(fullTransition->cluster2->structure == crystalStructure);
			// Check if it is a grain boundary.
			if(!fullTransition->isSelfTransition()) {
				bool isSymmetryRotation = false;
				for(const auto& symElement : StructureAnalysis::latticeStructure(crystalStructure).permutations) {
					if(fullTransition->tm.equals(symElement.first, CA_TRANSITION_MATRIX_EPSILON)) {
						isSymmetryRotation = true;
						break;
					}
				}
				if(!isSymmetryRotation) {
					continue;	// It's a grain boundary.
				}
			}

			// Compute the displacement shift vector.
			Vector3 displacement = coord1 - fullTransition->reverseTransform(coord2);

			// Check if it is a complete lattice vector.
			Vector3 rd = StructureAnalysis::latticeStructure(crystalStructure).primitiveCellInverse * displacement;
			bool isStackingFault = false;
			for(int dim = 0; dim < 3; dim++) {
				if(std::abs(rd[dim] - floor(rd[dim]+0.5f)) > CA_LATTICE_VECTOR_EPSILON) {
					isStackingFault = true;
					break;
				}
			}
			if(!isStackingFault)
				continue;

			TriMeshFace& triface = planarDefects()->mesh().addFace();
			for(int v = 0; v < 3; v++) {
				DelaunayTessellation::VertexHandle tessellationVertex = cell1->vertex(DelaunayTessellation::cellFacetVertexIndex(f, v));
				auto iter = meshVertexMap.find(tessellationVertex);
				int meshVertex;
				if(iter == meshVertexMap.end()) {
					meshVertex = planarDefects()->mesh().addVertex((Point3)tessellationVertex->point());
					meshVertexMap.insert(std::make_pair(tessellationVertex, meshVertex));
				}
				else meshVertex = iter->second;
				triface.setVertex(v, meshVertex);
			}
		}
	}

	qDebug() << "Number of planar defect triangles:" << planarDefects()->mesh().faceCount();

	return true;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
