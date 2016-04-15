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
#include "DelaunayTessellation2.h"

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

	std::vector<double> point_data;

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

	// Insert the original points first.
	for(size_t i = 0; i < numPoints; i++, ++positions) {

		// Skip points which are not included.
		if(selectedPoints && !*selectedPoints++)
			continue;

		// Add a small random perturbation to the particle positions to make the Delaunay triangulation more robust
		// against singular input data, e.g. particles forming an ideal crystal lattice.
		Point3 wp = simCell.wrapPoint(*positions);
		point_data.push_back((double)wp.x() + displacement(rng));
		point_data.push_back((double)wp.y() + displacement(rng));
		point_data.push_back((double)wp.z() + displacement(rng));

		if(progress && progress->isCanceled())
			return false;
	}

	// Initialize the Geogram library.
	static bool isGeogramInitialized = false;
	if(!isGeogramInitialized) {
		isGeogramInitialized = true;
		GEO::initialize();
		GEO::set_assert_mode(GEO::ASSERT_ABORT);
	}

	qDebug() << "Passing" << (point_data.size()/3) << "points to Geogram";

	// Create the internal Delaunay generator object.
	_dt = new GEO::Delaunay3d();
	_dt->set_keeps_infinite(true);
	_dt->set_vertices(numPoints, point_data.data());
	_cellInfo.resize(_dt->nb_cells());

	qDebug() << "Number of Geogram vertices:" << _dt->nb_vertices();
	qDebug() << "Number of Geogram cells:" << _dt->nb_cells();
	qDebug() << "Number of Geogram finite_cells:" << _dt->nb_finite_cells();

	return true;
}

DelaunayTessellation::Facet DelaunayTessellation::mirrorFacet(CellHandle cell, int face) const
{
	signed_index_t adjacentCell = _dt->cell_adjacent(cell, face);
	OVITO_ASSERT(adjacentCell >= 0);
	return std::pair<DelaunayTessellation::CellHandle, int>(adjacentCell, _dt->adjacent_index(adjacentCell, cell));
}

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

	auto determinant = [](
	 const RT& a00,  const RT& a01,  const RT& a02,
	 const RT& a10,  const RT& a11,  const RT& a12,
	 const RT& a20,  const RT& a21,  const RT& a22)
	{
	  const RT m02 = a00*a21 - a20*a01;
	  const RT m01 = a00*a11 - a10*a01;
	  const RT m12 = a10*a21 - a20*a11;
	  const RT m012 = m01*a22 - m02*a12 + m12*a02;
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

	return (square(num_x) + square(num_y) + square(num_z)) / square(2 * den);
}


}	// End of namespace
}	// End of namespace
}	// End of namespace
