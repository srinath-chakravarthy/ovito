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

#pragma once


#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <core/utilities/mesh/TriMesh.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/**
 * This class stores the extracted planar defects (stacking faults & grain boundaries).
 */
class PlanarDefects : public QSharedData
{
public:

	/// Default constructor.
	PlanarDefects() {}

	/// Returns the internal mesh containing the triangles.
	TriMesh& mesh() { return _mesh; }

	/// Returns the internal mesh containing the triangles.
	TriMesh& grainBoundaryMesh() { return _grainBoundaryMesh; }

private:

	/// The mesh containing the triangles.
	TriMesh _mesh;

	/// The mesh containing the triangles.
	TriMesh _grainBoundaryMesh;
};

}	// End of namespace
}	// End of namespace
}	// End of namespace



