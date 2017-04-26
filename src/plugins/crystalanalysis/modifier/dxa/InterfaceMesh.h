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
#include <core/utilities/concurrent/Promise.h>
#include <core/utilities/mesh/HalfEdgeMesh.h>
#include "ElasticMapping.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

struct BurgersCircuit;				// defined in BurgersCircuit.h
struct BurgersCircuitSearchStruct;	// defined in DislocationTracer.cpp
class DislocationTracer;			// defined in DislocationTracer.h

struct InterfaceMeshVertex
{
	/// This pointer is used during Burgers circuit search on the mesh.
	/// This field is used by the DislocationTracer class.
	BurgersCircuitSearchStruct* burgersSearchStruct = nullptr;

	/// A bit flag used by various algorithms.
	bool visited = false;
};

struct InterfaceMeshFace
{
	/// The Burgers circuit which has swept this facet.
	/// This field is used by the DislocationTracer class.
	BurgersCircuit* circuit = nullptr;
};

struct InterfaceMeshEdge
{
	/// The (unwrapped) vector connecting the two vertices.
	Vector3 physicalVector;

	/// The ideal vector in the reference configuration assigned to this edge.
	Vector3 clusterVector;

	/// The cluster transition when going from the cluster of node 1 to the cluster of node 2.
	ClusterTransition* clusterTransition;

	/// The Burgers circuit going through this edge.
	/// This field is used by the DislocationTracer class.
	BurgersCircuit* circuit = nullptr;

	/// If this edge is part of a Burgers circuit, then this points to the next edge in the circuit.
	/// This field is used by the DislocationTracer class.
	HalfEdgeMesh<InterfaceMeshEdge, InterfaceMeshFace, InterfaceMeshVertex>::Edge* nextCircuitEdge = nullptr;
};

/**
 * The interface mesh that separates the 'bad' crystal regions from the 'good' crystal regions.
 */
class InterfaceMesh : public HalfEdgeMesh<InterfaceMeshEdge, InterfaceMeshFace, InterfaceMeshVertex>
{
public:

	/// Constructor.
	InterfaceMesh(ElasticMapping& elasticMapping) :
		_elasticMapping(elasticMapping) {}

	/// Returns the mapping from the physical configuration of the system
	/// to the stress-free imaginary configuration.
	ElasticMapping& elasticMapping() { return _elasticMapping; }

	/// Returns the mapping from the physical configuration of the system
	/// to the stress-free imaginary configuration.
	const ElasticMapping& elasticMapping() const { return _elasticMapping; }

	/// Returns the underlying tessellation of the atomistic system.
	DelaunayTessellation& tessellation() { return elasticMapping().tessellation(); }

	/// Returns the structure analysis object.
	const StructureAnalysis& structureAnalysis() const { return elasticMapping().structureAnalysis(); }

	/// Creates the mesh facets separating good and bad tetrahedra.
	bool createMesh(FloatType maximumNeighborDistance, ParticleProperty* crystalClusters, PromiseBase& progress);

	/// Returns whether all tessellation cells belong to the good region.
	bool isCompletelyGood() const { return _isCompletelyGood; }

	/// Returns whether all tessellation cells belong to the bad region.
	bool isCompletelyBad() const { return _isCompletelyBad; }

	/// Generates the nodes and facets of the defect mesh based on the interface mesh.
	bool generateDefectMesh(const DislocationTracer& tracer, HalfEdgeMesh<>& defectMesh, PromiseBase& progress);

private:

	/// The underlying mapping from the physical configuration of the system
	/// to the stress-free imaginary configuration.
	ElasticMapping& _elasticMapping;

	/// Indicates that all tessellation cells belong to the good region.
	bool _isCompletelyGood;

	/// Indicates that all tessellation cells belong to the bad region.
	bool _isCompletelyBad;
};

}	// End of namespace
}	// End of namespace
}	// End of namespace


