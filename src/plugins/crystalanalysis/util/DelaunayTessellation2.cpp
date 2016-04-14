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
	qDebug() << "Geogram thread_safe:" << _dt->thread_safe();

	for(size_type i = 0; i < _dt->nb_cells(); i++) {
		if(_dt->cell_is_infinite(i))
			qDebug() << "infinite cell";
		for(size_type v = 0; v < 4; v++) {
			if(_dt->cell_vertex(i, v) < 0)
				qDebug() << _dt->cell_vertex(i, v);
		}
	}

	return true;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
