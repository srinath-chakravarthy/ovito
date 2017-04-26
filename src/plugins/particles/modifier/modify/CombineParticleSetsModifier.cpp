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
#include <core/dataset/importexport/FileSource.h>
#include <plugins/particles/objects/BondsObject.h>
#include <plugins/particles/objects/BondsDisplay.h>
#include <core/animation/AnimationSettings.h>
#include "CombineParticleSetsModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(CombineParticleSetsModifier, ParticleModifier);
DEFINE_FLAGS_REFERENCE_FIELD(CombineParticleSetsModifier, secondaryDataSource, "SecondarySource", DataObject, PROPERTY_FIELD_NO_SUB_ANIM);
SET_PROPERTY_FIELD_LABEL(CombineParticleSetsModifier, secondaryDataSource, "Secondary source");

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
CombineParticleSetsModifier::CombineParticleSetsModifier(DataSet* dataset) : ParticleModifier(dataset)
{
	INIT_PROPERTY_FIELD(secondaryDataSource);

	// Create the file source object, which will be responsible for loading
	// and caching the data to be merged.
	OORef<FileSource> fileSource(new FileSource(dataset));

	// Disable automatic adjustment of animation length for the source object.
	fileSource->setAdjustAnimationIntervalEnabled(false);

	setSecondaryDataSource(fileSource);
}

/******************************************************************************
* This modifies the input object.
******************************************************************************/
PipelineStatus CombineParticleSetsModifier::modifyParticles(TimePoint time, TimeInterval& validityInterval)
{
	// Get the trajectory data.
	if(!secondaryDataSource())
		throwException(tr("No particle data to be merged has been provided."));

	// Get the data to be merged into the pipeline.
	PipelineFlowState secondaryState = secondaryDataSource()->evaluateImmediately(PipelineEvalRequest(time, false));

	// Make sure the obtained dataset is valid and ready to use.
	if(secondaryState.status().type() == PipelineStatus::Error) {
		if(FileSource* fileSource = dynamic_object_cast<FileSource>(secondaryDataSource())) {
			if(fileSource->sourceUrl().isEmpty())
				throwException(tr("Please pick an input file to be merged."));
		}
		return secondaryState.status();
	}

	if(secondaryState.isEmpty()) {
		if(secondaryState.status().type() != PipelineStatus::Pending)
			throwException(tr("Secondary data source has not been specified yet or is empty. Please pick an input file to be merged."));
		else
			return PipelineStatus(PipelineStatus::Pending, tr("Waiting for input data to become ready..."));
	}

	// Merge validity intervals of primary and secondary datasets.
	validityInterval.intersect(secondaryState.stateValidity());

	// Merge attributes of primary and secondary dataset.
	for(auto a = secondaryState.attributes().cbegin(); a != secondaryState.attributes().cend(); ++a)
		output().attributes().insert(a.key(), a.value());

	// Get the particle positions of secondary set.
	ParticlePropertyObject* secondaryPosProperty = ParticlePropertyObject::findInState(secondaryState, ParticleProperty::PositionProperty);
	if(!secondaryPosProperty)
		throwException(tr("Second dataset does not contain any particles."));

	// Get the positions from the primary dataset.
	ParticlePropertyObject* posProperty = expectStandardProperty(ParticleProperty::PositionProperty);

	size_t primaryCount = posProperty->size();
	size_t secondaryCount = secondaryPosProperty->size();
	size_t finalCount = primaryCount + secondaryCount;

	// Extend all existing property arrays and copy data from secondary set if present.
	if(secondaryCount != 0) {
		for(DataObject* obj : input().objects()) {
			if(OORef<ParticlePropertyObject> prop = dynamic_object_cast<ParticlePropertyObject>(obj)) {
				if(prop->size() == primaryCount) {
					OORef<ParticlePropertyObject> newProperty = cloneHelper()->cloneObject(prop, false);
					newProperty->resize(finalCount, true);

					// Find corresponding property in second dataset.
					ParticlePropertyObject* secondProp;
					if(prop->type() != ParticleProperty::UserProperty)
						secondProp = ParticlePropertyObject::findInState(secondaryState, prop->type());
					else
						secondProp = ParticlePropertyObject::findInState(secondaryState, prop->name());
					if(secondProp && secondProp->size() == secondaryCount && secondProp->componentCount() == prop->componentCount() && secondProp->dataType() == prop->dataType()) {
						OVITO_ASSERT(newProperty->stride() == secondProp->stride());
						memcpy(static_cast<char*>(newProperty->data()) + newProperty->stride() * primaryCount, secondProp->constData(), newProperty->stride() * secondaryCount);
					}

					// Assign unique IDs.
					if(newProperty->type() == ParticleProperty::IdentifierProperty && primaryCount != 0) {
						int maxId = *std::max_element(newProperty->constDataInt(), newProperty->constDataInt() + primaryCount);
						std::iota(newProperty->dataInt() + primaryCount, newProperty->dataInt() + finalCount, maxId+1);
					}

					// Replace original property with the modified one.
					output().replaceObject(prop, newProperty);
				}
			}
		}
	}

	// Merge bonds if present.
	if(BondsObject* secondaryBonds = secondaryState.findObject<BondsObject>()) {

		// Collect bond properties.
		std::vector<BondProperty*> bondProperties;
		for(DataObject* obj : secondaryState.objects()) {
			if(OORef<BondPropertyObject> prop = dynamic_object_cast<BondPropertyObject>(obj)) {
				bondProperties.push_back(prop->storage());
			}
		}

		// Shift bond particle indices.
		QExplicitlySharedDataPointer<BondsStorage> shiftedBonds(new BondsStorage(*secondaryBonds->storage()));
		for(Bond& bond : *shiftedBonds) {
			bond.index1 += primaryCount;
			bond.index2 += primaryCount;
		}

		BondsDisplay* bondsDisplay = secondaryBonds->displayObjects().empty() ? nullptr : dynamic_object_cast<BondsDisplay>(secondaryBonds->displayObjects().front());
		addBonds(shiftedBonds.data(), bondsDisplay, bondProperties);
	}

	int secondaryFrame = secondaryState.attributes().value(QStringLiteral("SourceFrame"),
			dataset()->animationSettings()->timeToFrame(time)).toInt();

	QString statusMessage = tr("Combined %1 existing particles with %2 particles from frame %3 of second dataset.")
			.arg(primaryCount)
			.arg(secondaryCount)
			.arg(secondaryFrame);
	return PipelineStatus(secondaryState.status().type(), statusMessage);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
