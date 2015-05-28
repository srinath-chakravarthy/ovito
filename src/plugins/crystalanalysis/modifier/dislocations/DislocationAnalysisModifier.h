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
#include <plugins/particles/modifier/AsynchronousParticleModifier.h>
#include <plugins/particles/objects/SurfaceMeshDisplay.h>
#include <plugins/crystalanalysis/objects/dislocations/DislocationDisplay.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/*
 * Extracts dislocation lines from a crystal.
 */
class OVITO_CRYSTALANALYSIS_EXPORT DislocationAnalysisModifier : public AsynchronousParticleModifier
{
public:

	/// Constructor.
	Q_INVOKABLE DislocationAnalysisModifier(DataSet* dataset);

	/// \brief Returns the display object that is responsible for rendering the defect mesh.
	SurfaceMeshDisplay* defectMeshDisplay() const { return _defectMeshDisplay; }

	/// \brief Returns the display object that is responsible for rendering the dislocations.
	Objects::DislocationDisplay* dislocationDisplay() const { return _dislocationDisplay; }

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

	/// The display object for rendering the defect mesh.
	ReferenceField<SurfaceMeshDisplay> _defectMeshDisplay;

	/// The display object for rendering the dislocations.
	ReferenceField<Objects::DislocationDisplay> _dislocationDisplay;

	/// This stores the cached defect mesh produced by the modifier.
	QExplicitlySharedDataPointer<HalfEdgeMesh> _defectMesh;

	/// This stores the cached particle structure types computed by the modifier.
	QExplicitlySharedDataPointer<ParticleProperty> _structureTypes;

	/// This stores the cached particle cluster assignments computed by the modifier.
	QExplicitlySharedDataPointer<ParticleProperty> _atomClusters;

	/// Indicates that the entire simulation cell is part of the 'bad' crystal region.
	bool _isDefectRegionEverywhere;

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Dislocation analysis");
	Q_CLASSINFO("ModifierCategory", "Analysis");

	DECLARE_REFERENCE_FIELD(_dislocationDisplay);
	DECLARE_REFERENCE_FIELD(_defectMeshDisplay);
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

	Q_OBJECT
	OVITO_OBJECT
};

}	// End of namespace
}	// End of namespace
}	// End of namespace

#endif // __OVITO_DISLOCATION_ANALYSIS_MODIFIER_H
