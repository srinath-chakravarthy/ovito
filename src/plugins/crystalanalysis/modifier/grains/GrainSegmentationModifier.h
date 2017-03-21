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
#include <plugins/crystalanalysis/objects/patterns/PatternCatalog.h>
#include <plugins/crystalanalysis/data/ClusterGraph.h>
#include <plugins/crystalanalysis/modifier/dxa/StructureAnalysis.h>
#include <plugins/crystalanalysis/objects/partition_mesh/PartitionMesh.h>
#include <plugins/crystalanalysis/objects/partition_mesh/PartitionMeshDisplay.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/*
 * Identifies the grains in a polycrystal.
 */
class OVITO_CRYSTALANALYSIS_EXPORT GrainSegmentationModifier : public StructureIdentificationModifier
{
public:

	/// Constructor.
	Q_INVOKABLE GrainSegmentationModifier(DataSet* dataset);

	/// Resets the modifier's result cache.
	virtual void invalidateCachedResults() override;

protected:

	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// Handles reference events sent by reference targets of this object.
	virtual bool referenceEvent(RefTarget* source, ReferenceEvent* event) override;

	/// Creates a computation engine that will compute the modifier's results.
	virtual std::shared_ptr<ComputeEngine> createEngine(TimePoint time, TimeInterval validityInterval) override;

	/// Unpacks the results of the computation engine and stores them in the modifier.
	virtual void transferComputationResults(ComputeEngine* engine) override;

	/// Lets the modifier insert the cached computation results into the modification pipeline.
	virtual PipelineStatus applyComputationResults(TimePoint time, TimeInterval& validityInterval) override;

private:

	/// The type of crystal to be analyzed.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(StructureAnalysis::LatticeStructureType, inputCrystalStructure, setInputCrystalStructure);

	/// The minimum misorientation angle between adjacent grains.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, misorientationThreshold, setMisorientationThreshold);

	/// Controls the amount of noise allowed inside a grain.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, fluctuationTolerance, setFluctuationTolerance);

	/// The minimum number of crystalline atoms per grain.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, minGrainAtomCount, setMinGrainAtomCount);

	/// Enables the generation of the partition mesh.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, outputPartitionMesh, setOutputPartitionMesh);

	/// Controls the radius of the probe sphere used when constructing the free surfaces.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, probeSphereRadius, setProbeSphereRadius);

	/// Controls the amount of smoothing applied to the mesh.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, smoothingLevel, setSmoothingLevel);

	/// Controls whether only selected particles should be taken into account.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, onlySelectedParticles, setOnlySelectedParticles);

	/// The display object for rendering the mesh.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(PartitionMeshDisplay, meshDisplay, setMeshDisplay);

	/// This stores the cached mesh produced by the modifier.
	QExplicitlySharedDataPointer<PartitionMeshData> _partitionMesh;

	/// The ID of the grain that entirely fills the simulation cell (if any).
	int _spaceFillingRegion;

	/// The catalog of structure patterns.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(PatternCatalog, patternCatalog, setPatternCatalog);

	/// This stores the cached atom-to-cluster assignments computed by the modifier.
	QExplicitlySharedDataPointer<ParticleProperty> _atomClusters;

	/// This stores the cached cluster graph computed by the modifier.
	QExplicitlySharedDataPointer<ClusterGraph> _clusterGraph;

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Grain segmentation");
	Q_CLASSINFO("ModifierCategory", "Analysis");
};

}	// End of namespace
}	// End of namespace
}	// End of namespace


