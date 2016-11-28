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

#ifndef __OVITO_GRAIN_SEGMENTATION_ENGINE_H
#define __OVITO_GRAIN_SEGMENTATION_ENGINE_H

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/particles/modifier/analysis/StructureIdentificationModifier.h>
#include <plugins/particles/data/ParticleProperty.h>
#include <plugins/particles/data/BondProperty.h>
#include <plugins/particles/data/BondsStorage.h>
#include <plugins/crystalanalysis/objects/partition_mesh/PartitionMesh.h>

#include <core/utilities/io/CompressedTextWriter.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/*
 * Computation engine of the GrainSegmentationModifier, which performs decomposes a crystalline structure into individual grains.
 */
class GrainSegmentationEngine : public StructureIdentificationModifier::StructureIdentificationEngine
{
public:

#ifndef Q_CC_MSVC
	/// The maximum number of neighbor atoms taken into account for the PTM analysis.
	static constexpr int MAX_NEIGHBORS = 18;
#else
	enum { MAX_NEIGHBORS = 18 };
#endif

	/// The structure types recognized by the PTM library.
	enum StructureType {
		OTHER = 0,				//< Unidentified structure
		FCC,					//< Face-centered cubic
		HCP,					//< Hexagonal close-packed
		BCC,					//< Body-centered cubic
		ICO,					//< Icosahedral structure
		SC,						//< Simple cubic structure

		NUM_STRUCTURE_TYPES 	//< This just counts the number of defined structure types.
	};
	Q_ENUMS(StructureType);

	/// Constructor.
	GrainSegmentationEngine(const TimeInterval& validityInterval,
			ParticleProperty* positions, const SimulationCell& simCell,
			const QVector<bool>& typesToIdentify, ParticleProperty* selection,
			int inputCrystalStructure, FloatType rmsdCutoff, int numOrientationSmoothingIterations,
			FloatType orientationSmoothingWeight, FloatType misorientationThreshold,
			int minGrainAtomCount, FloatType probeSphereRadius, int meshSmoothingLevel);

	/// Computes the modifier's results and stores them in this object for later retrieval.
	virtual void perform() override;

	/// Returns the array of atom cluster IDs.
	ParticleProperty* atomClusters() const { return _atomClusters.data(); }

	
	/// Returns the array of atom cluster IDs.
	ParticleProperty* symmetry() const { return _symmetryorient.data(); }

	/// Returns the created cluster graph.
	ClusterGraph* outputClusterGraph() { return _outputClusterGraph.data(); }

	/// Returns the generated mesh.
	PartitionMeshData* mesh() { return _mesh.data(); }

	/// Return the ID of the grain that fills the entire simulation (if any).
	int spaceFillingGrain() const { return _spaceFillingGrain; }

	/// Returns the computed RMSD histogram data.
	const QVector<int>& rmsdHistogramData() const { return _rmsdHistogramData; }

	/// Returns the bin size of the RMSD histogram.
	FloatType rmsdHistogramBinSize() const { return _rmsdHistogramBinSize; }

	/// Returns the computed per-particle lattice orientations.
	ParticleProperty* localOrientations() const { return _orientations.data(); }

	/// Returns the bonds generated between neighboring lattice atoms.
	BondsStorage* latticeNeighborBonds() const { return _latticeNeighborBonds.data(); }

	/// Returns the computed disorientation angles between neighboring lattice atoms.
	BondProperty* neighborDisorientationAngles() const { return _neighborDisorientationAngles.data(); }

	/// Returns the computed distance transform values.
	ParticleProperty* defectDistances() const { return _defectDistances.data(); }

	/// Returns the property storing the distance transform basin each atom has been assigned to.
	ParticleProperty* defectDistanceBasins() const { return _defectDistanceBasins.data(); }	

private:

	/// Builds the triangle mesh for the grain boundaries.
	bool buildPartitionMesh();
	// Extract submesh from partitionmesh
	void extractMesh();

private:

	/// The structural type of the input crystal to be segmented.
	int _inputCrystalStructure;

	/// The output particle property.
	QExplicitlySharedDataPointer<ParticleProperty> _atomClusters;

	FloatType _rmsdCutoff;
	QVector<int> _rmsdHistogramData;
	FloatType _rmsdHistogramBinSize;
	QExplicitlySharedDataPointer<ParticleProperty> _rmsd;

	/// The computed per-particle lattice orientations.
	QExplicitlySharedDataPointer<ParticleProperty> _orientations;

	
	/// The computed per-particle lattice orientations.
	QExplicitlySharedDataPointer<ParticleProperty> _symmetryorient;

	/// The number of iterations of the smoothing procedure.
	int _numOrientationSmoothingIterations;

	/// The weighting parameter used by the smoothing algorithm.
	FloatType _orientationSmoothingWeight;

	/// The minimum misorientation angle between adjacent grains.
	FloatType _misorientationThreshold;

	/// The minimum number of crystalline atoms per grain.
	int _minGrainAtomCount;

	/// The probe sphere radius used to construct the free surfaces of the solid.
	FloatType _probeSphereRadius;

	/// The strength of smoothing applied to the constructed partition mesh.
	int _meshSmoothingLevel;

	/// The grain boundary mesh generated by the engine.
	QExplicitlySharedDataPointer<PartitionMeshData> _mesh;

	/// Stores the ID of the grain that fills the entire simulation (if any).
	int _spaceFillingGrain;

	/// The cluster graph generated by this compute engine, with one cluster per grain.
	QExplicitlySharedDataPointer<ClusterGraph> _outputClusterGraph;

	/// Stores the list of neighbors of each lattice atom.
	QExplicitlySharedDataPointer<ParticleProperty> _neighborLists;

	/// The bonds generated between neighboring lattice atoms.
	QExplicitlySharedDataPointer<BondsStorage> _latticeNeighborBonds;

	/// The computed disorientation angles between neighboring lattice atoms.
	QExplicitlySharedDataPointer<BondProperty> _neighborDisorientationAngles;

	/// The computed distance transform.
	QExplicitlySharedDataPointer<ParticleProperty> _defectDistances;

	/// The basin to which each atom has been assigned.
	QExplicitlySharedDataPointer<ParticleProperty> _defectDistanceBasins;
};

}	// End of namespace
}	// End of namespace
}	// End of namespace

#endif // __OVITO_GRAIN_SEGMENTATION_ENGINE_H
