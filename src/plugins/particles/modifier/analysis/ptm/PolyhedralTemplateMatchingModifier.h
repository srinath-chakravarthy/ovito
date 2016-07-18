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

#ifndef __OVITO_PTM_MODIFIER_H
#define __OVITO_PTM_MODIFIER_H

#include <plugins/particles/Particles.h>
#include <plugins/particles/modifier/analysis/StructureIdentificationModifier.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * \brief A modifier that uses the Polyhedral Template Matching method to identify
 *        local coordination structures.
 */
class OVITO_PARTICLES_EXPORT PolyhedralTemplateMatchingModifier : public StructureIdentificationModifier
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

	/// The alloy types recognized by the PTM library.
	enum AlloyType {
		ALLOY_NONE = 0,
		ALLOY_PURE = 1,
		ALLOY_L10 = 2,
		ALLOY_L12_CU = 3,
		ALLOY_L12_AU = 4,
		ALLOY_B2 = 5,

		NUM_ALLOY_TYPES 	//< This just counts the number of defined alloy types.
	};
	Q_ENUMS(AlloyType);

public:

	/// Constructor.
	Q_INVOKABLE PolyhedralTemplateMatchingModifier(DataSet* dataset);

	/// \brief Returns the RMSD cutoff.
	FloatType rmsdCutoff() const { return _rmsdCutoff; }

	/// \brief Sets the RMSD cutoff.
	void setRmsdCutoff(FloatType cutoff) { _rmsdCutoff = cutoff; }

	/// Returns the computed histogram of RMSD values.
	const QVector<int>& rmsdHistogramData() const { return _rmsdHistogramData; }

	/// Returns the bin size of the RMSD histogram.
	FloatType rmsdHistogramBinSize() const { return _rmsdHistogramBinSize; }

	/// Returns whether per-particle RMSD values are output by the modifier.
	bool outputRmsd() const { return _outputRmsd; }

	/// Sets whether per-particle RMSD values are output by the modifier.
	void setOutputRmsd(bool enable) { _outputRmsd = enable; }

	/// Returns whether local interatomic distances are output by the modifier.
	bool outputInteratomicDistance() const { return _outputInteratomicDistance; }

	/// Sets whether local interatomic distances are output by the modifier.
	void setOutputInteratomicDistance(bool enable) { _outputInteratomicDistance = enable; }

	/// Returns whether local orientations are output by the modifier.
	bool outputOrientation() const { return _outputOrientation; }

	/// Sets whether local orientations are output by the modifier.
	void setOutputOrientation(bool enable) { _outputOrientation = enable; }

	/// Returns whether elastic deformation gradients are output by the modifier.
	bool outputDeformationGradient() const { return _outputDeformationGradient; }

	/// Sets whether elastic deformation gradients are output by the modifier.
	void setOutputDeformationGradient(bool enable) { _outputDeformationGradient = enable; }

	/// Returns whether alloy types should be identified by the modifier.
	bool outputAlloyTypes() const { return _outputAlloyTypes; }

	/// Sets whether local alloy types should be identified by the modifier.
	void setOutputAlloyTypes(bool enable) { _outputAlloyTypes = enable; }

protected:

	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// Creates and initializes a computation engine that will compute the modifier's results.
	virtual std::shared_ptr<ComputeEngine> createEngine(TimePoint time, TimeInterval validityInterval) override;

	/// Unpacks the results of the computation engine and stores them in the modifier.
	virtual void transferComputationResults(ComputeEngine* engine) override;

	/// Lets the modifier insert the cached computation results into the modification pipeline.
	virtual PipelineStatus applyComputationResults(TimePoint time, TimeInterval& validityInterval) override;

