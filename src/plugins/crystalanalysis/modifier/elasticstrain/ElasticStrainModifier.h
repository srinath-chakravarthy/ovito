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

#ifndef __OVITO_ELASTIC_STRAIN_MODIFIER_H
#define __OVITO_ELASTIC_STRAIN_MODIFIER_H

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/particles/modifier/analysis/StructureIdentificationModifier.h>
#include <plugins/crystalanalysis/objects/patterns/PatternCatalog.h>
#include <plugins/crystalanalysis/data/ClusterGraph.h>
#include <plugins/crystalanalysis/modifier/dxa/StructureAnalysis.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/*
 * Extracts dislocation lines from a crystal.
 */
class OVITO_CRYSTALANALYSIS_EXPORT ElasticStrainModifier : public StructureIdentificationModifier
{
public:

	/// Constructor.
	Q_INVOKABLE ElasticStrainModifier(DataSet* dataset);

	/// Return the catalog of structure patterns.
	PatternCatalog* patternCatalog() const { return _patternCatalog; }

	/// Returns the type of crystal to be analyzed.
	StructureAnalysis::LatticeStructureType inputCrystalStructure() const { return static_cast<StructureAnalysis::LatticeStructureType>(_inputCrystalStructure.value()); }

	/// Sets the type of crystal to be analyzed.
	void setInputCrystalStructure(StructureAnalysis::LatticeStructureType structureType) { _inputCrystalStructure = structureType; }

	/// Returns whether atomic deformation gradient tensors should be computed and stored.
	bool calculateDeformationGradients() const { return _calculateDeformationGradients; }

	/// Sets whether atomic deformation gradient tensors should be computed and stored.
	void setCalculateDeformationGradients(bool enableCalculation) { _calculateDeformationGradients = enableCalculation; }

	/// Returns whether atomic strain tensors should be computed and stored.
	bool calculateStrainTensors() const { return _calculateStrainTensors; }

	/// Sets whether atomic strain tensors should be computed and stored.
	void setCalculateStrainTensors(bool enableCalculation) { _calculateStrainTensors = enableCalculation; }

	/// Returns whether the calculated strain tensors are pushed forward to the spatial reference frame.
	bool pushStrainTensorsForward() const { return _pushStrainTensorsForward; }

	/// Returns whether the calculated strain tensors should be pushed forward to the spatial reference frame.
	void setPushStrainTensorsForward(bool enable) { _pushStrainTensorsForward = enable; }

	/// Returns the lattice parameter of ideal crystal.
	FloatType latticeConstant() const { return _latticeConstant; }

	/// Sets the lattice parameter of ideal crystal.
	void setLatticeConstant(FloatType a) { _latticeConstant = a; }

	/// Returns the c/a ratio of the ideal crystal.
	FloatType axialRatio() const { return _caRatio; }

	/// Sets the c/a ratio of the ideal crystal.
	void setAxialRatio(FloatType ratio) { _caRatio = ratio; }

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

	/// Controls whether atomic deformation gradient tensors should be computed and stored.
	PropertyField<bool> _calculateDeformationGradients;

	/// Controls whether atomic strain tensors should be computed and stored.
	PropertyField<bool> _calculateStrainTensors;

	/// Controls whether the calculated strain tensors should be pushed forward to the spatial reference frame.
	PropertyField<bool> _pushStrainTensorsForward;

	/// The lattice parameter of ideal crystal.
	PropertyField<FloatType> _latticeConstant;

	/// The c/a ratio of the ideal crystal.
	PropertyField<FloatType> _caRatio;

	/// The catalog of structure patterns.
	ReferenceField<PatternCatalog> _patternCatalog;

	/// This stores the cached atom-to-cluster assignments computed by the modifier.
	QExplicitlySharedDataPointer<ParticleProperty> _atomClusters;

	/// This stores the cached cluster graph computed by the modifier.
	QExplicitlySharedDataPointer<ClusterGraph> _clusterGraph;

	/// This stores the cached results of the modifier.
	QExplicitlySharedDataPointer<ParticleProperty> _volumetricStrainValues;

	/// This stores the cached results of the modifier.
	QExplicitlySharedDataPointer<ParticleProperty> _strainTensors;

	/// This stores the cached results of the modifier.
	QExplicitlySharedDataPointer<ParticleProperty> _deformationGradients;

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Elastic strain calculation");
	Q_CLASSINFO("ModifierCategory", "Analysis");

	DECLARE_PROPERTY_FIELD(_inputCrystalStructure);
	DECLARE_PROPERTY_FIELD(_calculateDeformationGradients);
	DECLARE_PROPERTY_FIELD(_calculateStrainTensors);
	DECLARE_PROPERTY_FIELD(_latticeConstant);
	DECLARE_PROPERTY_FIELD(_caRatio);
	DECLARE_PROPERTY_FIELD(_pushStrainTensorsForward);
	DECLARE_REFERENCE_FIELD(_patternCatalog);
};

/**
 * Properties editor for the ElasticStrainModifier class.
 */
class OVITO_CRYSTALANALYSIS_EXPORT ElasticStrainModifierEditor : public ParticleModifierEditor
{
public:

	/// Default constructor.
	Q_INVOKABLE ElasticStrainModifierEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

private Q_SLOTS:

	/// Is called each time the parameters of the modifier have changed.
	void modifierChanged(RefTarget* editObject);

private:

	FloatParameterUI* _caRatioUI;

	Q_OBJECT
	OVITO_OBJECT
};

}	// End of namespace
}	// End of namespace
}	// End of namespace

#endif // __OVITO_ELASTIC_STRAIN_MODIFIER_H
