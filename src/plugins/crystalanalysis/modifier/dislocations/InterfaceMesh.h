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

#ifndef __OVITO_CA_INTERFACE_MESH_H
#define __OVITO_CA_INTERFACE_MESH_H

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <core/utilities/concurrent/FutureInterface.h>
#include <core/utilities/mesh/HalfEdgeMesh.h>
#include "ElasticMapping.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/**
 * The interface mesh that separates the 'bad' crystal regions from the 'good' crystal regions.
 */
class InterfaceMesh : public HalfEdgeMesh
{
public:

	/// Constructor.
	InterfaceMesh(const ElasticMapping& elasticMapping) :
		_elasticMapping(elasticMapping) {}

	/// Returns the mapping from the physical configuration of the system
	/// to the stress-free imaginary configuration.
	const ElasticMapping& elasticMapping() const { return _elasticMapping; }

	/// Returns the underlying tessellation of the atomistic system.
	const DelaunayTessellation& tessellation() const { return elasticMapping().tessellation(); }

	/// Returns the structure analysis object.
	const StructureAnalysis& structureAnalysis() const { return elasticMapping().structureAnalysis(); }

	/// Classifies each tetrahedron of the tessellation as being either good or bad.
	bool classifyTetrahedra(FutureInterfaceBase& progress);

	/// Creates the mesh facets separating good and bad tetrahedra.
	bool createSeparatingFacets(FutureInterfaceBase& progress);

	/// Links half-edges to opposite half-edges.
	bool linkHalfEdges(FutureInterfaceBase& progress);

private:

	/// The underlying mapping from the physical configuration of the system
	/// to the stress-free imaginary configuration.
	const ElasticMapping& _elasticMapping;

	/// The of tessellation cells belonging to the good crystal region.
	int _numGoodTetrahedra;

	/// Indicates that all tessellation cells belong to the good region.
	bool _isCompletelyGood;

};

}	// End of namespace
}	// End of namespace
}	// End of namespace

#endif // __OVITO_CA_INTERFACE_MESH_H