private:

	/// Analysis engine that performs the PTM.
	class PTMEngine : public StructureIdentificationEngine
	{
	public:

		/// Constructor.
		PTMEngine(const TimeInterval& validityInterval, ParticleProperty* positions, ParticleProperty* particleTypes, const SimulationCell& simCell,
				const QVector<bool>& typesToIdentify, ParticleProperty* selection,
				bool outputInteratomicDistance, bool outputOrientation, bool outputDeformationGradient, bool outputAlloyTypes) :
			StructureIdentificationEngine(validityInterval, positions, simCell, typesToIdentify, selection),
			_particleTypes(particleTypes),
			_rmsd(new ParticleProperty(positions->size(), qMetaTypeId<FloatType>(), 1, 0, tr("RMSD"), false)),
			_interatomicDistances(outputInteratomicDistance ? new ParticleProperty(positions->size(), qMetaTypeId<FloatType>(), 1, 0, tr("Interatomic Distance"), true) : nullptr),
			_orientations(outputOrientation ? new ParticleProperty(positions->size(), ParticleProperty::OrientationProperty, 0, true) : nullptr),
			_deformationGradients(outputDeformationGradient ? new ParticleProperty(positions->size(), ParticleProperty::ElasticDeformationGradientProperty, 0, true) : nullptr),
			_alloyTypes(outputAlloyTypes ? new ParticleProperty(positions->size(), qMetaTypeId<int>(), 1, 0, tr("Alloy Type"), true) : nullptr) {}

		/// Computes the modifier's results and stores them in this object for later retrieval.
		virtual void perform() override;

		QExplicitlySharedDataPointer<ParticleProperty> _particleTypes;
		QExplicitlySharedDataPointer<ParticleProperty> _rmsd;
		QExplicitlySharedDataPointer<ParticleProperty> _interatomicDistances;
		QExplicitlySharedDataPointer<ParticleProperty> _orientations;
		QExplicitlySharedDataPointer<ParticleProperty> _deformationGradients;
		QExplicitlySharedDataPointer<ParticleProperty> _alloyTypes;
		QVector<int> _rmsdHistogramData;
		FloatType _rmsdHistogramBinSize;
	};

private:

	/// The original structures types determined by PTM before the RMSD cutoff is applied.
	QExplicitlySharedDataPointer<ParticleProperty> _originalStructureTypes;

	/// The computed per-particle RMSD values.
	QExplicitlySharedDataPointer<ParticleProperty> _rmsd;

	/// The computed per-particle interatomic distance.
	QExplicitlySharedDataPointer<ParticleProperty> _interatomicDistances;

	/// The computed per-particle orientations.
	QExplicitlySharedDataPointer<ParticleProperty> _orientations;

	/// The computed per-particle deformation gradients.
	QExplicitlySharedDataPointer<ParticleProperty> _deformationGradients;

	/// The alloy types identified by the PTM routine.
	QExplicitlySharedDataPointer<ParticleProperty> _alloyTypes;

	/// The RMSD cutoff.
	PropertyField<FloatType> _rmsdCutoff;

	/// Controls the output of the per-particle RMSD values.
	PropertyField<bool> _outputRmsd;

	/// Controls the output of local interatomic distances.
	PropertyField<bool> _outputInteratomicDistance;

	/// Controls the output of local orientations.
	PropertyField<bool> _outputOrientation;

	/// Controls the output of elastic deformation gradients.
	PropertyField<bool> _outputDeformationGradient;

	/// Controls the output of alloy structure types.
	PropertyField<bool> _outputAlloyTypes;

	/// The computed histogram of RMSD values.
	QVector<int> _rmsdHistogramData;

	/// The bin size of the RMSD histogram;
	FloatType _rmsdHistogramBinSize;

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Polyhedral template matching");
	Q_CLASSINFO("ModifierCategory", "Analysis");

	DECLARE_PROPERTY_FIELD(_rmsdCutoff);
	DECLARE_PROPERTY_FIELD(_outputRmsd);
	DECLARE_PROPERTY_FIELD(_outputInteratomicDistance);
	DECLARE_PROPERTY_FIELD(_outputOrientation);
	DECLARE_PROPERTY_FIELD(_outputDeformationGradient);
	DECLARE_PROPERTY_FIELD(_outputAlloyTypes);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace

#endif // __OVITO_PTM_MODIFIER_H
