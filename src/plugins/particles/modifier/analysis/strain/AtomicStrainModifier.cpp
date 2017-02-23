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

#include <plugins/particles/Particles.h>
#include <core/scene/objects/DataObject.h>
#include <core/scene/pipeline/PipelineEvalRequest.h>
#include <core/animation/AnimationSettings.h>
#include <core/dataset/importexport/FileSource.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include <ptm/polar_decomposition.hpp>
#include "AtomicStrainModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(AtomicStrainModifier, AsynchronousParticleModifier);
DEFINE_FLAGS_REFERENCE_FIELD(AtomicStrainModifier, referenceConfiguration, "Reference Configuration", DataObject, PROPERTY_FIELD_NO_SUB_ANIM);
DEFINE_PROPERTY_FIELD(AtomicStrainModifier, referenceShown, "ShowReferenceConfiguration");
DEFINE_FLAGS_PROPERTY_FIELD(AtomicStrainModifier, eliminateCellDeformation, "EliminateCellDeformation", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(AtomicStrainModifier, assumeUnwrappedCoordinates, "AssumeUnwrappedCoordinates");
DEFINE_FLAGS_PROPERTY_FIELD(AtomicStrainModifier, cutoff, "Cutoff", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(AtomicStrainModifier, calculateDeformationGradients, "CalculateDeformationGradients");
DEFINE_PROPERTY_FIELD(AtomicStrainModifier, calculateStrainTensors, "CalculateStrainTensors");
DEFINE_PROPERTY_FIELD(AtomicStrainModifier, calculateNonaffineSquaredDisplacements, "CalculateNonaffineSquaredDisplacements");
DEFINE_PROPERTY_FIELD(AtomicStrainModifier, selectInvalidParticles, "SelectInvalidParticles");
DEFINE_PROPERTY_FIELD(AtomicStrainModifier, useReferenceFrameOffset, "UseReferenceFrameOffet");
DEFINE_PROPERTY_FIELD(AtomicStrainModifier, referenceFrameNumber, "ReferenceFrameNumber");
DEFINE_FLAGS_PROPERTY_FIELD(AtomicStrainModifier, referenceFrameOffset, "ReferenceFrameOffset", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(AtomicStrainModifier, calculateStretchTensors, "CalculateStretchTensors");
DEFINE_PROPERTY_FIELD(AtomicStrainModifier, calculateRotations, "CalculateRotations");
SET_PROPERTY_FIELD_LABEL(AtomicStrainModifier, referenceConfiguration, "Reference Configuration");
SET_PROPERTY_FIELD_LABEL(AtomicStrainModifier, referenceShown, "Show reference configuration");
SET_PROPERTY_FIELD_LABEL(AtomicStrainModifier, eliminateCellDeformation, "Eliminate homogeneous cell deformation");
SET_PROPERTY_FIELD_LABEL(AtomicStrainModifier, assumeUnwrappedCoordinates, "Assume unwrapped coordinates");
SET_PROPERTY_FIELD_LABEL(AtomicStrainModifier, cutoff, "Cutoff radius");
SET_PROPERTY_FIELD_LABEL(AtomicStrainModifier, calculateDeformationGradients, "Output deformation gradient tensors");
SET_PROPERTY_FIELD_LABEL(AtomicStrainModifier, calculateStrainTensors, "Output strain tensors");
SET_PROPERTY_FIELD_LABEL(AtomicStrainModifier, calculateNonaffineSquaredDisplacements, "Output non-affine squared displacements (D^2_min)");
SET_PROPERTY_FIELD_LABEL(AtomicStrainModifier, selectInvalidParticles, "Select invalid particles");
SET_PROPERTY_FIELD_LABEL(AtomicStrainModifier, useReferenceFrameOffset, "Use reference frame offset");
SET_PROPERTY_FIELD_LABEL(AtomicStrainModifier, referenceFrameNumber, "Reference frame number");
SET_PROPERTY_FIELD_LABEL(AtomicStrainModifier, referenceFrameOffset, "Reference frame offset");
SET_PROPERTY_FIELD_LABEL(AtomicStrainModifier, calculateStretchTensors, "Output stretch tensors");
SET_PROPERTY_FIELD_LABEL(AtomicStrainModifier, calculateRotations, "Output rotations");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(AtomicStrainModifier, cutoff, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(AtomicStrainModifier, referenceFrameNumber, IntegerParameterUnit, 1);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
AtomicStrainModifier::AtomicStrainModifier(DataSet* dataset) : AsynchronousParticleModifier(dataset),
	_referenceShown(false), _eliminateCellDeformation(false), _assumeUnwrappedCoordinates(false),
    _cutoff(3), _calculateDeformationGradients(false), _calculateStrainTensors(false), _calculateNonaffineSquaredDisplacements(false),
	_calculateStretchTensors(false), _calculateRotations(false),
    _selectInvalidParticles(true),
    _useReferenceFrameOffset(false), _referenceFrameNumber(0), _referenceFrameOffset(-1)
{
	INIT_PROPERTY_FIELD(referenceConfiguration);
	INIT_PROPERTY_FIELD(referenceShown);
	INIT_PROPERTY_FIELD(eliminateCellDeformation);
	INIT_PROPERTY_FIELD(assumeUnwrappedCoordinates);
	INIT_PROPERTY_FIELD(cutoff);
	INIT_PROPERTY_FIELD(calculateDeformationGradients);
	INIT_PROPERTY_FIELD(calculateStrainTensors);
	INIT_PROPERTY_FIELD(calculateNonaffineSquaredDisplacements);
	INIT_PROPERTY_FIELD(selectInvalidParticles);
	INIT_PROPERTY_FIELD(useReferenceFrameOffset);
	INIT_PROPERTY_FIELD(referenceFrameNumber);
	INIT_PROPERTY_FIELD(referenceFrameOffset);
	INIT_PROPERTY_FIELD(calculateStretchTensors);
	INIT_PROPERTY_FIELD(calculateRotations);

	// Create the file source object, which will be responsible for loading
	// and storing the reference configuration.
	OORef<FileSource> linkedFileObj(new FileSource(dataset));

	// Disable automatic adjustment of animation length for the reference object.
	// We don't want the scene's animation interval to be affected by an animation
	// loaded into the reference configuration object.
	linkedFileObj->setAdjustAnimationIntervalEnabled(false);
	setReferenceConfiguration(linkedFileObj);
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
std::shared_ptr<AsynchronousParticleModifier::ComputeEngine> AtomicStrainModifier::createEngine(TimePoint time, TimeInterval validityInterval)
{
	// Get the current positions.
	ParticlePropertyObject* posProperty = expectStandardProperty(ParticleProperty::PositionProperty);

	// Get the reference positions of the particles.
	if(!referenceConfiguration())
		throwException(tr("Cannot calculate displacements. Reference configuration has not been specified."));

	// What is the reference frame number to use?
	int referenceFrame;
	if(_useReferenceFrameOffset) {
		// Determine the current frame, preferably from the attributes stored with the pipeline flow state.
		// If the "SourceFrame" attribute is not present, infer it from the current animation time.
		int currentFrame = input().attributes().value(QStringLiteral("SourceFrame"),
				dataset()->animationSettings()->timeToFrame(time)).toInt();

		// Use frame offset relative to current configuration.
		referenceFrame = currentFrame + _referenceFrameOffset;

		// Results will only be valid for duration of current frame.
		validityInterval.intersect(time);
	}
	else {
		// Always use the same, user-specified frame as reference configuration.
		referenceFrame = _referenceFrameNumber;
	}

	// Get the reference configuration.
	PipelineFlowState refState;
	if(FileSource* fileSource = dynamic_object_cast<FileSource>(referenceConfiguration())) {
		if(fileSource->numberOfFrames() > 0) {
			if(referenceFrame < 0 || referenceFrame >= fileSource->numberOfFrames())
				throwException(tr("Requested reference frame %1 is out of range.").arg(referenceFrame));
			refState = fileSource->requestFrame(referenceFrame);
		}
	}
	else refState = referenceConfiguration()->evaluateImmediately(PipelineEvalRequest(dataset()->animationSettings()->frameToTime(referenceFrame), false));

	// Make sure the obtained reference configuration is valid and ready to use.
	if(refState.status().type() == PipelineStatus::Error)
		throw refState.status();
	if(refState.status().type() == PipelineStatus::Pending)
		throw PipelineStatus(PipelineStatus::Pending, tr("Waiting for input data to become ready..."));
	if(refState.isEmpty())
		throwException(tr("Reference configuration has not been specified yet or is empty. Please pick a reference simulation file."));

	// Make sure we really got back the requested reference frame.
	if(refState.attributes().value(QStringLiteral("SourceFrame"), referenceFrame).toInt() != referenceFrame)
		throwException(tr("Requested reference frame %1 is out of range.").arg(referenceFrame));

	// Get the reference position property.
	ParticlePropertyObject* refPosProperty = ParticlePropertyObject::findInState(refState, ParticleProperty::PositionProperty);
	if(!refPosProperty)
		throwException(tr("The reference configuration does not contain particle positions."));

	// Get simulation cells.
	SimulationCellObject* inputCell = expectSimulationCell();
	SimulationCellObject* refCell = refState.findObject<SimulationCellObject>();
	if(!refCell)
		throwException(tr("Reference configuration does not contain simulation cell info."));

	// Check simulation cell(s).
	if((!inputCell->is2D() && inputCell->volume3D() < FLOATTYPE_EPSILON) || (inputCell->is2D() && inputCell->volume2D() < FLOATTYPE_EPSILON))
		throwException(tr("Simulation cell is degenerate in the deformed configuration."));
	if((!inputCell->is2D() && refCell->volume3D() < FLOATTYPE_EPSILON) || (inputCell->is2D() && refCell->volume2D() < FLOATTYPE_EPSILON))
		throwException(tr("Simulation cell is degenerate in the reference configuration."));

	// Get particle identifiers.
	ParticlePropertyObject* identifierProperty = inputStandardProperty(ParticleProperty::IdentifierProperty);
	ParticlePropertyObject* refIdentifierProperty = ParticlePropertyObject::findInState(refState, ParticleProperty::IdentifierProperty);

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<AtomicStrainEngine>(validityInterval, posProperty->storage(), inputCell->data(), refPosProperty->storage(), refCell->data(),
			identifierProperty ? identifierProperty->storage() : nullptr, refIdentifierProperty ? refIdentifierProperty->storage() : nullptr,
            cutoff(), eliminateCellDeformation(), assumeUnwrappedCoordinates(), calculateDeformationGradients(), calculateStrainTensors(),
            calculateNonaffineSquaredDisplacements(), calculateRotations(), calculateStretchTensors());
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void AtomicStrainModifier::AtomicStrainEngine::perform()
{
	setProgressText(tr("Computing atomic strain tensors"));

	// Build particle-to-particle index maps.
	std::vector<int> currentToRefIndexMap(positions()->size());
	std::vector<int> refToCurrentIndexMap(refPositions()->size());
	if(_identifiers && _refIdentifiers) {
		OVITO_ASSERT(_identifiers->size() == positions()->size());
		OVITO_ASSERT(_refIdentifiers->size() == refPositions()->size());

		// Build map of particle identifiers in reference configuration.
		std::map<int, int> refMap;
		int index = 0;
		for(int id : _refIdentifiers->constIntRange()) {
			if(refMap.insert(std::make_pair(id, index)).second == false)
				throw Exception(tr("Particles with duplicate identifiers detected in reference configuration."));
			index++;
		}

		if(isCanceled())
			return;

		// Check for duplicate identifiers in current configuration
#if 0
		std::vector<size_t> idSet(_identifiers->constDataInt(), _identifiers->constDataInt() + _identifiers->size());
		std::sort(idSet.begin(), idSet.end());
		if(std::adjacent_find(idSet.begin(), idSet.end()) != idSet.end())
			throw Exception(tr("Particles with duplicate identifiers detected in input configuration."));
#else
		std::map<int, int> currentMap;
		index = 0;
		for(int id : _identifiers->constIntRange()) {
			if(currentMap.insert(std::make_pair(id, index)).second == false)
				throw Exception(tr("Particles with duplicate identifiers detected in current configuration."));
			index++;
		}
#endif

		if(isCanceled())
			return;

		// Build index maps.
		const int* id = _identifiers->constDataInt();
		for(auto& mappedIndex : currentToRefIndexMap) {
			auto iter = refMap.find(*id);
			if(iter != refMap.end())
				mappedIndex = iter->second;
			else
				mappedIndex = -1;
			++id;
		}

		if(isCanceled())
			return;

		id = _refIdentifiers->constDataInt();
		for(auto& mappedIndex : refToCurrentIndexMap) {
			auto iter = currentMap.find(*id);
			if(iter != currentMap.end())
				mappedIndex = iter->second;
			else
				mappedIndex = -1;
			++id;
		}
	}
	else {
		// Deformed and reference configuration must contain the same number of particles.
		if(positions()->size() != refPositions()->size())
			throw Exception(tr("Cannot calculate displacements. Numbers of particles in reference configuration and current configuration do not match."));
		// When particle identifiers are not available, use trivial 1-to-1 mapping.
		std::iota(refToCurrentIndexMap.begin(), refToCurrentIndexMap.end(), 0);
		std::iota(currentToRefIndexMap.begin(), currentToRefIndexMap.end(), 0);
	}
	if(isCanceled())
		return;

	// Automatically disable PBCs in Z direction for 2D systems.
	if(_simCell.is2D()) {
		_simCell.setPbcFlags(_simCell.pbcFlags()[0], _simCell.pbcFlags()[1], false);
		// Make sure the matrix is invertible.
		AffineTransformation m = _simCell.matrix();
		m.column(2) = Vector3(0,0,1);
		_simCell.setMatrix(m);
		m = _simCellRef.matrix();
		m.column(2) = Vector3(0,0,1);
		_simCellRef.setMatrix(m);
	}

	// PBCs flags of the current configuration always override PBCs flags
	// of the reference config.
	_simCellRef.setPbcFlags(_simCell.pbcFlags());
	_simCellRef.set2D(_simCell.is2D());

	// Precompute matrices.
	_currentSimCellInv = _simCell.inverseMatrix();
	_reducedToAbsolute = _eliminateCellDeformation ? _simCellRef.matrix() : _simCell.matrix();	

	// Prepare the neighbor list for the reference configuration.
	CutoffNeighborFinder neighborFinder;
	if(!neighborFinder.prepare(_cutoff, refPositions(), refCell(), nullptr, *this))
		return;

	// Perform individual strain calculation for each particle.
	parallelFor(positions()->size(), *this, [&neighborFinder, &refToCurrentIndexMap, &currentToRefIndexMap, this](size_t index) {
		if(!this->computeStrain(index, neighborFinder, refToCurrentIndexMap, currentToRefIndexMap))
			_numInvalidParticles.fetchAndAddRelaxed(1);
	});
}

/******************************************************************************
* Computes the strain tensor of a single particle.
******************************************************************************/
bool AtomicStrainModifier::AtomicStrainEngine::computeStrain(size_t particleIndex, CutoffNeighborFinder& neighborFinder, const std::vector<int>& refToCurrentIndexMap, const std::vector<int>& currentToRefIndexMap)
{
	// We do the following calculations using double precision to
	// get best results. Final results will be converted back to
	// standard precision numbers.

	Matrix_3<double> V = Matrix_3<double>::Zero();
	Matrix_3<double> W = Matrix_3<double>::Zero();
	int numNeighbors = 0;

	// Iterate over neighbors of central particle.
	int particleIndexReference = currentToRefIndexMap[particleIndex];
	if(particleIndexReference != -1) {
		const Point3 x = positions()->getPoint3(particleIndex);
		for(CutoffNeighborFinder::Query neighQuery(neighborFinder, particleIndexReference); !neighQuery.atEnd(); neighQuery.next()) {
			const Vector3& r0 = neighQuery.delta();
			int neighborIndexCurrent = refToCurrentIndexMap[neighQuery.current()];
			if(neighborIndexCurrent == -1) continue;
			Vector3 r = positions()->getPoint3(neighborIndexCurrent) - x;
			Vector3 sr = _currentSimCellInv * r;
			if(!_assumeUnwrappedCoordinates) {
				for(size_t k = 0; k < 3; k++) {
					if(_simCell.pbcFlags()[k])
						sr[k] -= floor(sr[k] + FloatType(0.5));
				}
			}
			r = _reducedToAbsolute * sr;

			for(size_t i = 0; i < 3; i++) {
				for(size_t j = 0; j < 3; j++) {
					V(i,j) += r0[j] * r0[i];
					W(i,j) += r0[j] * r[i];
				}
			}

			numNeighbors++;
		}
	}

	// Special handling for 2D systems.
	if(_simCell.is2D()) {
		// Assume plane strain.
		V(2,2) = W(2,2) = 1;
		V(0,2) = V(1,2) = V(2,0) = V(2,1) = 0;
		W(0,2) = W(1,2) = W(2,0) = W(2,1) = 0;
	}

	// Check if matrix can be inverted.
	Matrix_3<double> inverseV;
	if(numNeighbors < 2 || (!_simCell.is2D() && numNeighbors < 3) || !V.inverse(inverseV, FloatType(1e-4)) || std::abs(W.determinant()) < FloatType(1e-4)) {
		_invalidParticles->setInt(particleIndex, 1);
		if(_deformationGradients) {
			for(Matrix_3<double>::size_type col = 0; col < 3; col++) {
				for(Matrix_3<double>::size_type row = 0; row < 3; row++) {
					_deformationGradients->setFloatComponent(particleIndex, col*3+row, FloatType(0));
				}
			}
		}
		if(_strainTensors)
			_strainTensors->setSymmetricTensor2(particleIndex, SymmetricTensor2::Zero());
        if(_nonaffineSquaredDisplacements)
            _nonaffineSquaredDisplacements->setFloat(particleIndex, 0);
		_shearStrains->setFloat(particleIndex, 0);
		_volumetricStrains->setFloat(particleIndex, 0);
		if(_rotations)
			_rotations->setQuaternion(particleIndex, Quaternion(0,0,0,0));
		if(_stretchTensors)
			_stretchTensors->setSymmetricTensor2(particleIndex, SymmetricTensor2::Zero());
		return false;
	}

	// Calculate deformation gradient tensor.
	Matrix_3<double> F = W * inverseV;
	if(_deformationGradients) {
		for(Matrix_3<double>::size_type col = 0; col < 3; col++) {
			for(Matrix_3<double>::size_type row = 0; row < 3; row++) {
				_deformationGradients->setFloatComponent(particleIndex, col*3+row, (FloatType)F(row,col));
			}
		}
	}

	// Polar decomposition.
	if(rotations() || stretchTensors()) {
		Matrix_3<double> R, U;
		polar_decomposition_3x3(F.elements(), false, R.elements(), U.elements());
		if(rotations()) {
			rotations()->setQuaternion(particleIndex, (Quaternion)QuaternionT<double>(R));
		}
		if(stretchTensors()) {
			stretchTensors()->setSymmetricTensor2(particleIndex,
					SymmetricTensor2(U(0,0), U(1,1), U(2,2), U(0,1), U(0,2), U(1,2)));
		}
	}

	// Calculate strain tensor.
	SymmetricTensor2T<double> strain = (Product_AtA(F) - SymmetricTensor2T<double>::Identity()) * 0.5;
	if(_strainTensors)
		_strainTensors->setSymmetricTensor2(particleIndex, (SymmetricTensor2)strain);

    // Calculate nonaffine displacements.
    if(_nonaffineSquaredDisplacements) {
        double D2min = 0.0;

        // Again iterate over neighbor vectors of central particle.
        numNeighbors = 0;
        const Point3 x = positions()->getPoint3(particleIndex);
        for(CutoffNeighborFinder::Query neighQuery(neighborFinder, particleIndexReference); !neighQuery.atEnd(); neighQuery.next()) {
            const Vector3& r0 = neighQuery.delta();
			int neighborIndexCurrent = refToCurrentIndexMap[neighQuery.current()];
			if(neighborIndexCurrent == -1) continue;
            Vector3 r = positions()->getPoint3(neighborIndexCurrent) - x;
            Vector3 sr = _currentSimCellInv * r;
            if(!_assumeUnwrappedCoordinates) {
                for(size_t k = 0; k < 3; k++) {
                    if(_simCell.pbcFlags()[k])
						sr[k] -= floor(sr[k] + FloatType(0.5));
                }
            }
            r = _reducedToAbsolute * sr;

            Vector_3<double> rDouble(r.x(), r.y(), r.z());
            Vector_3<double> r0Double(r0.x(), r0.y(), r0.z());
            Vector_3<double> dr = rDouble - F * r0Double;
            D2min += dr.squaredLength();
		}

        _nonaffineSquaredDisplacements->setFloat(particleIndex, D2min);
	}

	// Calculate von Mises shear strain.
	double xydiff = strain.xx() - strain.yy();
	double xzdiff = strain.xx() - strain.zz();
	double yzdiff = strain.yy() - strain.zz();
	double shearStrain = sqrt(strain.xy()*strain.xy() + strain.xz()*strain.xz() + strain.yz()*strain.yz() +
			(xydiff*xydiff + xzdiff*xzdiff + yzdiff*yzdiff) / 6.0);
	OVITO_ASSERT(std::isfinite(shearStrain));
	_shearStrains->setFloat(particleIndex, (FloatType)shearStrain);

	// Calculate volumetric component.
	double volumetricStrain = (strain(0,0) + strain(1,1) + strain(2,2)) / 3.0;
	OVITO_ASSERT(std::isfinite(volumetricStrain));
	_volumetricStrains->setFloat(particleIndex, (FloatType)volumetricStrain);

	_invalidParticles->setInt(particleIndex, 0);
	return true;
}

/******************************************************************************
* Unpacks the results of the computation engine and stores them in the modifier.
******************************************************************************/
void AtomicStrainModifier::transferComputationResults(ComputeEngine* engine)
{
	AtomicStrainEngine* eng = static_cast<AtomicStrainEngine*>(engine);
	_shearStrainValues = eng->shearStrains();
	_volumetricStrainValues = eng->volumetricStrains();
	_strainTensors = eng->strainTensors();
	_deformationGradients = eng->deformationGradients();
	_nonaffineSquaredDisplacements = eng->nonaffineSquaredDisplacements();
	_invalidParticles = eng->invalidParticles();
	_rotations = eng->rotations();
	_stretchTensors = eng->stretchTensors();
	_numInvalidParticles = eng->numInvalidParticles();
}

/******************************************************************************
* Lets the modifier insert the cached computation results into the
* modification pipeline.
******************************************************************************/
PipelineStatus AtomicStrainModifier::applyComputationResults(TimePoint time, TimeInterval& validityInterval)
{
	if(!_shearStrainValues || !_volumetricStrainValues)
		throwException(tr("No computation results available."));

	if(outputParticleCount() != _shearStrainValues->size() || outputParticleCount() != _volumetricStrainValues->size())
		throwException(tr("The number of input particles has changed. The stored results have become invalid."));

	if(selectInvalidParticles() && _invalidParticles)
		outputStandardProperty(_invalidParticles.data());

	if(calculateStrainTensors() && _strainTensors)
		outputStandardProperty(_strainTensors.data());

	if(calculateDeformationGradients() && _deformationGradients)
		outputStandardProperty(_deformationGradients.data());

	if(calculateNonaffineSquaredDisplacements() && _nonaffineSquaredDisplacements)
		outputCustomProperty(_nonaffineSquaredDisplacements.data());

	if(_volumetricStrainValues)
		outputCustomProperty(_volumetricStrainValues.data());

	if(_shearStrainValues)
		outputCustomProperty(_shearStrainValues.data());

	if(calculateRotations() && _rotations)
		outputStandardProperty(_rotations.data());

	if(calculateStretchTensors() && _stretchTensors)
		outputStandardProperty(_stretchTensors.data());

	output().attributes().insert(QStringLiteral("AtomicStrain.invalid_particle_count"), QVariant::fromValue(_numInvalidParticles));

	if(invalidParticleCount() == 0)
		return PipelineStatus::Success;
	else
		return PipelineStatus(PipelineStatus::Warning, tr("Failed to compute local deformation for %1 particles. Increase cutoff radius to include more neighbors.").arg(invalidParticleCount()));
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void AtomicStrainModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	AsynchronousParticleModifier::propertyChanged(field);

	// Recompute results when the parameters change.
	if(field == PROPERTY_FIELD(eliminateCellDeformation) ||
			field == PROPERTY_FIELD(assumeUnwrappedCoordinates) ||
			field == PROPERTY_FIELD(cutoff) ||
			field == PROPERTY_FIELD(calculateDeformationGradients) ||
			field == PROPERTY_FIELD(calculateStrainTensors) ||
			field == PROPERTY_FIELD(calculateNonaffineSquaredDisplacements) ||
			field == PROPERTY_FIELD(calculateRotations) ||
			field == PROPERTY_FIELD(calculateStretchTensors) ||
			field == PROPERTY_FIELD(useReferenceFrameOffset) ||
			field == PROPERTY_FIELD(referenceFrameNumber) ||
			field == PROPERTY_FIELD(referenceFrameOffset))
		invalidateCachedResults();
}

/******************************************************************************
* Is called when a RefTarget referenced by this object has generated an event.
******************************************************************************/
bool AtomicStrainModifier::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	// Recompute results when the reference configuration changes.
	if(source == referenceConfiguration()) {
		if(event->type() == ReferenceEvent::TargetChanged || event->type() == ReferenceEvent::PendingStateChanged) {
			invalidateCachedResults();
		}
	}
	return AsynchronousParticleModifier::referenceEvent(source, event);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
