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

#ifndef __OVITO_GRAIN_SEGMENTATION_MODIFIER_H
#define __OVITO_GRAIN_SEGMENTATION_MODIFIER_H

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/particles/modifier/analysis/StructureIdentificationModifier.h>
#include <plugins/crystalanalysis/objects/patterns/PatternCatalog.h>
#include <plugins/crystalanalysis/data/ClusterGraph.h>
#include <plugins/crystalanalysis/modifier/dxa/StructureAnalysis.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/*
 * Identifies the grains in a polycrystal.
 */
class OVITO_CRYSTALANALYSIS_EXPORT GrainSegmentationModifier : public StructureIdentificationModifier
{
public:

	/// Constructor.
	Q_INVOKABLE GrainSegmentationModifier(DataSet* dataset);

	/// Return the catalog of structure patterns.
	PatternCatalog* patternCatalog() const { return _patternCatalog; }

	/// Returns the type of crystal to be analyzed.
	StructureAnalysis::LatticeStructureType inputCrystalStructure() const { return static_cast<StructureAnalysis::LatticeStructureType>(_inputCrystalStructure.value()); }

	/// Sets the type of crystal to be analyzed.
	void setInputCrystalStructure(StructureAnalysis::LatticeStructureType structureType) { _inputCrystalStructure = structureType; }

	/// Returns the minimum misorientation angle between adjacent grains.
	FloatType misorientationThreshold() const { return _misorientationThreshold; }

	/// Sets the minimum misorientation angle between adjacent grains.
	void setMisorientationThreshold(FloatType threshold) { _misorientationThreshold = threshold; }

	/// Returns the amount of noise tolerated within a grain.
	FloatType fluctuationTolerance() const { return _fluctuationTolerance; }

	/// Sets the amount of noise tolerated within a grain.
	void setFluctuationTolerance(FloatType tolerance) { _fluctuationTolerance = tolerance; }

	/// Returns the minimum number of crystalline atoms per grain.
	int minGrainAtomCount() const { return _minGrainAtomCount; }

	/// Sets the minimum number of crystalline atoms per grain.
	void setMinGrainAtomCount(int minAtoms) { _minGrainAtomCount = minAtoms; }

	/// Resets the modifier's result cache.
	virtual void invalidateCachedResults() override;

protected:

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
	PropertyField<int> _inputCrystalStructure;

	/// The minimum misorientation angle between adjacent grains.
	PropertyField<FloatType> _misorientationThreshold;

	/// Controls the amount of noise allowed inside a grain.
	PropertyField<FloatType> _fluctuationTolerance;

	/// The minimum number of crystalline atoms per grain.
	PropertyField<int> _minGrainAtomCount;

	/// The catalog of structure patterns.
	ReferenceField<PatternCatalog> _patternCatalog;

	/// This stores the cached atom-to-cluster assignments computed by the modifier.
	QExplicitlySharedDataPointer<ParticleProperty> _atomClusters;

	/// This stores the cached cluster graph computed by the modifier.
	QExplicitlySharedDataPointer<ClusterGraph> _clusterGraph;

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Grain segmentation");
	Q_CLASSINFO("ModifierCategory", "Analysis");

	DECLARE_PROPERTY_FIELD(_inputCrystalStructure);
	DECLARE_PROPERTY_FIELD(_misorientationThreshold);
	DECLARE_PROPERTY_FIELD(_fluctuationTolerance);
	DECLARE_PROPERTY_FIELD(_minGrainAtomCount);
	DECLARE_REFERENCE_FIELD(_patternCatalog);
};

/**
 * Properties editor for the GrainSegmentationModifier class.
 */
class OVITO_CRYSTALANALYSIS_EXPORT GrainSegmentationModifierEditor : public ParticleModifierEditor
{
public:

	/// Default constructor.
	Q_INVOKABLE GrainSegmentationModifierEditor() {}

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

#endif // __OVITO_GRAIN_SEGMENTATION_MODIFIER_H
