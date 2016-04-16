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
bool DelaunayTessellation::generateTessellation(const SimulationCell& simCell, const Point3* positions, size_t numPoints, FloatType ghostLayerSize, const int* selectedPoints, FutureInterfaceBase* progress)
{
	if(progress) progress->setProgressRange(0);

	const double epsilon = 2e-5;

	// Set up random number generator to generate random perturbations.
#if 0
	std::minstd_rand rng;
	std::uniform_real_distribution<double> displacement(-epsilon, epsilon);
#else
	#if BOOST_VERSION > 146000
		boost::random::mt19937 rng;
		boost::random::uniform_real_distribution displacement(-epsilon, epsilon);
	#else
		boost::mt19937 rng;
		boost::uniform_real<> displacement(-epsilon, epsilon);
	#endif
#endif
	rng.seed(4);

	_simCell = simCell;

	// Build the list of input points.
	_particleIndices.clear();
	_pointData.clear();
	for(size_t i = 0; i < numPoints; i++, ++positions) {

		// Skip points which are not included.
		if(selectedPoints && !*selectedPoints++)
			continue;

		// Add a small random perturbation to the particle positions to make the Delaunay triangulation more robust
		// against singular input data, e.g. particles forming an ideal crystal lattice.
		Point3 wp = simCell.wrapPoint(*positions);
		_pointData.push_back((double)wp.x() + displacement(rng));
		_pointData.push_back((double)wp.y() + displacement(rng));
		_pointData.push_back((double)wp.z() + displacement(rng));
		_particleIndices.push_back((int)i);

		if(progress && progress->isCanceled())
			return false;
	}
	_primaryVertexCount = _particleIndices.size();

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
				auto primaryPos = _pointData.cbegin();
				for(int vertexIndex = 0; vertexIndex < _primaryVertexCount; vertexIndex++) {
					if(progress && progress->isCanceled())
						return false;

					double x = (*primaryPos++) + shiftd.x();
					double y = (*primaryPos++) + shiftd.y();
					double z = (*primaryPos++) + shiftd.z();
					Point3 pimage = Point3(x,y,z);
					bool isClipped = false;
					for(size_t dim = 0; dim < 3; dim++) {
						FloatType d = cellNormals[dim].dot(pimage - Point3::Origin());
						if(d < cuts[dim][0] || d > cuts[dim][1]) {
							isClipped = true;
							break;
						}
					}
					if(!isClipped) {
						_pointData.push_back(x);
						_pointData.push_back(y);
						_pointData.push_back(z);
						_particleIndices.push_back(_particleIndices[vertexIndex]);
					}
				}
			}
		}
	}

	// Initialize the Geogram library.
	static bool isGeogramInitialized = false;
	if(!isGeogramInitialized) {
		isGeogramInitialized = true;
		GEO::initialize();
		GEO::set_assert_mode(GEO::ASSERT_ABORT);
	}

	// Create the internal Delaunay generator object.
	_dt = new GEO::Delaunay3d();
	_dt->set_keeps_infinite(true);
	_dt->set_reorder(true);

	// Construct Delaunay tessellation.
	bool result = _dt->set_vertices(_pointData.size()/3, _pointData.data(), [progress](int value, int maxProgress) {
		if(progress) {
			if(maxProgress != progress->progressMaximum()) progress->setProgressRange(maxProgress);
			return progress->setProgressValueIntermittent(value);
		}
		else return true;
	});
	if(!result) return false;

	// Classify tessellation cells as ghost or local cells.
	_numPrimaryTetrahedra = 0;
	_cellInfo.resize(_dt->nb_cells());
	for(CellIterator cell = begin_cells(); cell != end_cells(); ++cell) {
		if(classifyGhostCell(cell)) {
			_cellInfo[cell].isGhost = true;
			_cellInfo[cell].index = -1;
		}
		else {
			_cellInfo[cell].isGhost = false;
			_cellInfo[cell].index = _numPrimaryTetrahedra++;
		}
	}

	return true;
}

/******************************************************************************
* Determines whether the given tetrahedral cell is a ghost cell (or an invalid cell).
******************************************************************************/
bool DelaunayTessellation::classifyGhostCell(CellHandle cell) const
{
	if(!isValidCell(cell))
		return true;

	// Find head vertex with the lowest index.
	VertexHandle headVertex = cellVertex(cell, 0);
	int headVertexIndex = vertexIndex(headVertex);
	OVITO_ASSERT(headVertexIndex >= 0);
	for(int v = 1; v < 4; v++) {
		VertexHandle p = cellVertex(cell, v);
		int vindex = vertexIndex(p);
		OVITO_ASSERT(vindex >= 0);
		if(vindex < headVertexIndex) {
			headVertex = p;
			headVertexIndex = vindex;
		}
	}

	return isGhostVertex(headVertex);
}

/******************************************************************************
* Alpha test routine.
******************************************************************************/
bool DelaunayTessellation::compare_squared_radius_3(CellHandle cell, FloatType alpha) const
{
	const double* v0 = _dt->vertex_ptr(_dt->cell_vertex(cell, 0));
	const double* v1 = _dt->vertex_ptr(_dt->cell_vertex(cell, 1));
	const double* v2 = _dt->vertex_ptr(_dt->cell_vertex(cell, 2));
	const double* v3 = _dt->vertex_ptr(_dt->cell_vertex(cell, 3));
	double px = v0[0], py = v0[1], pz = v0[2];
	double qx = v1[0], qy = v1[1], qz = v1[2];
	double rx = v2[0], ry = v2[1], rz = v2[2];
	double sx = v3[0], sy = v3[1], sz = v3[2];

	auto square = [](double d) { return d*d; };

	// Translate p to origin to simplify the expression.
	double qpx = qx-px;
	double qpy = qy-py;
	double qpz = qz-pz;
	double qp2 = square(qpx) + square(qpy) + square(qpz);
	double rpx = rx-px;
	double rpy = ry-py;
	double rpz = rz-pz;
	double rp2 = square(rpx) + square(rpy) + square(rpz);
	double spx = sx-px;
	double spy = sy-py;
	double spz = sz-pz;
	double sp2 = square(spx) + square(spy) + square(spz);

	auto determinant = [](double a00, double a01, double a02,
			double a10, double a11, double a12,
			double a20, double a21, double a22)
	{
		double m02 = a00*a21 - a20*a01;
		double m01 = a00*a11 - a10*a01;
		double m12 = a10*a21 - a20*a11;
		double m012 = m01*a22 - m02*a12 + m12*a02;
		return m012;
	};

	double num_x = determinant(qpy,qpz,qp2,
							   rpy,rpz,rp2,
							   spy,spz,sp2);
	double num_y = determinant(qpx,qpz,qp2,
							   rpx,rpz,rp2,
							   spx,spz,sp2);
	double num_z = determinant(qpx,qpy,qp2,
							   rpx,rpy,rp2,
							   spx,spy,sp2);
	double den   = determinant(qpx,qpy,qpz,
							   rpx,rpy,rpz,
							   spx,spy,spz);

	return (square(num_x) + square(num_y) + square(num_z)) / square(2 * den) < alpha;
}


}	// End of namespace
}	// End of namespace
}	// End of namespace
