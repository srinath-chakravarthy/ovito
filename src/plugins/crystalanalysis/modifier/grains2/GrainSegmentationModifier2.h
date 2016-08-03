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

#ifndef __OVITO_GRAIN_SEGMENTATION_MODIFIER2_H
#define __OVITO_GRAIN_SEGMENTATION_MODIFIER2_H

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/particles/modifier/analysis/StructureIdentificationModifier.h>
#include <plugins/crystalanalysis/objects/patterns/PatternCatalog.h>
#include <plugins/crystalanalysis/data/ClusterGraph.h>
#include <plugins/crystalanalysis/objects/partition_mesh/PartitionMesh.h>
#include <plugins/crystalanalysis/objects/partition_mesh/PartitionMeshDisplay.h>
#include "GrainSegmentationEngine2.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/*
 * Identifies the grains in a polycrystal.
 */
class OVITO_CRYSTALANALYSIS_EXPORT GrainSegmentationModifier2 : public StructureIdentificationModifier
{
public:

	/// Constructor.
	Q_INVOKABLE GrainSegmentationModifier2(DataSet* dataset);

	/// Return the catalog of structure patterns.
	PatternCatalog* patternCatalog() const { return _patternCatalog; }

	/// Returns the type of crystal to be analyzed.
	GrainSegmentationEngine2::StructureType inputCrystalStructure() const { return static_cast<GrainSegmentationEngine2::StructureType>(_inputCrystalStructure.value()); }

	/// Sets the type of crystal to be analyzed.
	void setInputCrystalStructure(GrainSegmentationEngine2::StructureType structureType) { _inputCrystalStructure = structureType; }

	/// \brief Returns the RMSD cutoff.
	FloatType rmsdCutoff() const { return _rmsdCutoff; }

	/// \brief Sets the RMSD cutoff.
	void setRmsdCutoff(FloatType cutoff) { _rmsdCutoff = cutoff; }

	/// Returns the computed histogram of RMSD values.
	const QVector<int>& rmsdHistogramData() const { return _rmsdHistogramData; }

	/// Returns the bin size of the RMSD histogram.
	FloatType rmsdHistogramBinSize() const { return _rmsdHistogramBinSize; }

	/// Returns whether local orientations are output by the modifier.
	bool outputLocalOrientations() const { return _outputLocalOrientations; }

	/// Sets whether local orientations are output by the modifier.
	void setOutputLocalOrientations(bool enable) { _outputLocalOrientations = enable; }

	/// Returns the minimum misorientation angle between adjacent grains.
	FloatType misorientationThreshold() const { return _misorientationThreshold; }

	/// Sets the minimum misorientation angle between adjacent grains.
	void setMisorientationThreshold(FloatType threshold) { _misorientationThreshold = threshold; }

	/// Returns the minimum number of crystalline atoms per grain.
	int minGrainAtomCount() const { return _minGrainAtomCount; }

	/// Sets the minimum number of crystalline atoms per grain.
	void setMinGrainAtomCount(int minAtoms) { _minGrainAtomCount = minAtoms; }

	/// \brief Returns the radius parameter used during construction of the free surface.
	FloatType probeSphereRadius() const { return _probeSphereRadius; }

	/// \brief Sets the radius parameter used during construction of the free surface.
	void setProbeSphereRadius(FloatType radius) { _probeSphereRadius = radius; }

	/// \brief Returns the level of smoothing applied to the constructed partition mesh.
	int smoothingLevel() const { return _smoothingLevel; }

	/// \brief Sets the level of smoothing applied to the constructed partition mesh.
	void setSmoothingLevel(int level) { _smoothingLevel = level; }

	/// Returns whether only selected particles are taken into account.
	bool onlySelectedParticles() const { return _onlySelectedParticles; }

	/// Sets whether only selected particles should be taken into account.
	void setOnlySelectedParticles(bool onlySelected) { _onlySelectedParticles = onlySelected; }

	/// Returns whether the generation of the partition mesh is enabled.
	bool outputPartitionMesh() const { return _outputPartitionMesh; }

	/// Enables the generation of the partition mesh.
	void setOutputPartitionMesh(bool enable) { _outputPartitionMesh = enable; }

	/// \brief Returns the display object that is responsible for rendering the grain boundary mesh.
	PartitionMeshDisplay* meshDisplay() const { return _meshDisplay; }

	/// Returns the display object that is responsible for rendering the bonds generated by the modifier.
	BondsDisplay* bondsDisplay() const { return _bondsDisplay; }

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
	PropertyField<int> _inputCrystalStructure;

