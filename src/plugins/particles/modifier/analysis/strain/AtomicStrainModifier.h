///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2014) Alexander Stukowski
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


#include <plugins/particles/Particles.h>
#include <plugins/particles/data/ParticleProperty.h>
#include <plugins/particles/util/CutoffNeighborFinder.h>
#include "../../AsynchronousParticleModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * \brief Calculates the per-particle strain tensors based on a reference configuration.
 */
class OVITO_PARTICLES_EXPORT AtomicStrainModifier : public AsynchronousParticleModifier
{
public:

	/// Constructor.
	Q_INVOKABLE AtomicStrainModifier(DataSet* dataset);

	/// Returns the computed von Mises shear strain values.
	const ParticleProperty& shearStrainValues() const { OVITO_CHECK_POINTER(_shearStrainValues.constData()); return *_shearStrainValues; }

	/// After a successful evaluation of the modifier, this returns the number of invalid particles for which
	/// the strain tensor could not be computed.
	size_t invalidParticleCount() const { return _numInvalidParticles; }

protected:

	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// Is called when a RefTarget referenced by this object has generated an event.
	virtual bool referenceEvent(RefTarget* source, ReferenceEvent* event) override;

	/// Creates a computation engine that will compute the modifier's results.
	virtual std::shared_ptr<ComputeEngine> createEngine(TimePoint time, TimeInterval validityInterval) override;

	/// Unpacks the results of the computation engine and stores them in the modifier.
	virtual void transferComputationResults(ComputeEngine* engine) override;

	/// Lets the modifier insert the cached computation results into the modification pipeline.
	virtual PipelineStatus applyComputationResults(TimePoint time, TimeInterval& validityInterval) override;

private:

	/// Computes the modifier's results.
	class AtomicStrainEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		AtomicStrainEngine(const TimeInterval& validityInterval, ParticleProperty* positions, const SimulationCell& simCell,
				ParticleProperty* refPositions, const SimulationCell& simCellRef,
				ParticleProperty* identifiers, ParticleProperty* refIdentifiers,
				FloatType cutoff, bool eliminateCellDeformation, bool assumeUnwrappedCoordinates,
				bool calculateDeformationGradients, bool calculateStrainTensors,
				bool calculateNonaffineSquaredDisplacements, bool calculateRotations, bool calculateStretchTensors) :
			ComputeEngine(validityInterval),
			_positions(positions), _simCell(simCell),
			_refPositions(refPositions), _simCellRef(simCellRef),
			_identifiers(identifiers), _refIdentifiers(refIdentifiers),
			_cutoff(cutoff), _eliminateCellDeformation(eliminateCellDeformation), _assumeUnwrappedCoordinates(assumeUnwrappedCoordinates),
			_shearStrains(new ParticleProperty(positions->size(), qMetaTypeId<FloatType>(), 1, 0, tr("Shear Strain"), false)),
			_volumetricStrains(new ParticleProperty(positions->size(), qMetaTypeId<FloatType>(), 1, 0, tr("Volumetric Strain"), false)),
			_strainTensors(calculateStrainTensors ? new ParticleProperty(positions->size(), ParticleProperty::StrainTensorProperty, 0, false) : nullptr),
			_deformationGradients(calculateDeformationGradients ? new ParticleProperty(positions->size(), ParticleProperty::DeformationGradientProperty, 0, false) : nullptr),
			_nonaffineSquaredDisplacements(calculateNonaffineSquaredDisplacements ? new ParticleProperty(positions->size(), qMetaTypeId<FloatType>(), 1, 0, tr("Nonaffine Squared Displacement"), false) : nullptr),
			_invalidParticles(new ParticleProperty(positions->size(), ParticleProperty::SelectionProperty, 0, false)),
			_rotations(calculateRotations ? new ParticleProperty(positions->size(), ParticleProperty::RotationProperty, 0, false) : nullptr),
			_stretchTensors(calculateStretchTensors ? new ParticleProperty(positions->size(), ParticleProperty::StretchTensorProperty, 0, false) : nullptr) {}

		/// Computes the modifier's results and stores them in this object for later retrieval.
		virtual void perform() override;

		/// Returns the property storage that contains the input particle positions.
		ParticleProperty* positions() const { return _positions.data(); }

		/// Returns the property storage that contains the reference particle positions.
		ParticleProperty* refPositions() const { return _refPositions.data(); }

		/// Returns the simulation cell data.
		const SimulationCell& cell() const { return _simCell; }

		/// Returns the reference simulation cell data.
		const SimulationCell& refCell() const { return _simCellRef; }

		/// Returns the property storage that contains the computed per-particle shear strain values.
		ParticleProperty* shearStrains() const { return _shearStrains.data(); }

		/// Returns the property storage that contains the computed per-particle volumetric strain values.
		ParticleProperty* volumetricStrains() const { return _volumetricStrains.data(); }

		/// Returns the property storage that contains the computed per-particle strain tensors.
		ParticleProperty* strainTensors() const { return _strainTensors.data(); }

		/// Returns the property storage that contains the computed per-particle deformation gradient tensors.
		ParticleProperty* deformationGradients() const { return _deformationGradients.data(); }

