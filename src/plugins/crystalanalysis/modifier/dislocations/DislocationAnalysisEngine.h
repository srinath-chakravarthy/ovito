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

#ifndef __OVITO_DISLOCATION_ANALYSIS_ENGINE_H
#define __OVITO_DISLOCATION_ANALYSIS_ENGINE_H

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/particles/modifier/AsynchronousParticleModifier.h>
#include <plugins/particles/modifier/analysis/cna/CommonNeighborAnalysisModifier.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/*
 * Computation engine of the DislocationAnalysisModifier, which performs the actual dislocation analysis.
 */
class DislocationAnalysisEngine : public AsynchronousParticleModifier::ComputeEngine
{
public:

	/// The coordination structure types.
	enum CoordinationStructureType {
		COORD_OTHER = 0,		//< Unidentified structure
		COORD_FCC,				//< Face-centered cubic
		COORD_HCP,				//< Hexagonal close-packed
		COORD_BCC,				//< Body-centered cubic

		NUM_COORD_TYPES 		//< This just counts the number of defined coordination types.
	};

	/// The lattice structure types.
	enum LatticeStructureType {
		LATTICE_OTHER = 0,			//< Unidentified structure
		LATTICE_FCC,				//< Face-centered cubic
		LATTICE_HCP,				//< Hexagonal close-packed
		LATTICE_BCC,				//< Body-centered cubic

		NUM_LATTICE_TYPES 			//< This just counts the number of defined coordination types.
	};

	/// The maximum number of neighbor atoms taken into account for the common neighbor analysis.
#ifndef Q_CC_MSVC
	static constexpr int MAX_NEIGHBORS = 14;
#else
	enum { MAX_NEIGHBORS = 14 };
#endif

	struct CoordinationStructure {
		int numNeighbors;
		std::vector<Vector3> latticeVectors;
		CommonNeighborAnalysisModifier::NeighborBondArray neighborArray;
		int cnaSignatures[MAX_NEIGHBORS];
		int commonNeighbors[MAX_NEIGHBORS][3];
	};

	struct LatticeStructure {
		std::vector<Vector3> latticeVectors;
	};

public:

	/// Constructor.
	DislocationAnalysisEngine(const TimeInterval& validityInterval, ParticleProperty* positions, const SimulationCell& simCell);

	/// Computes the modifier's results and stores them in this object for later retrieval.
	virtual void perform() override;

	/// Returns the generated defect mesh.
	HalfEdgeMesh* defectMesh() { return _defectMesh.data(); }

	/// Returns the input particle positions.
	ParticleProperty* positions() const { return _positions.data(); }

	/// Returns the input simulation cell.
	const SimulationCell& cell() const { return _simCell; }

	/// Indicates whether the entire simulation cell is part of the 'bad' crystal region.
	bool isDefectRegionEverywhere() const { return _isDefectRegionEverywhere; }

protected:

	/// Determines the coordination structure of a particle.
	void determineLocalStructure(NearestNeighborFinder& neighList, size_t particleIndex);

	/// Combines adjacent atoms to clusters.
	void buildClusters();

	/// Prepares the list of coordination structures.
	static void initializeCoordinationStructures();

private:

	QExplicitlySharedDataPointer<ParticleProperty> _positions;
	QExplicitlySharedDataPointer<HalfEdgeMesh> _defectMesh;
	QExplicitlySharedDataPointer<ParticleProperty> _structureTypes;
	QExplicitlySharedDataPointer<ParticleProperty> _neighborLists;
	QExplicitlySharedDataPointer<ParticleProperty> _atomClusters;
	QExplicitlySharedDataPointer<ParticleProperty> _atomLatticeVectors;
	SimulationCell _simCell;
	bool _isDefectRegionEverywhere;
	static CoordinationStructure _coordinationStructures[NUM_COORD_TYPES];
	static LatticeStructure _latticeStructures[NUM_LATTICE_TYPES];
};

}	// End of namespace
}	// End of namespace
}	// End of namespace

#endif // __OVITO_DISLOCATION_ANALYSIS_ENGINE_H
