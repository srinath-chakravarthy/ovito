///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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
#include <plugins/particles/objects/SurfaceMesh.h>
#include <plugins/particles/objects/SimulationCellObject.h>
#include "CreateIsosurfaceModifier.h"
#include "MarchingCubes.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Fields)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(CreateIsosurfaceModifier, AsynchronousParticleModifier);
DEFINE_PROPERTY_FIELD(CreateIsosurfaceModifier, sourceQuantity, "SourceQuantity");
DEFINE_FLAGS_REFERENCE_FIELD(CreateIsosurfaceModifier, isolevelController, "IsolevelController", Controller, PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_REFERENCE_FIELD(CreateIsosurfaceModifier, surfaceMeshDisplay, "SurfaceMeshDisplay", SurfaceMeshDisplay, PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);
SET_PROPERTY_FIELD_LABEL(CreateIsosurfaceModifier, sourceQuantity, "Source quantity");
SET_PROPERTY_FIELD_LABEL(CreateIsosurfaceModifier, isolevelController, "Isolevel");
SET_PROPERTY_FIELD_LABEL(CreateIsosurfaceModifier, surfaceMeshDisplay, "Surface mesh display");

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
CreateIsosurfaceModifier::CreateIsosurfaceModifier(DataSet* dataset) : AsynchronousParticleModifier(dataset)
{
	INIT_PROPERTY_FIELD(sourceQuantity);
	INIT_PROPERTY_FIELD(isolevelController);
	INIT_PROPERTY_FIELD(surfaceMeshDisplay);

	setIsolevelController(ControllerManager::createFloatController(dataset));

	// Create the display object.
	_surfaceMeshDisplay = new SurfaceMeshDisplay(dataset);
	_surfaceMeshDisplay->setShowCap(false);
	_surfaceMeshDisplay->setSmoothShading(true);
	_surfaceMeshDisplay->setObjectTitle(tr("Isosurface"));
}

/******************************************************************************
* Asks the modifier for its validity interval at the given time.
******************************************************************************/
TimeInterval CreateIsosurfaceModifier::modifierValidity(TimePoint time)
{
	TimeInterval interval = AsynchronousParticleModifier::modifierValidity(time);
	if(isolevelController()) interval.intersect(isolevelController()->validityInterval(time));
	return interval;
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool CreateIsosurfaceModifier::isApplicableTo(const PipelineFlowState& input)
{
	return (input.findObject<FieldQuantityObject>() != nullptr);
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a pipeline.
******************************************************************************/
void CreateIsosurfaceModifier::initializeModifier(PipelineObject* pipeline, ModifierApplication* modApp)
{
	ParticleModifier::initializeModifier(pipeline, modApp);

	// Use the first available particle property from the input state as data source when the modifier is newly created.
	if(sourceQuantity().isNull()) {
		PipelineFlowState input = getModifierInput(modApp);
		for(DataObject* o : input.objects()) {
			FieldQuantityObject* fq = dynamic_object_cast<FieldQuantityObject>(o);
			if(fq && fq->componentCount() <= 1) {
				setSourceQuantity(FieldQuantityReference(fq, (fq->componentCount() > 1) ? 0 : -1));
			}
		}
	}
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void CreateIsosurfaceModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	AsynchronousParticleModifier::propertyChanged(field);

	// Recompute results when the parameters have changed.
	if(field == PROPERTY_FIELD(sourceQuantity))
		invalidateCachedResults();
}

/******************************************************************************
* Handles reference events sent by reference targets of this object.
******************************************************************************/
bool CreateIsosurfaceModifier::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	// Do not propagate messages from the attached display object.
	if(source == surfaceMeshDisplay())
		return false;

	// Recompute results when the input parameters have changed.
	if(source == isolevelController() && event->type() == ReferenceEvent::TargetChanged)
		invalidateCachedResults();

	return AsynchronousParticleModifier::referenceEvent(source, event);
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
std::shared_ptr<AsynchronousParticleModifier::ComputeEngine> CreateIsosurfaceModifier::createEngine(TimePoint time, TimeInterval validityInterval)
{
	// Get modifier inputs.
	SimulationCellObject* simCell = expectSimulationCell();
	if(sourceQuantity().isNull())
		throwException(tr("Select a field quantity first."));
	FieldQuantityObject* quantity = sourceQuantity().findInState(input());
	if(!quantity)
		throwException(tr("The selected field quantity with the name '%1' does not exist.").arg(sourceQuantity().name()));
	if(sourceQuantity().vectorComponent() >= (int)quantity->componentCount())
		throwException(tr("The selected vector component is out of range. The field quantity '%1' contains only %2 values per field value.").arg(sourceQuantity().name()).arg(quantity->componentCount()));

	FloatType isolevel = isolevelController() ? isolevelController()->getFloatValue(time, validityInterval) : 0;

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<ComputeIsosurfaceEngine>(validityInterval, quantity->storage(), 
			sourceQuantity().vectorComponent(), simCell->data(), isolevel);
}

/******************************************************************************
* Unpacks the results of the computation engine and stores them in the modifier.
******************************************************************************/
void CreateIsosurfaceModifier::transferComputationResults(ComputeEngine* engine)
{
	ComputeIsosurfaceEngine* eng = static_cast<ComputeIsosurfaceEngine*>(engine);
	_surfaceMesh = eng->mesh();
	_isCompletelySolid = eng->isCompletelySolid();
	_minValue = eng->minValue();
	_maxValue = eng->maxValue();
}

/******************************************************************************
* Lets the modifier insert the cached computation results into the
* modification pipeline.
******************************************************************************/
PipelineStatus CreateIsosurfaceModifier::applyComputationResults(TimePoint time, TimeInterval& validityInterval)
{
	if(!_surfaceMesh)
		throwException(tr("No computation results available."));

	// Create the output data object.
	OORef<SurfaceMesh> meshObj(new SurfaceMesh(dataset(), _surfaceMesh.data()));
	meshObj->setIsCompletelySolid(_isCompletelySolid);
	meshObj->addDisplayObject(_surfaceMeshDisplay);

	// Insert output object into the pipeline.
	output().addObject(meshObj);

	return PipelineStatus(PipelineStatus::Success, tr("Minimum value: %1\nMaximum value: %2").arg(_minValue).arg(_maxValue));
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void CreateIsosurfaceModifier::ComputeIsosurfaceEngine::perform()
{
	setProgressText(tr("Constructing isosurface"));

	if(quantity()->shape().size() != 3)
		throw Exception(tr("Can construct isosurface only for three-dimensional fields"));
	if(quantity()->dataType() != qMetaTypeId<FloatType>())
		throw Exception(tr("Can construct isosurface only for floating-point data"));

	const FloatType* fieldData = quantity()->constDataFloat() + std::max(_vectorComponent, 0);
	const size_t shape[3] = {quantity()->shape()[0], quantity()->shape()[1], quantity()->shape()[2]}; 

	MarchingCubes mc(shape[0], shape[1], shape[2], fieldData, quantity()->componentCount(), *mesh());
	if(!mc.generateIsosurface(_isolevel, *this))
		return;
	_isCompletelySolid = mc.isCompletelySolid();

	// Determin min/max field values.
	const FloatType* fieldDataEnd = fieldData + shape[0]*shape[1]*shape[2]*quantity()->componentCount();
	_minValue =  FLOATTYPE_MAX;
	_maxValue = -FLOATTYPE_MAX;
	for(; fieldData != fieldDataEnd; fieldData += quantity()->componentCount()) {
		if(*fieldData < _minValue) _minValue = *fieldData;
		if(*fieldData > _maxValue) _maxValue = *fieldData;
	}

	// Transform mesh vertices from orthogonal grid space to world space.
	const AffineTransformation tm = _simCell.matrix() * 
		Matrix3(FloatType(1) / shape[0], 0, 0, 0, FloatType(1) / shape[1], 0, 0, 0, FloatType(1) / shape[2]);
	for(HalfEdgeMesh<>::Vertex* vertex : mesh()->vertices())
		vertex->setPos(tm * vertex->pos());

	if(isCanceled())
		return;

	if(!mesh()->connectOppositeHalfedges())
		throw Exception("Isosurface mesh is not closed.");
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