		/// Returns the property storage that contains the computed per-particle deformation gradient tensors.
		ParticleProperty* nonaffineSquaredDisplacements() const { return _nonaffineSquaredDisplacements.data(); }

		/// Returns the property storage that contains the selection of invalid particles.
		ParticleProperty* invalidParticles() const { return _invalidParticles.data(); }

		/// Returns the property storage that contains the computed rotations.
		ParticleProperty* rotations() const { return _rotations.data(); }

		/// Returns the property storage that contains the computed stretch tensors.
		ParticleProperty* stretchTensors() const { return _stretchTensors.data(); }

		/// Returns the number of invalid particles for which the strain tensor could not be computed.
		size_t numInvalidParticles() const { return _numInvalidParticles.load(); }

	private:

		/// Computes the strain tensor of a single particle.
		bool computeStrain(size_t particleIndex, CutoffNeighborFinder& neighborListBuilder, const std::vector<int>& refToCurrentIndexMap, const std::vector<int>& currentToRefIndexMap);

		FloatType _cutoff;
		SimulationCell _simCell;
		SimulationCell _simCellRef;
		AffineTransformation _currentSimCellInv;
		AffineTransformation _reducedToAbsolute;
		QExplicitlySharedDataPointer<ParticleProperty> _positions;
		QExplicitlySharedDataPointer<ParticleProperty> _refPositions;
		QExplicitlySharedDataPointer<ParticleProperty> _identifiers;
		QExplicitlySharedDataPointer<ParticleProperty> _refIdentifiers;
		QExplicitlySharedDataPointer<ParticleProperty> _shearStrains;
		QExplicitlySharedDataPointer<ParticleProperty> _volumetricStrains;
		QExplicitlySharedDataPointer<ParticleProperty> _strainTensors;
		QExplicitlySharedDataPointer<ParticleProperty> _deformationGradients;
		QExplicitlySharedDataPointer<ParticleProperty> _nonaffineSquaredDisplacements;
		QExplicitlySharedDataPointer<ParticleProperty> _invalidParticles;
		QExplicitlySharedDataPointer<ParticleProperty> _rotations;
		QExplicitlySharedDataPointer<ParticleProperty> _stretchTensors;
		bool _eliminateCellDeformation;
		bool _assumeUnwrappedCoordinates;
		QAtomicInt _numInvalidParticles;
	};

	/// This stores the cached results of the modifier.
	QExplicitlySharedDataPointer<ParticleProperty> _shearStrainValues;

	/// This stores the cached results of the modifier.
	QExplicitlySharedDataPointer<ParticleProperty> _volumetricStrainValues;

	/// This stores the cached results of the modifier.
	QExplicitlySharedDataPointer<ParticleProperty> _strainTensors;

	/// This stores the cached results of the modifier.
	QExplicitlySharedDataPointer<ParticleProperty> _deformationGradients;

	/// This stores the cached results of the modifier.
	QExplicitlySharedDataPointer<ParticleProperty> _nonaffineSquaredDisplacements;

	/// This stores the selection of invalid particles.
	QExplicitlySharedDataPointer<ParticleProperty> _invalidParticles;

	/// This stores the computed rotations.
	QExplicitlySharedDataPointer<ParticleProperty> _rotations;

	/// This stores the computed stretch tensors.
	QExplicitlySharedDataPointer<ParticleProperty> _stretchTensors;

	/// The reference configuration.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(DataObject, referenceConfiguration, setReferenceConfiguration);

	/// Controls the whether the reference configuration is shown instead of the current configuration.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, referenceShown, setReferenceShown);

	/// Controls the whether the homogeneous deformation of the simulation cell is eliminated from the calculated displacement vectors.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, eliminateCellDeformation, setEliminateCellDeformation);

	/// Controls the whether we assume the particle coordinates are unwrapped when calculating the displacement vectors.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, assumeUnwrappedCoordinates, setAssumeUnwrappedCoordinates);

	/// Controls the cutoff radius for the neighbor lists.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, cutoff, setCutoff);

	/// Controls the whether atomic deformation gradient tensors should be computed and stored.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, calculateDeformationGradients, setCalculateDeformationGradients);

	/// Controls the whether atomic strain tensors should be computed and stored.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, calculateStrainTensors, setCalculateStrainTensors);

	/// Controls the whether non-affine displacements should be computed and stored.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, calculateNonaffineSquaredDisplacements, setCalculateNonaffineSquaredDisplacements);

	/// Controls the whether local rotations should be computed and stored.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, calculateRotations, setCalculateRotations);

	/// Controls the whether atomic stretch tensors should be computed and stored.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, calculateStretchTensors, setCalculateStretchTensors);

	/// Controls the whether particles, for which the strain tensor could not be computed, are selected.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, selectInvalidParticles, setSelectInvalidParticles);

	/// Specify reference frame relative to current frame.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, useReferenceFrameOffset, setUseReferenceFrameOffset);

	/// Absolute frame number from reference file to use when calculating displacement vectors.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, referenceFrameNumber, setReferenceFrameNumber);

	/// Relative frame offset for reference coordinates.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, referenceFrameOffset, setReferenceFrameOffset);

	/// Counts the number of invalid particles for which the strain tensor could not be computed.
	size_t _numInvalidParticles;

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Atomic strain");
	Q_CLASSINFO("ModifierCategory", "Analysis");
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


