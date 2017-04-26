///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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
#include <core/dataset/importexport/FileSource.h>
#include <core/animation/AnimationSettings.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include "CalculateDisplacementsModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(CalculateDisplacementsModifier, ParticleModifier);
DEFINE_FLAGS_REFERENCE_FIELD(CalculateDisplacementsModifier, referenceConfiguration, "Reference Configuration", DataObject, PROPERTY_FIELD_NO_SUB_ANIM);
DEFINE_PROPERTY_FIELD(CalculateDisplacementsModifier, referenceShown, "ShowReferenceConfiguration");
DEFINE_FLAGS_PROPERTY_FIELD(CalculateDisplacementsModifier, eliminateCellDeformation, "EliminateCellDeformation", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(CalculateDisplacementsModifier, assumeUnwrappedCoordinates, "AssumeUnwrappedCoordinates");
DEFINE_PROPERTY_FIELD(CalculateDisplacementsModifier, useReferenceFrameOffset, "UseReferenceFrameOffet");
DEFINE_PROPERTY_FIELD(CalculateDisplacementsModifier, referenceFrameNumber, "ReferenceFrameNumber");
DEFINE_FLAGS_PROPERTY_FIELD(CalculateDisplacementsModifier, referenceFrameOffset, "ReferenceFrameOffset", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_REFERENCE_FIELD(CalculateDisplacementsModifier, vectorDisplay, "VectorDisplay", VectorDisplay, PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);
SET_PROPERTY_FIELD_LABEL(CalculateDisplacementsModifier, referenceConfiguration, "Reference Configuration");
SET_PROPERTY_FIELD_LABEL(CalculateDisplacementsModifier, referenceShown, "Show reference configuration");
SET_PROPERTY_FIELD_LABEL(CalculateDisplacementsModifier, eliminateCellDeformation, "Eliminate homogeneous cell deformation");
SET_PROPERTY_FIELD_LABEL(CalculateDisplacementsModifier, assumeUnwrappedCoordinates, "Assume unwrapped coordinates");
SET_PROPERTY_FIELD_LABEL(CalculateDisplacementsModifier, useReferenceFrameOffset, "Use reference frame offset");
SET_PROPERTY_FIELD_LABEL(CalculateDisplacementsModifier, referenceFrameNumber, "Reference frame number");
SET_PROPERTY_FIELD_LABEL(CalculateDisplacementsModifier, referenceFrameOffset, "Reference frame offset");
SET_PROPERTY_FIELD_LABEL(CalculateDisplacementsModifier, vectorDisplay, "Vector display");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(CalculateDisplacementsModifier, referenceFrameNumber, IntegerParameterUnit, 0);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
CalculateDisplacementsModifier::CalculateDisplacementsModifier(DataSet* dataset) : ParticleModifier(dataset),
    _referenceShown(false), _eliminateCellDeformation(false),
    _useReferenceFrameOffset(false), _referenceFrameNumber(0), _referenceFrameOffset(-1),
    _assumeUnwrappedCoordinates(false)
{
	INIT_PROPERTY_FIELD(referenceConfiguration);
	INIT_PROPERTY_FIELD(referenceShown);
	INIT_PROPERTY_FIELD(eliminateCellDeformation);
	INIT_PROPERTY_FIELD(assumeUnwrappedCoordinates);
	INIT_PROPERTY_FIELD(useReferenceFrameOffset);
	INIT_PROPERTY_FIELD(referenceFrameNumber);
	INIT_PROPERTY_FIELD(referenceFrameOffset);
	INIT_PROPERTY_FIELD(vectorDisplay);

	// Create the file source object, which will be responsible for loading
	// and storing the reference configuration.
	OORef<FileSource> linkedFileObj(new FileSource(dataset));

	// Disable automatic adjustment of animation length for the reference object.
	// We don't want the scene's animation interval to be affected by an animation
	// loaded into the reference configuration object.
	linkedFileObj->setAdjustAnimationIntervalEnabled(false);
	setReferenceConfiguration(linkedFileObj);

	// Create display object for vectors.
	setVectorDisplay(new VectorDisplay(dataset));
	vectorDisplay()->setObjectTitle(tr("Displacements"));

	// Don't show vectors by default, because too many vectors can make the
	// program freeze. User has to enable the display manually.
	vectorDisplay()->setEnabled(false);

	// Configure vector display such that arrows point from the reference particle positions
	// to the current particle positions.
	vectorDisplay()->setReverseArrowDirection(false);
	vectorDisplay()->setArrowPosition(VectorDisplay::Head);
}
/******************************************************************************
* Handles reference events sent by reference targets of this object.
******************************************************************************/
bool CalculateDisplacementsModifier::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	// Do not propagate messages sent by the attached display object.
	if(source == vectorDisplay())
		return false;

	return ParticleModifier::referenceEvent(source, event);
}

/******************************************************************************
* This modifies the input object.
******************************************************************************/
PipelineStatus CalculateDisplacementsModifier::modifyParticles(TimePoint time, TimeInterval& validityInterval)
{
	// Get the reference positions of the particles.
	if(!referenceConfiguration())
		throwException(tr("Cannot calculate displacement vectors. Reference configuration has not been specified."));

	// What is the reference frame number to use?
	int referenceFrame;
	if(useReferenceFrameOffset()) {
		// Determine the current frame, preferably from the attributes stored with the pipeline flow state.
		// If the "SourceFrame" attribute is not present, infer it from the current animation time.
		int currentFrame = input().attributes().value(QStringLiteral("SourceFrame"),
				dataset()->animationSettings()->timeToFrame(time)).toInt();

		// Use frame offset relative to current configuration.
		referenceFrame = currentFrame + referenceFrameOffset();

		// Results are only valid for current frame.
		validityInterval.intersect(time);
	}
	else {
		// Always use the same, user-specified frame as reference configuration.
		referenceFrame = referenceFrameNumber();
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
		return refState.status();
	if(refState.isEmpty()) {
		if(refState.status().type() != PipelineStatus::Pending)
			throwException(tr("Reference configuration has not been specified yet or is empty. Please pick a reference simulation file."));
		else
			return PipelineStatus(PipelineStatus::Pending, tr("Waiting for input data to become ready..."));
	}
	// Make sure we really got back the requested reference frame.
	if(refState.attributes().value(QStringLiteral("SourceFrame"), referenceFrame).toInt() != referenceFrame)
		throwException(tr("Requested reference frame %1 is out of range.").arg(referenceFrame));

	// Get the reference positions.
	ParticlePropertyObject* refPosProperty = ParticlePropertyObject::findInState(refState, ParticleProperty::PositionProperty);
	if(!refPosProperty)
		throwException(tr("Reference configuration does not contain any particle positions."));

	// Get the current positions.
	ParticlePropertyObject* posProperty = expectStandardProperty(ParticleProperty::PositionProperty);

	// Build particle-to-particle index map.
	std::vector<size_t> indexToIndexMap(inputParticleCount());
	ParticlePropertyObject* identifierProperty = inputStandardProperty(ParticleProperty::IdentifierProperty);
	ParticlePropertyObject* refIdentifierProperty = ParticlePropertyObject::findInState(refState, ParticleProperty::IdentifierProperty);;
	if(identifierProperty && refIdentifierProperty) {

		// Build map of particle identifiers in reference configuration.
		std::map<int, size_t> refMap;
		size_t index = 0;
		const int* id = refIdentifierProperty->constDataInt();
		const int* id_end = id + refIdentifierProperty->size();
		for(; id != id_end; ++id, ++index) {
			if(refMap.insert(std::make_pair(*id, index)).second == false)
				throwException(tr("Particles with duplicate identifiers detected in reference configuration."));
		}

		// Check for duplicate identifiers in current configuration.
		std::vector<size_t> idSet(identifierProperty->constDataInt(), identifierProperty->constDataInt() + identifierProperty->size());
		std::sort(idSet.begin(), idSet.end());
		if(std::adjacent_find(idSet.begin(), idSet.end()) != idSet.end())
			throwException(tr("Particles with duplicate identifiers detected in input configuration."));

		// Build index map.
		index = 0;
		id = identifierProperty->constDataInt();
		for(auto& mappedIndex : indexToIndexMap) {
			auto iter = refMap.find(*id);
			if(iter == refMap.end())
				throwException(tr("Particle id %1 from current configuration not found in reference configuration.").arg(*id));
			mappedIndex = iter->second;
			index++;
			++id;
		}
	}
	else {
		// Deformed and reference configuration must contain the same number of particles.
		if(posProperty->size() != refPosProperty->size()) {
			if(refState.status().type() != PipelineStatus::Pending)
				throwException(tr("Cannot calculate displacement vectors. Numbers of particles in reference configuration and current configuration do not match."));
			else
				return PipelineStatus(PipelineStatus::Pending, tr("Waiting for input data to become ready..."));
		}
		// When particle identifiers are not available, use trivial 1-to-1 mapping.
		std::iota(indexToIndexMap.begin(), indexToIndexMap.end(), size_t(0));
	}

	// Get simulation cells.
	SimulationCellObject* inputCell = expectSimulationCell();
	SimulationCellObject* refCell = refState.findObject<SimulationCellObject>();
	if(!refCell)
		throwException(tr("Reference configuration does not contain simulation cell info."));

#if 0
	// If enabled, feed particle positions from reference configuration into geometry pipeline.
	ParticlePropertyObject* outputPosProperty = nullptr;
	if(referenceShown()) {
		outputPosProperty = outputStandardProperty(ParticleProperty::PositionProperty);
		OVITO_ASSERT(outputPosProperty->size() == refPosProperty->size());
		outputPosProperty->setStorage(refPosProperty->storage());
		outputSimulationCell()->setCellMatrix(refCell->cellMatrix());
	}
#endif

	// Create the displacement property.
	ParticlePropertyObject* displacementProperty = outputStandardProperty(ParticleProperty::DisplacementProperty);
	ParticlePropertyObject* displacementMagnitudeProperty = outputStandardProperty(ParticleProperty::DisplacementMagnitudeProperty);
	OVITO_ASSERT(displacementProperty->size() == posProperty->size());
	OVITO_ASSERT(displacementMagnitudeProperty->size() == posProperty->size());

	// Plug in our internal display object.
	displacementProperty->setDisplayObject(_vectorDisplay);

	// Get simulation cell info.
	const std::array<bool, 3> pbc = inputCell->pbcFlags();
	AffineTransformation simCell;
	AffineTransformation simCellRef;
	if(_referenceShown) {
		simCellRef = inputCell->cellMatrix();
		simCell = refCell->cellMatrix();
	}
	else {
		simCellRef = refCell->cellMatrix();
		simCell = inputCell->cellMatrix();
	}

	// Compute inverse cell transformation.
	AffineTransformation simCellInv;
	AffineTransformation simCellRefInv;
	if(eliminateCellDeformation()) {
		if(std::abs(simCell.determinant()) < FLOATTYPE_EPSILON || std::abs(simCellRef.determinant()) < FLOATTYPE_EPSILON)
			throwException(tr("Simulation cell is degenerate in either the deformed or the reference configuration."));

		simCellInv = simCell.inverse();
		simCellRefInv = simCellRef.inverse();
	}

	// Compute displacement vectors.
	const bool unwrap = !assumeUnwrappedCoordinates();
	const Point3* u0 = refPosProperty->constDataPoint3();
	const Point3* u_begin = posProperty->constDataPoint3();
	Vector3* d_begin = displacementProperty->dataVector3();
	FloatType* dmag_begin = displacementMagnitudeProperty->dataFloat();
	auto index_begin = indexToIndexMap.begin();
	if(eliminateCellDeformation()) {
		parallelForChunks(displacementProperty->size(), [d_begin, dmag_begin, u_begin, index_begin, u0, unwrap, pbc, simCellRef, simCellInv, simCellRefInv] (size_t startIndex, size_t count) {
			Vector3* d = d_begin + startIndex;
			FloatType* dmag = dmag_begin + startIndex;
			const Point3* u = u_begin + startIndex;
			auto index = index_begin + startIndex;
			for(; count; --count, ++d, ++dmag, ++u, ++index) {
				Point3 ru = simCellInv * (*u);
				Point3 ru0 = simCellRefInv * u0[*index];
				Vector3 delta = ru - ru0;
				if(unwrap) {
					for(int k = 0; k < 3; k++) {
						if(!pbc[k]) continue;
						if(delta[k] > 0.5) delta[k] -= 1.0;
						else if(delta[k] < -0.5) delta[k] += 1.0;
					}
				}
				*d = simCellRef * delta;
				*dmag = d->length();
			}
		});
	}
	else {
		parallelForChunks(displacementProperty->size(), [d_begin, dmag_begin, u_begin, index_begin, u0, unwrap, pbc, simCellRef, simCellInv, simCellRefInv] (size_t startIndex, size_t count) {
			Vector3* d = d_begin + startIndex;
			FloatType* dmag = dmag_begin + startIndex;
			const Point3* u = u_begin + startIndex;
			auto index = index_begin + startIndex;
			for(; count; --count, ++d, ++dmag, ++u, ++index) {
				*d = *u - u0[*index];
				if(unwrap) {
					for(int k = 0; k < 3; k++) {
						if(!pbc[k]) continue;
						if((*d + simCellRef.column(k)).squaredLength() < d->squaredLength())
							*d += simCellRef.column(k);
						else if((*d - simCellRef.column(k)).squaredLength() < d->squaredLength())
							*d -= simCellRef.column(k);
					}
				}
				*dmag = d->length();
			}
		});
	}
	if(_referenceShown) {
		// Flip all displacement vectors.
		std::for_each(displacementProperty->dataVector3(), displacementProperty->dataVector3() + displacementProperty->size(), [](Vector3& d) { d = -d; });
	}
	displacementProperty->changed();

	return refState.status().type();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
