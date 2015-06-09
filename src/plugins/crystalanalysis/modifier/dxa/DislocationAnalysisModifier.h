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

#ifndef __OVITO_DISLOCATION_ANALYSIS_MODIFIER_H
#define __OVITO_DISLOCATION_ANALYSIS_MODIFIER_H

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/particles/modifier/analysis/StructureIdentificationModifier.h>
#include <plugins/particles/objects/SurfaceMeshDisplay.h>
#include <plugins/crystalanalysis/objects/dislocations/DislocationDisplay.h>
#include <plugins/crystalanalysis/objects/patterns/PatternCatalog.h>
#include <plugins/crystalanalysis/data/DislocationNetwork.h>
#include <plugins/crystalanalysis/data/ClusterGraph.h>
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

	/// \brief Returns the display object that is responsible for rendering the defect mesh.
	SurfaceMeshDisplay* defectMeshDisplay() const { return _defectMeshDisplay; }

	/// \brief Returns the display object that is responsible for rendering the interface mesh.
	SurfaceMeshDisplay* interfaceMeshDisplay() const { return _interfaceMeshDisplay; }

	/// \brief Returns the display object that is responsible for rendering the dislocations.
	DislocationDisplay* dislocationDisplay() const { return _dislocationDisplay; }

	/// \brief Returns the internal modifier that smoothes the extracted dislocation lines.
	SmoothDislocationsModifier* smoothDislocationsModifier() const { return _smoothDislocationsModifier; }

	/// \brief Returns the internal modifier that smoothes the defect surface mesh.
	SmoothSurfaceModifier* smoothSurfaceModifier() const { return _smoothSurfaceModifier; }

	/// Return the catalog of structure patterns.
	PatternCatalog* patternCatalog() const { return _patternCatalog; }

	/// Returns the maximum length of trial circuits.
	int maxTrialCircuitSize() const { return _maxTrialCircuitSize; }

	/// Sets the maximum length of trial circuits.
	void setMaxTrialCircuitSize(int maxLength) { _maxTrialCircuitSize = maxLength; }

	/// Returns the maximum elongation of Burgers circuits while they are being advanced.
	int circuitStretchability() const { return _circuitStretchability; }

	/// Sets maximum elongation of Burgers circuits while they are being advanced.
	void setCircuitStretchability(int stretchability) { _circuitStretchability = stretchability; }

	/// Returns the type of crystal to be analyzed.
	int crystalStructure() const { return _crystalStructure; }

	/// Sets the type of crystal to be analyzed.
	void setCrystalStructure(int structureType) { _crystalStructure = structureType; }

	/// Returns whether the interface mesh is output.
	bool outputInterfaceMesh() const { return _outputInterfaceMesh; }

	/// Controls whether the interface mesh is output.
	void setOutputInterfaceMesh(bool enable) { _outputInterfaceMesh = enable; }

	/// Resets the modifier's result cache.
	virtual void invalidateCachedResults() override;

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
	PropertyField<int> _crystalStructure;

	/// The maximum length of trial circuits.
	PropertyField<int> _maxTrialCircuitSize;

	/// The maximum elongation of Burgers circuits while they are being advanced.
	PropertyField<int> _circuitStretchability;

	/// Controls the output of the interface mesh
	PropertyField<bool> _outputInterfaceMesh;

	/// The catalog of structure patterns.
	ReferenceField<PatternCatalog> _patternCatalog;

	/// The display object for rendering the defect mesh.
	ReferenceField<SurfaceMeshDisplay> _defectMeshDisplay;

	/// The display object for rendering the interface mesh.
	ReferenceField<SurfaceMeshDisplay> _interfaceMeshDisplay;

	/// The display object for rendering the dislocations.
	ReferenceField<DislocationDisplay> _dislocationDisplay;

	/// The internal modifier that smoothes the extracted dislocation lines.
	ReferenceField<SmoothDislocationsModifier> _smoothDislocationsModifier;

	/// The internal modifier that smoothes the defect surface mesh.
	ReferenceField<SmoothSurfaceModifier> _smoothSurfaceModifier;

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

	/// The cached simulation cell from the last analysis run.
	SimulationCell _simCell;

	/// Indicates that the entire simulation cell is part of the 'good' crystal region.
	bool _isGoodEverywhere;

	/// Indicates that the entire simulation cell is part of the 'bad' crystal region.
	bool _isBadEverywhere;

	/// List of edges, which don't have a lattice vector.
	QExplicitlySharedDataPointer<BondsStorage> _unassignedEdges;

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Dislocation analysis (DXA)");
	Q_CLASSINFO("ModifierCategory", "Analysis");

	DECLARE_PROPERTY_FIELD(_crystalStructure);
	DECLARE_PROPERTY_FIELD(_maxTrialCircuitSize);
	DECLARE_PROPERTY_FIELD(_circuitStretchability);
	DECLARE_PROPERTY_FIELD(_outputInterfaceMesh);
	DECLARE_REFERENCE_FIELD(_patternCatalog);
	DECLARE_REFERENCE_FIELD(_dislocationDisplay);
	DECLARE_REFERENCE_FIELD(_defectMeshDisplay);
	DECLARE_REFERENCE_FIELD(_interfaceMeshDisplay);
	DECLARE_REFERENCE_FIELD(_smoothDislocationsModifier);
	DECLARE_REFERENCE_FIELD(_smoothSurfaceModifier);
};

/**
 * Properties editor for the DislocationAnalysisModifier class.
 */
class OVITO_CRYSTALANALYSIS_EXPORT DislocationAnalysisModifierEditor : public ParticleModifierEditor
{
public:

	/// Default constructor.
	Q_INVOKABLE DislocationAnalysisModifierEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

private:

	/// The editor for the selected input structure type.
	OORef<PropertiesEditor> _structureTypeSubEditor;

	Q_OBJECT
	OVITO_OBJECT
};

}	// End of namespace
}	// End of namespace
}	// End of namespace

#endif // __OVITO_DISLOCATION_ANALYSIS_MODIFIER_H
