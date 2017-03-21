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

#pragma once


#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/particles/modifier/analysis/StructureIdentificationModifier.h>
#include <plugins/crystalanalysis/modifier/dxa/StructureAnalysis.h>
#include <plugins/crystalanalysis/objects/partition_mesh/PartitionMesh.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/*
 * Computation engine of the GrainSegmentationModifier, which performs decomposes a crystalline structure into individual grains.
 */
class GrainSegmentationEngine : public StructureIdentificationModifier::StructureIdentificationEngine
{
public:

	/// Constructor.
	GrainSegmentationEngine(const TimeInterval& validityInterval,
			ParticleProperty* positions, const SimulationCell& simCell,
			ParticleProperty* selection,
			int inputCrystalStructure, FloatType misorientationThreshold,
			FloatType fluctuationTolerance, int minGrainAtomCount,
			FloatType probeSphereRadius, int meshSmoothingLevel);

	/// Computes the modifier's results and stores them in this object for later retrieval.
	virtual void perform() override;

	/// Returns the array of atom cluster IDs.
	ParticleProperty* atomClusters() const { return _structureAnalysis.atomClusters(); }

	/// Returns the created cluster graph.
	ClusterGraph* outputClusterGraph() { return _outputClusterGraph.data(); }

	/// Returns the property storage that contains the computed per-particle deformation gradient tensors.
	ParticleProperty* deformationGradients() const { return _deformationGradients.data(); }

	/// Returns the generated mesh.
	PartitionMeshData* mesh() { return _mesh.data(); }

	/// Return the ID of the grain that fills the entire simulation (if any).
	int spaceFillingGrain() const { return _spaceFillingGrain; }

private:

	/** This internal class stores working data for a grain of the polycrystal. */
	struct Grain {

		/// Number of atoms that belong to the grain.
		int atomCount = 1;

		/// Number of atoms that belong to the grain and for which a local orientation tensor was computed.
		int latticeAtomCount = 0;

		/// The (average) lattice orientation tensor of the grain.
		Matrix3 orientation;

		/// Cluster that is used to define the grain's lattice orientation.
		Cluster* cluster = nullptr;

		/// Unique ID assigned to the grain.
		int id;

		/// This field is used by the disjoint-set forest algorithm using union-by-rank and path compression.
		size_t rank = 0;

		/// Pointer to the parent grain. This field is used by the disjoint-set algorithm.
		Grain* parent;

		/// Returns true if this is a root grain in the disjoint set structure.
		bool isRoot() const { return parent == this; }

		/// Default constructor.
		Grain() {
			parent = this;
		}

		/// Merges another grain into this one.
		void join(Grain& grainB, const Matrix3& alignmentTM = Matrix3::Zero()) {
			grainB.parent = this;
			if(grainB.cluster != nullptr) {
				OVITO_ASSERT(cluster != nullptr);
				FloatType weightA = FloatType(latticeAtomCount) / (latticeAtomCount + grainB.latticeAtomCount);
				FloatType weightB = FloatType(1) - weightA;
				orientation = weightA * orientation + weightB * (grainB.orientation * alignmentTM);
			}
			atomCount += grainB.atomCount;
			latticeAtomCount += grainB.latticeAtomCount;
		}
	};

	/**
	 * An edge connecting two adjacent grains in the graph of grains.
	 */
	struct GrainGraphEdge
	{
		/// Identifier of grain 1.
		int a;

		/// Identifier of grain 2.
		int b;

		/// Misorientation angle between the two grains.
		FloatType misorientation;

		/// This comparison operator is used to sort edges.
		bool operator<(const GrainGraphEdge& other) const { return misorientation < other.misorientation; }
	};


	/// Calculates the misorientation angle between two lattice orientations.
	FloatType calculateMisorientation(const Grain& grainA, const Grain& grainB, Matrix3* alignmentTM = nullptr);

	/// Computes the angle of rotation from a rotation matrix.
	static FloatType angleFromMatrix(const Matrix3& tm);

	/// Tests if two grain should be merged and merges them if deemed necessary.
	bool mergeTest(Grain& grainA, Grain& grainB, bool allowForFluctuations);

	/// Assigns contiguous IDs to all parent grains.
	size_t assignIdsToGrains();

	/// Returns the parent grain of another grain.
	Grain& parentGrain(const Grain& grain) const {
		Grain* parent = grain.parent;
		while(!parent->isRoot()) parent = parent->parent;
		return *parent;
	}

	/// Returns the parent grain of an atom.
	Grain& parentGrain(int atomIndex) {
		Grain& parent = parentGrain(_grains[atomIndex]);
		// Perform path compression:
		_grains[atomIndex].parent = &parent;
		return parent;
	}

	/// Builds the triangle mesh for the grain boundaries.
	bool buildPartitionMesh();

private:

	int _inputCrystalStructure;
	StructureAnalysis _structureAnalysis;
	QExplicitlySharedDataPointer<ParticleProperty> _deformationGradients;

	/// The minimum misorientation angle between adjacent grains.
	FloatType _misorientationThreshold;

	/// Controls the amount of noise allowed inside a grain.
	FloatType _fluctuationTolerance;

	/// The minimum number of crystalline atoms per grain.
	int _minGrainAtomCount;

	/// The probe sphere radius used to construct the free surfaces of the solid.
	FloatType _probeSphereRadius;

	/// The strength of smoothing applied to the constructed partition mesh.
	int _meshSmoothingLevel;

	/// The working list of grains (contains one element per input atom).
	std::vector<Grain> _grains;

	/// The final number of grains.
	int _grainCount;

	/// The grain boundary mesh generated by the engine.
	QExplicitlySharedDataPointer<PartitionMeshData> _mesh;

	/// Stores the ID of the grain that fills the entire simulation (if any).
	int _spaceFillingGrain;

	/// The cluster graph generated by this engine, with one cluster per grain.
	QExplicitlySharedDataPointer<ClusterGraph> _outputClusterGraph;
};

}	// End of namespace
}	// End of namespace
}	// End of namespace


