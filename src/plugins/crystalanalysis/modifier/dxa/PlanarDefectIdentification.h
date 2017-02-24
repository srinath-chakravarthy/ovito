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
#include <core/utilities/MemoryPool.h>
#include <core/utilities/concurrent/FutureInterface.h>
#include <plugins/crystalanalysis/data/Cluster.h>
#include <plugins/crystalanalysis/data/ClusterGraph.h>
#include <plugins/crystalanalysis/data/PlanarDefects.h>
#include "ElasticMapping.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/**
 * Extract planar defects (stacking faults & grain boundaries) from a crystal.
 */
class PlanarDefectIdentification
{
public:

	/// Constructor.
	PlanarDefectIdentification(ElasticMapping& elasticMapping) :
		_elasticMapping(elasticMapping), _incidentEdges(elasticMapping.structureAnalysis().atomCount(), nullptr),
		_planarDefects(new PlanarDefects()) {}

	/// Returns the elastic mapping of the crystal.
	const ElasticMapping& elasticMapping() const { return _elasticMapping; }

	/// Returns the structure analysis object.
	const StructureAnalysis& structureAnalysis() const { return _elasticMapping.structureAnalysis(); }

	/// Returns the underlying tessellation.
	const DelaunayTessellation& tessellation() const { return _elasticMapping.tessellation(); }

	/// Returns the cluster graph.
	ClusterGraph& clusterGraph() { return _elasticMapping.clusterGraph(); }

	/// Extracts the planar defects.
	bool extractPlanarDefects(int crystalStructure, FutureInterfaceBase& progress);

	/// Returns the extracted planar defects.
	PlanarDefects* planarDefects() { return _planarDefects.data(); }

private:

	/// The elastic mapping of the crystal.
	ElasticMapping& _elasticMapping;

	/// Stores the incident edge for each vertex of the tessellation.
	std::vector<ElasticMapping::TessellationEdge*> _incidentEdges;

	/// The extracted planar defects.
	QExplicitlySharedDataPointer<PlanarDefects> _planarDefects;
};

}	// End of namespace
}	// End of namespace
}	// End of namespace


