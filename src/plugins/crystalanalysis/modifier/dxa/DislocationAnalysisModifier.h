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
#include <plugins/particles/objects/SurfaceMeshDisplay.h>
#include <plugins/crystalanalysis/objects/dislocations/DislocationDisplay.h>
#include <plugins/crystalanalysis/objects/patterns/PatternCatalog.h>
#include <plugins/crystalanalysis/data/DislocationNetwork.h>
#include <plugins/crystalanalysis/data/ClusterGraph.h>
#include <plugins/crystalanalysis/data/PlanarDefects.h>
#include <plugins/crystalanalysis/modifier/dxa/StructureAnalysis.h>
#include <plugins/crystalanalysis/modifier/SmoothDislocationsModifier.h>
#include <plugins/crystalanalysis/modifier/SmoothSurfaceModifier.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/*
 * Extracts dislocation lines from a crystal.
 */
class OVITO_CRYSTALANALYSIS_EXPORT DislocationAnalysisModifier : public StructureIdentificationModifier
{
public:

	/// Constructor.
	Q_INVOKABLE DislocationAnalysisModifier(DataSet* dataset);

	/// Returns the number of segments found per dislocation type.
	const std::map<BurgersVectorFamily*,int>& segmentCounts() const { return _segmentCounts; }

	/// Returns the total length of segments found per dislocation type.
	const std::map<BurgersVectorFamily*,FloatType>& dislocationLengths() const { return _dislocationLengths; }

	/// Resets the modifier's result cache.
	virtual void invalidateCachedResults() override;

	/// Returns whether dislocation line smoothing is enabled.
	bool lineSmoothingEnabled() const { return smoothDislocationsModifier()->smoothingEnabled(); }

	/// Enables/disables dislocation line smoothing.
	void setLineSmoothingEnabled(bool enable) { smoothDislocationsModifier()->setSmoothingEnabled(enable); }

	/// Returns the dislocation line smoothing strength.
	int lineSmoothingLevel() const { return smoothDislocationsModifier()->smoothingLevel(); }

	/// Sets the dislocation line smoothing strength.
	void setLineSmoothingLevel(int level) { smoothDislocationsModifier()->setSmoothingLevel(level); }

	/// Returns whether coarsening of dislocation line points is enabled.
	bool lineCoarseningEnabled() const { return smoothDislocationsModifier()->coarseningEnabled(); }

	/// Enables/disables coarsening of dislocation line points.
	void setLineCoarseningEnabled(bool enable) { smoothDislocationsModifier()->setCoarseningEnabled(enable); }

	/// Returns the target distance between successive line points after coarsening.
	FloatType linePointInterval() const { return smoothDislocationsModifier()->linePointInterval(); }

	/// Sets the target distance between successive line points after coarsening.
	void setLinePointInterval(FloatType d) { smoothDislocationsModifier()->setLinePointInterval(d); }

	/// Returns the surface smoothing strength for the defect mesh.
	int defectMeshSmoothingLevel() const { return smoothSurfaceModifier()->smoothingLevel(); }

	/// Sets the surface smoothing strength for the defect mesh.
	void setDefectMeshSmoothingLevel(int level) { smoothSurfaceModifier()->setSmoothingLevel(level); }

protected:

	/// Handles reference events sent by reference targets of this object.
	virtual bool referenceEvent(RefTarget* source, ReferenceEvent* event) override;

	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// Creates a computation engine that will compute the modifier's results.
	virtual std::shared_ptr<ComputeEngine> createEngine(TimePoint time, TimeInterval validityInterval) override;

	/// Unpacks the results of the computation engine and stores them in the modifier.
	virtual void transferComputationResults(ComputeEngine* engine) override;

	/// Lets the modifier insert the cached computation results into the modification pipeline.
	virtual PipelineStatus applyComputationResults(TimePoint time, TimeInterval& validityInterval) override;

private:

	/// The type of crystal to be analyzed.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(StructureAnalysis::LatticeStructureType, inputCrystalStructure, setInputCrystalStructure);

	/// The maximum length of trial circuits.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, maxTrialCircuitSize, setMaxTrialCircuitSize);

	/// The maximum elongation of Burgers circuits while they are being advanced.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, circuitStretchability, setCircuitStretchability);

	/// Controls the output of the interface mesh.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, outputInterfaceMesh, setOutputInterfaceMesh);

	/// Enables the reconstruction of missing tessellation edge lattice vectors.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, reconstructEdgeVectors, setReconstructEdgeVectors);

	/// Restricts the identification to perfect lattice dislocations.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, onlyPerfectDislocations, setOnlyPerfectDislocations);

	/// The catalog of structure patterns.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(PatternCatalog, patternCatalog, setPatternCatalog);

	/// The display object for rendering the defect mesh.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(SurfaceMeshDisplay, defectMeshDisplay, setDefectMeshDisplay);

	/// The display object for rendering the interface mesh.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(SurfaceMeshDisplay, interfaceMeshDisplay, setInterfaceMeshDisplay);

	/// The display object for rendering the dislocations.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(DislocationDisplay, dislocationDisplay, setDislocationDisplay);

	/// The internal modifier that smoothes the extracted dislocation lines.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(SmoothDislocationsModifier, smoothDislocationsModifier, setSmoothDislocationsModifier);

	/// The internal modifier that smoothes the defect surface mesh.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(SmoothSurfaceModifier, smoothSurfaceModifier, setSmoothSurfaceModifier);

	/// This stores the cached defect mesh produced by the modifier.
	QExplicitlySharedDataPointer<HalfEdgeMesh<>> _defectMesh;

	/// This stores the cached defect interface produced by the modifier.
	QExplicitlySharedDataPointer<HalfEdgeMesh<>> _interfaceMesh;

	/// This stores the cached atom-to-cluster assignments computed by the modifier.
	QExplicitlySharedDataPointer<ParticleProperty> _atomClusters;

	/// This stores the cached cluster graph computed by the modifier.
	QExplicitlySharedDataPointer<ClusterGraph> _clusterGraph;

	/// This stores the cached dislocations computed by the modifier.
	QExplicitlySharedDataPointer<DislocationNetwork> _dislocationNetwork;

	/// This stores the cached planar defects extracted by the modifier.
	QExplicitlySharedDataPointer<PlanarDefects> _planarDefects;

	/// The cached simulation cell from the last analysis run.
	SimulationCell _simCell;

	/// Indicates that the entire simulation cell is part of the 'good' crystal region.
	bool _isGoodEverywhere;

	/// Indicates that the entire simulation cell is part of the 'bad' crystal region.
	bool _isBadEverywhere;

	/// List of edges, which don't have a lattice vector.
	QExplicitlySharedDataPointer<BondsStorage> _unassignedEdges;

	/// The number of segments found per dislocation type.
	std::map<BurgersVectorFamily*,int> _segmentCounts;

	/// The total length of segments found per dislocation type.
	std::map<BurgersVectorFamily*,FloatType> _dislocationLengths;

	/// The structure pattern each dislocation type belongs to.
	std::map<BurgersVectorFamily*,StructurePattern*> _dislocationStructurePatterns;

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Dislocation analysis (DXA)");
	Q_CLASSINFO("ModifierCategory", "Analysis");
};

}	// End of namespace
}	// End of namespace
}	// End of namespace


