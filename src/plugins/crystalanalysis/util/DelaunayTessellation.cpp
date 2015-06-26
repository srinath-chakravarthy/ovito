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

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include "DelaunayTessellation.h"

#include <boost/random/mersenne_twister.hpp>
#if BOOST_VERSION > 146000
#include <boost/random/uniform_real_distribution.hpp>
#else
#include <boost/random/uniform_real.hpp>
#endif

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/******************************************************************************
* Generates the tessellation.
******************************************************************************/
bool DelaunayTessellation::generateTessellation(const SimulationCell& simCell, const Point3* positions, size_t numPoints, FloatType ghostLayerSize, FutureInterfaceBase* progress)
{
	if(progress) progress->setProgressRange(0);
	std::vector<DT::Point> cgalPoints;

	const double epsilon = 1e-7;

	// Set up random number generator to generate random perturbations.
#if 0
	std::minstd_rand rng;
	rng.seed(1);
	std::uniform_real_distribution<double> displacement(-epsilon, epsilon);
#else
	#if BOOST_VERSION > 146000
		boost::random::mt19937 rng;
		rng.seed(1);
		boost::random::uniform_real_distribution displacement(-epsilon, epsilon);
	#else
		boost::mt19937 rng;
		rng.seed(1);
		boost::uniform_real<> displacement(-epsilon, epsilon);
	#endif
#endif

	// Insert the original points first.
	cgalPoints.reserve(numPoints);
	for(size_t i = 0; i < numPoints; i++, ++positions) {

		// Add a small random perturbation to the particle positions to make the Delaunay triangulation more robust
		// against singular input data, e.g. particles forming an ideal crystal lattice.
		Point3 wp = simCell.wrapPoint(*positions);
		double coords[3];
		coords[0] = (double)wp.x() + displacement(rng);
		coords[1] = (double)wp.y() + displacement(rng);
		coords[2] = (double)wp.z() + displacement(rng);

		cgalPoints.push_back(Point3WithIndex(coords[0], coords[1], coords[2], i, false));

		if(progress && progress->isCanceled())
			return false;
	}

	int vertexCount = numPoints;

	Vector3I stencilCount;
	FloatType cuts[3][2];
	Vector3 cellNormals[3];
	for(size_t dim = 0; dim < 3; dim++) {
		cellNormals[dim] = simCell.cellNormalVector(dim);
		cuts[dim][0] = cellNormals[dim].dot(simCell.reducedToAbsolute(Point3(0,0,0)) - Point3::Origin());
		cuts[dim][1] = cellNormals[dim].dot(simCell.reducedToAbsolute(Point3(1,1,1)) - Point3::Origin());

		if(simCell.pbcFlags()[dim]) {
			stencilCount[dim] = (int)ceil(ghostLayerSize / simCell.matrix().column(dim).dot(cellNormals[dim]));
			cuts[dim][0] -= ghostLayerSize;
			cuts[dim][1] += ghostLayerSize;
		}
		else {
			stencilCount[dim] = 0;
			cuts[dim][0] -= ghostLayerSize;
			cuts[dim][1] += ghostLayerSize;
		}
	}

	// Create ghost images of input vertices.
	for(int ix = -stencilCount[0]; ix <= +stencilCount[0]; ix++) {
		for(int iy = -stencilCount[1]; iy <= +stencilCount[1]; iy++) {
			for(int iz = -stencilCount[2]; iz <= +stencilCount[2]; iz++) {
				if(ix == 0 && iy == 0 && iz == 0) continue;

				Vector3 shift = simCell.reducedToAbsolute(Vector3(ix,iy,iz));
				Vector_3<double> shiftd = (Vector_3<double>)shift;
				for(int vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++) {
					if(progress && progress->isCanceled())
						return false;

					Point3 pimage = (Point3)cgalPoints[vertexIndex] + shift;
					bool isClipped = false;
					for(size_t dim = 0; dim < 3; dim++) {
						FloatType d = cellNormals[dim].dot(pimage - Point3::Origin());
						if(d < cuts[dim][0] || d > cuts[dim][1]) {
							isClipped = true;
							break;
						}
					}
					if(!isClipped) {
						cgalPoints.push_back(Point3WithIndex(cgalPoints[vertexIndex].x() + shift.x(),
								cgalPoints[vertexIndex].y() + shift.y(), cgalPoints[vertexIndex].z() + shift.z(), vertexIndex, true));
					}
				}
			}
		}
	}

	if(progress && progress->isCanceled())
		return false;

	// Disabled the following line, because spatial_sort() seems to produce different results
	// on different platforms.

	//CGAL::spatial_sort(cgalPoints.begin(), cgalPoints.end(), _dt.geom_traits());

	if(progress) {
		progress->setProgressRange(cgalPoints.size());
		if(!progress->setProgressValue(0))
			return false;
	}

	DT::Vertex_handle hint;
	for(auto p = cgalPoints.begin(); p != cgalPoints.end(); ++p) {
		hint = _dt.insert(*p, hint);

		if(progress) {
			if(!progress->setProgressValueIntermittent(p - cgalPoints.begin()))
				return false;
		}
	}

	// Classify tessellation cells as ghost or local cells.
	_numPrimaryTetrahedra = 0;
	for(CellIterator cell = begin_cells(); cell != end_cells(); ++cell) {
		if(isGhostCell(cell)) {
			cell->info().isGhost = true;
			cell->info().index = -1;
		}
		else {
			cell->info().isGhost = false;
			cell->info().index = _numPrimaryTetrahedra++;
		}
	}

	return true;
}

