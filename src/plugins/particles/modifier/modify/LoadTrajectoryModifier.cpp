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

#include <plugins/particles/Particles.h>
#include <core/scene/objects/DataObject.h>
#include <core/scene/pipeline/PipelineEvalRequest.h>
#include <core/dataset/importexport/FileSource.h>
#include <core/animation/AnimationSettings.h>
#include <plugins/particles/objects/BondsObject.h>
#include "LoadTrajectoryModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(LoadTrajectoryModifier, ParticleModifier);
DEFINE_FLAGS_REFERENCE_FIELD(LoadTrajectoryModifier, trajectorySource, "TrajectorySource", DataObject, PROPERTY_FIELD_NO_SUB_ANIM);
SET_PROPERTY_FIELD_LABEL(LoadTrajectoryModifier, trajectorySource, "Trajectory source");

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
LoadTrajectoryModifier::LoadTrajectoryModifier(DataSet* dataset) : ParticleModifier(dataset)
{
	INIT_PROPERTY_FIELD(trajectorySource);

	// Create the file source object, which will be responsible for loading
	// and caching the trajectory data.
	OORef<FileSource> fileSource(new FileSource(dataset));

	// Enable automatic adjustment of animation length for the trajectory source object.
	fileSource->setAdjustAnimationIntervalEnabled(true);

	setTrajectorySource(fileSource);
}