	/// The RMSD cutoff for the PTM.
	PropertyField<FloatType> _rmsdCutoff;

	/// The minimum misorientation angle between adjacent grains.
	PropertyField<FloatType> _misorientationThreshold;

	/// The minimum number of crystalline atoms per grain.
	PropertyField<int> _minGrainAtomCount;

	/// Enables the generation of the partition mesh.
	PropertyField<bool> _outputPartitionMesh;

	/// Controls the radius of the probe sphere used when constructing the free surfaces.
	PropertyField<FloatType> _probeSphereRadius;

	/// Controls the amount of smoothing applied to the mesh.
	PropertyField<int> _smoothingLevel;

	/// Controls whether only selected particles should be taken into account.
	PropertyField<bool> _onlySelectedParticles;

	/// Controls the output of local orientations.
	PropertyField<bool> _outputLocalOrientations;

	/// The number of iterations of the smoothing procedure.
	PropertyField<int> _numOrientationSmoothingIterations;

	/// The weighting parameter used by the smoothing algorithm.
	PropertyField<FloatType> _orientationSmoothingWeight;

	/// The display object for rendering the mesh.
	ReferenceField<PartitionMeshDisplay> _meshDisplay;

	/// The display object for rendering the bonds generated by the modifier.
	ReferenceField<BondsDisplay> _bondsDisplay;

	/// This stores the cached mesh produced by the modifier.
	QExplicitlySharedDataPointer<PartitionMeshData> _partitionMesh;

	/// The ID of the grain that entirely fills the simulation cell (if any).
	int _spaceFillingRegion;

	/// The catalog of structure patterns.
	ReferenceField<PatternCatalog> _patternCatalog;

	/// This stores the cached atom-to-cluster assignments computed by the modifier.
	QExplicitlySharedDataPointer<ParticleProperty> _atomClusters;

	/// This stores the cached cluster graph computed by the modifier.
	QExplicitlySharedDataPointer<ClusterGraph> _clusterGraph;

	/// The computed per-particle orientations.
	QExplicitlySharedDataPointer<ParticleProperty> _localOrientations;

	/// The computed histogram of RMSD values.
	QVector<int> _rmsdHistogramData;

	/// The bin size of the RMSD histogram;
	FloatType _rmsdHistogramBinSize;

	/// The bonds generated between neighboring lattice atoms.
	QExplicitlySharedDataPointer<BondsStorage> _latticeNeighborBonds;

	/// The computed disorientation angles between neighboring lattice atoms.
	QExplicitlySharedDataPointer<BondProperty> _neighborDisorientationAngles;

	/// The distance transform results.
	QExplicitlySharedDataPointer<ParticleProperty> _defectDistances;

	/// Markers for the local distance transform maxima.
	QExplicitlySharedDataPointer<ParticleProperty> _defectDistanceMaxima;

	QExplicitlySharedDataPointer<ParticleProperty> _vertexColors;
	QExplicitlySharedDataPointer<BondProperty> _edgeCapacity;
	QExplicitlySharedDataPointer<BondProperty> _residualEdgeCapacity;

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Grain segmentation 2");
	Q_CLASSINFO("ModifierCategory", "Analysis");

	DECLARE_PROPERTY_FIELD(_inputCrystalStructure);
	DECLARE_PROPERTY_FIELD(_rmsdCutoff);
	DECLARE_PROPERTY_FIELD(_misorientationThreshold);
	DECLARE_PROPERTY_FIELD(_minGrainAtomCount);
	DECLARE_PROPERTY_FIELD(_outputPartitionMesh);
	DECLARE_PROPERTY_FIELD(_probeSphereRadius);
	DECLARE_PROPERTY_FIELD(_smoothingLevel);
	DECLARE_PROPERTY_FIELD(_onlySelectedParticles);
	DECLARE_PROPERTY_FIELD(_outputLocalOrientations);
	DECLARE_PROPERTY_FIELD(_numOrientationSmoothingIterations);
	DECLARE_PROPERTY_FIELD(_orientationSmoothingWeight);
	DECLARE_REFERENCE_FIELD(_patternCatalog);
	DECLARE_REFERENCE_FIELD(_meshDisplay);
	DECLARE_REFERENCE_FIELD(_bondsDisplay);
};

}	// End of namespace
}	// End of namespace
}	// End of namespace

#endif // __OVITO_GRAIN_SEGMENTATION_MODIFIER_H