/******************************************************************************
* Determines whether the given tetrahedral cell is a ghost cell (or an invalid cell).
******************************************************************************/
bool DelaunayTessellation::isGhostCell(CellHandle cell) const
{
	// Find head vertex with the lowest index.
	const auto& p0 = cell->vertex(0)->point();
	int headVertex = p0.index();
	if(headVertex == -1) {
		OVITO_ASSERT(!isValidCell(cell));
		return true;
	}
	bool isGhost = p0.isGhost();
	for(int v = 1; v < 4; v++) {
		const auto& p = cell->vertex(v)->point();
		if(p.index() == -1) {
			OVITO_ASSERT(!isValidCell(cell));
			return true;
		}
		if(p.index() < headVertex) {
			headVertex = p.index();
			isGhost = p.isGhost();
		}
	}

	OVITO_ASSERT(isValidCell(cell));
	return isGhost;
}

/******************************************************************************
* Writes the tessellation to a VTK file for visualization.
******************************************************************************/
void DelaunayTessellation::dumpToVTKFile(const QString& filename) const
{
	QFile file(filename);
	file.open(QIODevice::WriteOnly);
	QTextStream stream(&file);

	// Write VTK output file.
	stream << "# vtk DataFile Version 3.0" << endl;
	stream << "# Dislocation output" << endl;
	stream << "ASCII" << endl;
	stream << "DATASET UNSTRUCTURED_GRID" << endl;
	stream << "POINTS " << (_dt.number_of_vertices()) << " double" << endl;
	std::map<VertexHandle,int> outputVertices;
	for(auto v = _dt.finite_vertices_begin(); v != _dt.finite_vertices_end(); ++v) {
		outputVertices.insert(std::make_pair((VertexHandle)v, (int)outputVertices.size()));
		stream << v->point().x() << " " << v->point().y() << " " << v->point().z() << "\n";
	}
	OVITO_ASSERT(outputVertices.size() == _dt.number_of_vertices());

	int numCells = _dt.number_of_finite_cells();
	stream << endl << "CELLS " << numCells << " " << (numCells * 5) << endl;
	for(auto cell = _dt.finite_cells_begin(); cell != _dt.finite_cells_end(); ++cell) {
		stream << "4";
		for(int i = 0; i < 4; i++) {
			stream << " " << outputVertices[cell->vertex(i)];
		}
		stream << "\n";
	}
	stream << endl << "CELL_TYPES " << numCells << "\n";
	for(size_t i = 0; i < numCells; i++)
		stream << "10" << "\n";

	stream << endl << "CELL_DATA " << numCells << "\n";
	stream << endl << "SCALARS flag int" << "\n";
	stream << "LOOKUP_TABLE default" << "\n";
	for(auto cell = _dt.finite_cells_begin(); cell != _dt.finite_cells_end(); ++cell) {
		stream << cell->info().flag << "\n";
	}
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