/******************************************************************************
* This modifies the input object.
******************************************************************************/
PipelineStatus LoadTrajectoryModifier::modifyParticles(TimePoint time, TimeInterval& validityInterval)
{
	// Get the trajectory data.
	if(!trajectorySource())
		throwException(tr("No trajectory data has been provided."));

	// Get the trajectory frame.
	PipelineFlowState trajState = trajectorySource()->evaluateImmediately(PipelineEvalRequest(time, false));

	// Make sure the obtained configuration is valid and ready to use.
	if(trajState.status().type() == PipelineStatus::Error) {
		if(FileSource* fileSource = dynamic_object_cast<FileSource>(trajectorySource())) {
			if(fileSource->sourceUrl().isEmpty())
				throwException(tr("Please pick the input file containing the trajectories."));
		}
		return trajState.status();
	}

	if(trajState.isEmpty()) {
		if(trajState.status().type() != PipelineStatus::Pending)
			throwException(tr("Data source has not been specified yet or is empty. Please pick a trajectory file."));
		else
			return PipelineStatus(PipelineStatus::Pending, tr("Waiting for input data to become ready..."));
	}

	// Merge validity intervals of topology and trajectory datasets.
	validityInterval.intersect(trajState.stateValidity());

	// Merge attributes of topology and trajectory datasets.
	for(auto a = trajState.attributes().cbegin(); a != trajState.attributes().cend(); ++a)
		output().attributes().insert(a.key(), a.value());

	// Get the current particle positions.
	ParticlePropertyObject* trajectoryPosProperty = ParticlePropertyObject::findInState(trajState, ParticleProperty::PositionProperty);
	if(!trajectoryPosProperty)
		throwException(tr("Trajectory dataset does not contain any particle positions."));

	// Get the positions from the topology dataset.
	ParticlePropertyObject* posProperty = expectStandardProperty(ParticleProperty::PositionProperty);

	// Build particle-to-particle index map.
	std::vector<size_t> indexToIndexMap(inputParticleCount());
	ParticlePropertyObject* identifierProperty = inputStandardProperty(ParticleProperty::IdentifierProperty);
	ParticlePropertyObject* trajIdentifierProperty = ParticlePropertyObject::findInState(trajState, ParticleProperty::IdentifierProperty);;
	if(identifierProperty && trajIdentifierProperty) {

		// Build map of particle identifiers in trajectory dataset.
		std::map<int, size_t> refMap;
		size_t index = 0;
		const int* id = trajIdentifierProperty->constDataInt();
		const int* id_end = id + trajIdentifierProperty->size();
		for(; id != id_end; ++id, ++index) {
			if(refMap.insert(std::make_pair(*id, index)).second == false)
				throwException(tr("Particles with duplicate identifiers detected in trajectory data."));
		}

		// Check for duplicate identifiers in topology dataset.
		std::vector<size_t> idSet(identifierProperty->constDataInt(), identifierProperty->constDataInt() + identifierProperty->size());
		std::sort(idSet.begin(), idSet.end());
		if(std::adjacent_find(idSet.begin(), idSet.end()) != idSet.end())
			throwException(tr("Particles with duplicate identifiers detected in topology dataset."));

		// Build index map.
		index = 0;
		id = identifierProperty->constDataInt();
		for(auto& mappedIndex : indexToIndexMap) {
			auto iter = refMap.find(*id);
			if(iter == refMap.end())
				throwException(tr("Particle id %1 from topology dataset not found in trajectory dataset.").arg(*id));
			mappedIndex = iter->second;
			index++;
			++id;
		}
	}
	else {
		// Topology dataset and trajectory data must contain the same number of particles.
		if(posProperty->size() != trajectoryPosProperty->size()) {
			if(trajState.status().type() != PipelineStatus::Pending)
				throwException(tr("Cannot apply trajectories to current particle dataset. Number of particles in the trajectory file does not match."));
			else
				return PipelineStatus(PipelineStatus::Pending, tr("Waiting for input data to become ready..."));
		}

		// When particle identifiers are not available, use trivial 1-to-1 mapping.
		std::iota(indexToIndexMap.begin(), indexToIndexMap.end(), size_t(0));
	}

	// Transfer particle positions.
	ParticlePropertyObject* outputPosProperty = outputStandardProperty(ParticleProperty::PositionProperty);
	for(size_t i = 0; i < inputParticleCount(); i++) {
		outputPosProperty->setPoint3(i, trajectoryPosProperty->getPoint3(indexToIndexMap[i]));
	}
	outputPosProperty->changed();

	// Transfer box geometry.
	SimulationCellObject* topologyCell = input().findObject<SimulationCellObject>();
	SimulationCellObject* trajectoryCell = trajState.findObject<SimulationCellObject>();
	if(topologyCell && trajectoryCell) {
		SimulationCellObject* outputCell = outputSimulationCell();
		outputCell->setCellMatrix(trajectoryCell->cellMatrix());
		AffineTransformation simCell = trajectoryCell->cellMatrix();

		// Trajectories of atoms may cross periodic boundaries and if atomic positions are
		// stored in wrapped coordinates, then it becomes necessary to fix bonds using the minimum image convention.
		std::array<bool, 3> pbc = topologyCell->pbcFlags();
		if((pbc[0] || pbc[1] || pbc[2]) && std::abs(simCell.determinant()) > FLOATTYPE_EPSILON) {
			AffineTransformation inverseSimCell = simCell.inverse();

			for(DataObject* obj : output().objects()) {
				if(BondsObject* bondsObj = dynamic_object_cast<BondsObject>(obj)) {
					// Is the object still a shallow copy of the input?
					if(input().contains(bondsObj)) {
						// If yes, make a real copy of the object, which may be modified.
						OORef<BondsObject> newObject = cloneHelper()->cloneObject(bondsObj, false);
						output().replaceObject(bondsObj, newObject);
						bondsObj = newObject;
					}

					// Wrap bonds crossing a periodic boundary by resetting their PBC shift vectors.
					for(Bond& bond : *bondsObj->modifiableStorage()) {
						const Point3& p1 = outputPosProperty->getPoint3(bond.index1);
						const Point3& p2 = outputPosProperty->getPoint3(bond.index2);
						for(size_t dim = 0; dim < 3; dim++) {
							if(pbc[dim]) {
								bond.pbcShift[dim] = (int8_t)floor(inverseSimCell.prodrow(p1 - p2, dim) + FloatType(0.5));
							}
						}
					}
					bondsObj->changed();
				}
			}
		}
	}

	return trajState.status().type();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
