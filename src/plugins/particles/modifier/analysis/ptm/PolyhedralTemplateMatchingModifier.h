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
	static constexpr int MAX_NEIGHBORS = 14;
#else
	enum { MAX_NEIGHBORS = 14 };
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
		PTMEngine(const TimeInterval& validityInterval, ParticleProperty* positions, const SimulationCell& simCell,
				const QVector<bool>& typesToIdentify, ParticleProperty* selection,
				bool outputScalingFactor, bool outputOrientation, bool outputDeformationGradient) :
			StructureIdentificationEngine(validityInterval, positions, simCell, typesToIdentify, selection),
			_rmsd(new ParticleProperty(positions->size(), qMetaTypeId<FloatType>(), 1, 0, tr("RMSD"), false)),
			_scalingFactors(outputScalingFactor ? new ParticleProperty(positions->size(), qMetaTypeId<FloatType>(), 1, 0, tr("Scaling Factor"), true) : nullptr),
			_orientations(outputOrientation ? new ParticleProperty(positions->size(), ParticleProperty::OrientationProperty, 0, true) : nullptr),
			_deformationGradients(outputDeformationGradient ? new ParticleProperty(positions->size(), ParticleProperty::ElasticDeformationGradientProperty, 0, true) : nullptr) {}

		/// Computes the modifier's results and stores them in this object for later retrieval.
		virtual void perform() override;

		QExplicitlySharedDataPointer<ParticleProperty> _rmsd;
		QExplicitlySharedDataPointer<ParticleProperty> _scalingFactors;
		QExplicitlySharedDataPointer<ParticleProperty> _orientations;
		QExplicitlySharedDataPointer<ParticleProperty> _deformationGradients;
		QVector<int> _rmsdHistogramData;
		FloatType _rmsdHistogramBinSize;
	};

private:

	/// The original structures types determined by PTM before the RMSD cutoff is applied.
	QExplicitlySharedDataPointer<ParticleProperty> _originalStructureTypes;

	/// The computed per-particle RMSD values.
	QExplicitlySharedDataPointer<ParticleProperty> _rmsd;

	/// The computed per-particle scaling factors.
	QExplicitlySharedDataPointer<ParticleProperty> _scalingFactors;

	/// The computed per-particle orientations.
	QExplicitlySharedDataPointer<ParticleProperty> _orientations;

	/// The computed per-particle deformation gradients.
	QExplicitlySharedDataPointer<ParticleProperty> _deformationGradients;

	/// The RMSD cutoff.
	PropertyField<FloatType> _rmsdCutoff;

	/// Controls the output of the per-particle RMSD values.
	PropertyField<bool> _outputRmsd;

	/// Controls the output of local scaling values.
	PropertyField<bool> _outputScalingFactor;

	/// Controls the output of local orientations.
	PropertyField<bool> _outputOrientation;

	/// Controls the output of elastic deformation gradients.
	PropertyField<bool> _outputDeformationGradient;

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
	DECLARE_PROPERTY_FIELD(_outputScalingFactor);
	DECLARE_PROPERTY_FIELD(_outputOrientation);
	DECLARE_PROPERTY_FIELD(_outputDeformationGradient);
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace

#endif // __OVITO_PTM_MODIFIER_H
