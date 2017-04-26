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
#include <core/scene/pipeline/PipelineObject.h>
#include <core/animation/AnimationSettings.h>
#include <plugins/particles/objects/SurfaceMesh.h>
#include "AffineTransformationModifier.h"

#include <QtConcurrent>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(AffineTransformationModifier, ParticleModifier);
DEFINE_PROPERTY_FIELD(AffineTransformationModifier, transformationTM, "Transformation");
DEFINE_PROPERTY_FIELD(AffineTransformationModifier, applyToParticles, "ApplyToParticles");
DEFINE_PROPERTY_FIELD(AffineTransformationModifier, selectionOnly, "SelectionOnly");
DEFINE_PROPERTY_FIELD(AffineTransformationModifier, applyToSimulationBox, "ApplyToSimulationBox");
DEFINE_PROPERTY_FIELD(AffineTransformationModifier, targetCell, "DestinationCell");
DEFINE_PROPERTY_FIELD(AffineTransformationModifier, relativeMode, "RelativeMode");
DEFINE_PROPERTY_FIELD(AffineTransformationModifier, applyToSurfaceMesh, "ApplyToSurfaceMesh");
DEFINE_PROPERTY_FIELD(AffineTransformationModifier, applyToVectorProperties, "ApplyToVectorProperties");
SET_PROPERTY_FIELD_LABEL(AffineTransformationModifier, transformationTM, "Transformation");
SET_PROPERTY_FIELD_LABEL(AffineTransformationModifier, applyToParticles, "Transform particle positions");
SET_PROPERTY_FIELD_LABEL(AffineTransformationModifier, selectionOnly, "Selected particles only");
SET_PROPERTY_FIELD_LABEL(AffineTransformationModifier, applyToSimulationBox, "Transform simulation cell");
SET_PROPERTY_FIELD_LABEL(AffineTransformationModifier, targetCell, "Destination cell geometry");
SET_PROPERTY_FIELD_LABEL(AffineTransformationModifier, relativeMode, "Relative transformation");
SET_PROPERTY_FIELD_LABEL(AffineTransformationModifier, applyToSurfaceMesh, "Transform surface mesh");
SET_PROPERTY_FIELD_LABEL(AffineTransformationModifier, applyToVectorProperties, "Transform vector properties");

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
AffineTransformationModifier::AffineTransformationModifier(DataSet* dataset) : ParticleModifier(dataset),
	_applyToParticles(true), _selectionOnly(false), _applyToSimulationBox(false),
	_transformationTM(AffineTransformation::Identity()), _targetCell(AffineTransformation::Zero()),
	_relativeMode(true), _applyToSurfaceMesh(true), _applyToVectorProperties(false)
{
	INIT_PROPERTY_FIELD(transformationTM);
	INIT_PROPERTY_FIELD(applyToParticles);
	INIT_PROPERTY_FIELD(selectionOnly);
	INIT_PROPERTY_FIELD(applyToSimulationBox);
	INIT_PROPERTY_FIELD(targetCell);
	INIT_PROPERTY_FIELD(relativeMode);
	INIT_PROPERTY_FIELD(applyToSurfaceMesh);
	INIT_PROPERTY_FIELD(applyToVectorProperties);
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a PipelineObject.
******************************************************************************/
void AffineTransformationModifier::initializeModifier(PipelineObject* pipeline, ModifierApplication* modApp)
{
	ParticleModifier::initializeModifier(pipeline, modApp);

	// Take the simulation cell from the input object as the default destination cell geometry for absolute scaling.
	if(targetCell() == AffineTransformation::Zero()) {
		PipelineFlowState input = getModifierInput(modApp);
		SimulationCellObject* cell = input.findObject<SimulationCellObject>();
		if(cell)
			setTargetCell(cell->cellMatrix());
	}
}

/******************************************************************************
* Modifies the particle object.
******************************************************************************/
PipelineStatus AffineTransformationModifier::modifyParticles(TimePoint time, TimeInterval& validityInterval)
{
	AffineTransformation tm;
	if(relativeMode()) {
		tm = transformationTM();
		if(applyToSimulationBox()) {
			AffineTransformation deformedCell = tm * expectSimulationCell()->cellMatrix();
			outputSimulationCell()->setCellMatrix(deformedCell);
		}
	}
	else {
		AffineTransformation oldCell = expectSimulationCell()->cellMatrix();
		if(oldCell.determinant() == 0)
			throwException(tr("Input simulation cell is degenerate."));
		tm = targetCell() * oldCell.inverse();
		if(applyToSimulationBox())
			outputSimulationCell()->setCellMatrix(targetCell());
	}

	if(applyToParticles()) {
		expectStandardProperty(ParticleProperty::PositionProperty);
		ParticlePropertyObject* posProperty = outputStandardProperty(ParticleProperty::PositionProperty, true);

		if(selectionOnly()) {
			ParticlePropertyObject* selProperty = inputStandardProperty(ParticleProperty::SelectionProperty);
			if(selProperty) {
				const int* sbegin = selProperty->constDataInt();
				Point3* pbegin = posProperty->dataPoint3();
				Point3* pend = pbegin + posProperty->size();
				QtConcurrent::blockingMap(pbegin, pend, [&tm, pbegin, sbegin](Point3& p) {
					if(sbegin[&p - pbegin])
						p = tm * p;
				});
			}
		}
		else {
			Point3* const pbegin = posProperty->dataPoint3();
			Point3* const pend = pbegin + posProperty->size();

			// Check if the matrix describes a pure translation. If yes, we can
			// simply add vectors instead of computing full matrix products.
			Vector3 translation = tm.translation();
			if(tm == AffineTransformation::translation(translation)) {
				for(Point3* p = pbegin; p != pend; ++p)
					*p += translation;
			}
			else {
				QtConcurrent::blockingMap(pbegin, pend, [&tm](Point3& p) { p = tm * p; });
			}
		}

		posProperty->changed();
	}

	if(applyToVectorProperties()) {
		for(DataObject* obj : input().objects()) {
			if(OORef<ParticlePropertyObject> inputProperty = dynamic_object_cast<ParticlePropertyObject>(obj)) {
				if( inputProperty->type() == ParticleProperty::VelocityProperty ||
					inputProperty->type() == ParticleProperty::ForceProperty ||
					inputProperty->type() == ParticleProperty::DisplacementProperty) {

					PropertyBase* property = outputStandardProperty(inputProperty->type(), true)->modifiableStorage();
					OVITO_ASSERT(property->dataType() == qMetaTypeId<FloatType>());
					OVITO_ASSERT(property->componentCount() == 3);
					Vector3* const pbegin = property->dataVector3();
					Vector3* const pend = pbegin + property->size();
					if(!selectionOnly()) {
						QtConcurrent::blockingMap(pbegin, pend, [&tm](Vector3& v) { v = tm * v; });
					}
					else {
						ParticlePropertyObject* selProperty = inputStandardProperty(ParticleProperty::SelectionProperty);
						if(selProperty) {
							QtConcurrent::blockingMap(pbegin, pend, [&tm, pbegin, selProperty](Vector3& v) {
								if(selProperty->getInt(&v - pbegin))
									v = tm * v;
							});
						}
					}
				}
			}
		}
	}

	if(applyToSurfaceMesh()) {
		for(int index = 0; index < input().objects().size(); index++) {
			// Apply transformation to vertices of surface mesh.
			if(SurfaceMesh* inputSurface = dynamic_object_cast<SurfaceMesh>(input().objects()[index])) {
				OORef<SurfaceMesh> outputSurface = cloneHelper()->cloneObject(inputSurface, false);
				for(HalfEdgeMesh<>::Vertex* vertex : outputSurface->modifiableStorage()->vertices())
					vertex->pos() = tm * vertex->pos();
				outputSurface->changed();
				output().replaceObject(inputSurface, outputSurface);
			}
		}
	}

	return PipelineStatus::Success;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
