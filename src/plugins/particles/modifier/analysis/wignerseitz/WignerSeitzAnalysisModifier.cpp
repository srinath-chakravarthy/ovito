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
#include <plugins/particles/util/NearestNeighborFinder.h>
#include <plugins/particles/objects/ParticleTypeProperty.h>
#include <core/dataset/importexport/FileSource.h>
#include <core/animation/AnimationSettings.h>
#include <core/scene/pipeline/PipelineEvalRequest.h>
#include "WignerSeitzAnalysisModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(WignerSeitzAnalysisModifier, AsynchronousParticleModifier);
DEFINE_REFERENCE_FIELD(WignerSeitzAnalysisModifier, referenceConfiguration, "Reference Configuration", DataObject);
DEFINE_FLAGS_PROPERTY_FIELD(WignerSeitzAnalysisModifier, eliminateCellDeformation, "EliminateCellDeformation", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(WignerSeitzAnalysisModifier, useReferenceFrameOffset, "UseReferenceFrameOffet");
DEFINE_PROPERTY_FIELD(WignerSeitzAnalysisModifier, referenceFrameNumber, "ReferenceFrameNumber");
DEFINE_FLAGS_PROPERTY_FIELD(WignerSeitzAnalysisModifier, referenceFrameOffset, "ReferenceFrameOffset", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(WignerSeitzAnalysisModifier, perTypeOccupancy, "PerTypeOccupancy", PROPERTY_FIELD_MEMORIZE);
SET_PROPERTY_FIELD_LABEL(WignerSeitzAnalysisModifier, referenceConfiguration, "Reference Configuration");
SET_PROPERTY_FIELD_LABEL(WignerSeitzAnalysisModifier, eliminateCellDeformation, "Eliminate homogeneous cell deformation");
SET_PROPERTY_FIELD_LABEL(WignerSeitzAnalysisModifier, useReferenceFrameOffset, "Use reference frame offset");
SET_PROPERTY_FIELD_LABEL(WignerSeitzAnalysisModifier, referenceFrameNumber, "Reference frame number");
SET_PROPERTY_FIELD_LABEL(WignerSeitzAnalysisModifier, referenceFrameOffset, "Reference frame offset");
SET_PROPERTY_FIELD_LABEL(WignerSeitzAnalysisModifier, perTypeOccupancy, "Output per-type occupancies");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(WignerSeitzAnalysisModifier, referenceFrameNumber, IntegerParameterUnit, 0);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
WignerSeitzAnalysisModifier::WignerSeitzAnalysisModifier(DataSet* dataset) : AsynchronousParticleModifier(dataset),
	_eliminateCellDeformation(false),
	_useReferenceFrameOffset(false), _referenceFrameNumber(0), _referenceFrameOffset(-1),
	_vacancyCount(0), _interstitialCount(0),
	_perTypeOccupancy(false)
{
	INIT_PROPERTY_FIELD(referenceConfiguration);
	INIT_PROPERTY_FIELD(eliminateCellDeformation);
	INIT_PROPERTY_FIELD(useReferenceFrameOffset);
	INIT_PROPERTY_FIELD(referenceFrameNumber);
	INIT_PROPERTY_FIELD(referenceFrameOffset);
	INIT_PROPERTY_FIELD(perTypeOccupancy);

	// Create the file source object that will be responsible for loading
	// and storing the reference configuration.
	OORef<FileSource> linkedFileObj(new FileSource(dataset));

	// Disable the automatic adjustment of the animation length.
	// We don't want the scene's animation interval to be affected by an animation
	// loaded into the reference configuration object.
	linkedFileObj->setAdjustAnimationIntervalEnabled(false);
	setReferenceConfiguration(linkedFileObj);
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
std::shared_ptr<AsynchronousParticleModifier::ComputeEngine> WignerSeitzAnalysisModifier::createEngine(TimePoint time, TimeInterval validityInterval)
{
	// Get the current positions.
	ParticlePropertyObject* posProperty = expectStandardProperty(ParticleProperty::PositionProperty);

	// Get the reference configuration.
	PipelineFlowState refState = getReferenceState(time);
	if(refState.isEmpty())
		throwException(tr("Reference configuration has not been specified yet or is empty. Please pick a reference simulation file."));

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
	if(inputCell->volume3D() < FLOATTYPE_EPSILON)
		throwException(tr("Simulation cell is degenerate in the deformed configuration."));
	if(refCell->volume3D() < FLOATTYPE_EPSILON)
		throwException(tr("Simulation cell is degenerate in the reference configuration."));

	// Get the particle types.
	ParticleProperty* typeProperty = nullptr;
	int ptypeMinId = std::numeric_limits<int>::max();
	int ptypeMaxId = std::numeric_limits<int>::lowest();
	if(perTypeOccupancy()) {
		ParticleTypeProperty* ptypeProp = static_object_cast<ParticleTypeProperty>(expectStandardProperty(ParticleProperty::ParticleTypeProperty));
		// Determine range of particle type IDs.
		for(ParticleType* pt : ptypeProp->particleTypes()) {
			if(pt->id() < ptypeMinId) ptypeMinId = pt->id();
			if(pt->id() > ptypeMaxId) ptypeMaxId = pt->id();
		}
		typeProperty = ptypeProp->storage();
	}

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<WignerSeitzAnalysisEngine>(validityInterval, posProperty->storage(), inputCell->data(),
			refPosProperty->storage(), refCell->data(), eliminateCellDeformation(), typeProperty, ptypeMinId, ptypeMaxId);
}

/******************************************************************************
* Returns the reference state to be used to perform the analysis at the given time.
******************************************************************************/
PipelineFlowState WignerSeitzAnalysisModifier::getReferenceState(TimePoint time)
{
	// Get the reference positions of particles.
	if(!referenceConfiguration())
		throwException(tr("Cannot perform analysis without a reference configuration."));

	// What is the reference frame number to use?
	int referenceFrame;
	if(useReferenceFrameOffset()) {
		// Determine the current frame, preferably from the "SourceFrame" attribute stored with the pipeline flow state.
		// If the "SourceFrame" attribute is not present, infer it from the current animation time.
		int currentFrame = input().attributes().value(QStringLiteral("SourceFrame"),
				dataset()->animationSettings()->timeToFrame(time)).toInt();

		// Use frame offset relative to current configuration.
		referenceFrame = currentFrame + referenceFrameOffset();
	}
	else {
		// Always use the same, user-specified frame as reference configuration.
		referenceFrame = referenceFrameNumber();
	}

	// Get the reference configuration.
	PipelineFlowState refState;
	if(FileSource* linkedFileObj = dynamic_object_cast<FileSource>(referenceConfiguration())) {
		if(linkedFileObj->numberOfFrames() > 0) {
			if(referenceFrame < 0 || referenceFrame >= linkedFileObj->numberOfFrames())
				throwException(tr("Requested reference frame %1 is out of range.").arg(referenceFrame));
			refState = linkedFileObj->requestFrame(referenceFrame);
		}
	}
	else refState = referenceConfiguration()->evaluateImmediately(PipelineEvalRequest(dataset()->animationSettings()->frameToTime(referenceFrame), false));

	// Make sure the obtained reference configuration is valid and ready to use.
	if(refState.status().type() == PipelineStatus::Error)
		throw refState.status();
	if(refState.status().type() == PipelineStatus::Pending)
		throw PipelineStatus(PipelineStatus::Pending, tr("Waiting for input data to become ready..."));

	// Make sure we really received the requested reference frame.
	if(refState.attributes().value(QStringLiteral("SourceFrame"), referenceFrame).toInt() != referenceFrame)
		throwException(tr("Requested reference frame %1 is out of range.").arg(referenceFrame));

	return refState;
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void WignerSeitzAnalysisModifier::WignerSeitzAnalysisEngine::perform()
{
	setProgressText(tr("Performing Wigner-Seitz cell analysis"));

	size_t particleCount = positions()->size();
	if(refPositions()->size() == 0)
		throw Exception(tr("Reference configuration for WS analysis contains no sites."));

	// PBCs flags of the current configuration always override PBCs flags
	// of the reference config.
	_simCellRef.setPbcFlags(_simCell.pbcFlags());

	// Prepare the closest-point query structure.
	NearestNeighborFinder neighborTree(0);
	if(!neighborTree.prepare(refPositions(), refCell(), nullptr, *this))
		return;

	// Determine the number of components of the occupancy property.
	int ncomponents = 1;
	int typemin, typemax;
	if(particleTypes()) {
		auto minmax = std::minmax_element(particleTypes()->constDataInt(), particleTypes()->constDataInt() + particleTypes()->size());
		typemin = std::min(_ptypeMinId, *minmax.first);
		typemax = std::max(_ptypeMaxId, *minmax.second);
		if(typemin < 0)
			throw Exception(tr("Negative particle types are not supported by this modifier."));
		if(typemax > 32)
			throw Exception(tr("Number of particle types is too large for this modifier. Cannot compute occupancy numbers for more than 32 particle types."));
		ncomponents = typemax - typemin + 1;
	}

	// Create output storage.
	_occupancyNumbers = new ParticleProperty(refPositions()->size(), qMetaTypeId<int>(), ncomponents, 0, tr("Occupancy"), true);
	if(ncomponents > 1 && typemin != 1) {
		QStringList componentNames;
		for(int i = typemin; i <= typemax; i++)
			componentNames.push_back(QString::number(i));
		occupancyNumbers()->setComponentNames(componentNames);
	}

	AffineTransformation tm;
	if(_eliminateCellDeformation)
		tm = refCell().matrix() * cell().inverseMatrix();

	// Assign particles to reference sites.
	FloatType closestDistanceSq;
	int particleIndex = 0;
	setProgressMaximum(particleCount);
	for(const Point3& p : positions()->constPoint3Range()) {

		int closestIndex = neighborTree.findClosestParticle(_eliminateCellDeformation ? (tm * p) : p, closestDistanceSq);
		OVITO_ASSERT(closestIndex >= 0 && closestIndex < occupancyNumbers()->size());
		if(ncomponents == 1) {
			occupancyNumbers()->dataInt()[closestIndex]++;
		}
		else {
			int offset = particleTypes()->getInt(particleIndex) - typemin;
			OVITO_ASSERT(offset >= 0 && offset < occupancyNumbers()->componentCount());
			occupancyNumbers()->dataInt()[closestIndex * ncomponents + offset]++;
		}

		particleIndex++;
		if(!setProgressValueIntermittent(particleIndex))
			return;
	}

	// Count defects.
	_vacancyCount = 0;
	_interstitialCount = 0;
	if(ncomponents == 1) {
		for(int oc : occupancyNumbers()->constIntRange()) {
			if(oc == 0) _vacancyCount++;
			else if(oc > 1) _interstitialCount += oc - 1;
		}
	}
	else {
		const int* o = occupancyNumbers()->constDataInt();
		for(size_t i = 0; i < refPositions()->size(); i++) {
			int oc = 0;
			for(int j = 0; j < ncomponents; j++) {
				oc += *o++;
			}
			if(oc == 0) _vacancyCount++;
			else if(oc > 1) _interstitialCount += oc - 1;
		}
	}
}

/******************************************************************************
* Unpacks the results of the computation engine and stores them in the modifier.
******************************************************************************/
void WignerSeitzAnalysisModifier::transferComputationResults(ComputeEngine* engine)
{
	WignerSeitzAnalysisEngine* eng = static_cast<WignerSeitzAnalysisEngine*>(engine);
	_occupancyNumbers = eng->occupancyNumbers();
	_vacancyCount = eng->vacancyCount();
	_interstitialCount = eng->interstitialCount();
}

/******************************************************************************
* Lets the modifier insert the cached computation results into the
* modification pipeline.
******************************************************************************/
PipelineStatus WignerSeitzAnalysisModifier::applyComputationResults(TimePoint time, TimeInterval& validityInterval)
{
	if(!_occupancyNumbers)
		throwException(tr("No computation results available."));

	PipelineFlowState refState = getReferenceState(time);

	QVariantMap oldAttributes = output().attributes();
	TimeInterval oldValidity = output().stateValidity();

	// Replace pipeline contents with reference configuration.
	output() = refState;
	output().setStateValidity(oldValidity);
	output().attributes() = oldAttributes;

	ParticlePropertyObject* posProperty = ParticlePropertyObject::findInState(output(), ParticleProperty::PositionProperty);
	if(!posProperty)
		throwException(tr("This modifier cannot be evaluated, because the reference configuration does not contain any particles."));
	_outputParticleCount = posProperty->size();

	if(posProperty->size() != _occupancyNumbers->size())
		throwException(tr("The number of particles in the reference configuration has changed. The stored results have become invalid."));

	outputCustomProperty(_occupancyNumbers.data());

	output().attributes().insert(QStringLiteral("WignerSeitz.vacancy_count"), QVariant::fromValue(vacancyCount()));
	output().attributes().insert(QStringLiteral("WignerSeitz.interstitial_count"), QVariant::fromValue(interstitialCount()));

	return PipelineStatus(PipelineStatus::Success, tr("Found %1 vacancies and %2 interstitials").arg(vacancyCount()).arg(interstitialCount()));
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void WignerSeitzAnalysisModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	AsynchronousParticleModifier::propertyChanged(field);

	// Recompute modifier results when the parameters have changed.
	if(field == PROPERTY_FIELD(eliminateCellDeformation)
			|| field == PROPERTY_FIELD(perTypeOccupancy)
			|| field == PROPERTY_FIELD(useReferenceFrameOffset)
			|| field == PROPERTY_FIELD(referenceFrameNumber)
			|| field == PROPERTY_FIELD(referenceFrameOffset))
		invalidateCachedResults();
}

/******************************************************************************
* Is called when a RefTarget referenced by this object has generated an event.
******************************************************************************/
bool WignerSeitzAnalysisModifier::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
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
