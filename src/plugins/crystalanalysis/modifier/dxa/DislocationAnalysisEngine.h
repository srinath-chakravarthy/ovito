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
#include <plugins/particles/modifier/analysis/StructureIdentificationModifier.h>
#include <plugins/crystalanalysis/util/DelaunayTessellation.h>
#include "StructureAnalysis.h"
#include "ElasticMapping.h"
#include "InterfaceMesh.h"
#include "DislocationTracer.h"
#if 0
#include "PlanarDefectIdentification.h"
#endif

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/*
 * Computation engine of the DislocationAnalysisModifier, which performs the actual dislocation analysis.
 */
class DislocationAnalysisEngine : public StructureIdentificationModifier::StructureIdentificationEngine
{
public:

	/// Constructor.
	DislocationAnalysisEngine(const TimeInterval& validityInterval,
			ParticleProperty* positions, const SimulationCell& simCell,
			int inputCrystalStructure, int maxTrialCircuitSize, int maxCircuitElongation,
			bool reconstructEdgeVectors, ParticleProperty* particleSelection,
			ParticleProperty* crystalClusters,
			std::vector<Matrix3>&& preferredCrystalOrientations,
			bool onlyPerfectDislocations);

	/// Computes the modifier's results and stores them in this object for later retrieval.
	virtual void perform() override;

	/// Returns the computed defect mesh.
	HalfEdgeMesh<>* defectMesh() { return _defectMesh.data(); }

	/// Returns the computed interface mesh.
	const InterfaceMesh& interfaceMesh() const { return _interfaceMesh; }

	/// Indicates whether the entire simulation cell is part of the 'good' crystal region.
	bool isGoodEverywhere() const { return _interfaceMesh.isCompletelyGood(); }

	/// Indicates whether the entire simulation cell is part of the 'bad' crystal region.
	bool isBadEverywhere() const { return _interfaceMesh.isCompletelyBad(); }

	/// Returns the array of atom cluster IDs.
	ParticleProperty* atomClusters() const { return _structureAnalysis.atomClusters(); }

	/// Gives access to the elastic mapping computation engine.
	ElasticMapping& elasticMapping() { return _elasticMapping; }

	/// Returns the created cluster graph.
	ClusterGraph* clusterGraph() { return &_structureAnalysis.clusterGraph(); }

	/// Returns the extracted dislocation network.
	DislocationNetwork* dislocationNetwork() { return &_dislocationTracer.network(); }

#if 0
	/// Returns the planar defect identification engine.
	PlanarDefectIdentification& planarDefectIdentification() { return _planarDefectIdentification; }
#endif

	/// Returns the input particle property that stores the cluster assignment of atoms.
	ParticleProperty* crystalClusters() const { return _crystalClusters.data(); }

private:

	int _inputCrystalStructure;
	bool _reconstructEdgeVectors;
	bool _onlyPerfectDislocations;
	QExplicitlySharedDataPointer<HalfEdgeMesh<>> _defectMesh;
	QExplicitlySharedDataPointer<ParticleProperty> _crystalClusters;
	StructureAnalysis _structureAnalysis;
	DelaunayTessellation _tessellation;
	ElasticMapping _elasticMapping;
	InterfaceMesh _interfaceMesh;
	DislocationTracer _dislocationTracer;
#if 0
	PlanarDefectIdentification _planarDefectIdentification;
#endif
};

}	// End of namespace
}	// End of namespace
}	// End of namespace


