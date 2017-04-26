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
#include <core/viewport/Viewport.h>
#include <core/viewport/ViewportConfiguration.h>
#include <core/scene/ObjectNode.h>
#include <core/scene/SelectionSet.h>
#include <core/animation/controller/Controller.h>
#include <core/animation/AnimationSettings.h>
#include <core/scene/pipeline/PipelineObject.h>
#include <core/plugins/PluginManager.h>
#include <plugins/particles/objects/SimulationCellObject.h>
#include "SliceModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(SliceModifier, ParticleModifier);
IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(SliceModifierFunction, RefTarget);
IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(SliceParticlesFunction, SliceModifierFunction);
DEFINE_REFERENCE_FIELD(SliceModifier, normalController, "PlaneNormal", Controller);
DEFINE_REFERENCE_FIELD(SliceModifier, distanceController, "PlaneDistance", Controller);
DEFINE_REFERENCE_FIELD(SliceModifier, widthController, "SliceWidth", Controller);
DEFINE_PROPERTY_FIELD(SliceModifier, createSelection, "CreateSelection");
DEFINE_PROPERTY_FIELD(SliceModifier, inverse, "Inverse");
DEFINE_PROPERTY_FIELD(SliceModifier, applyToSelection, "ApplyToSelection");
SET_PROPERTY_FIELD_LABEL(SliceModifier, normalController, "Normal");
SET_PROPERTY_FIELD_LABEL(SliceModifier, distanceController, "Distance");
SET_PROPERTY_FIELD_LABEL(SliceModifier, widthController, "Slice width");
SET_PROPERTY_FIELD_LABEL(SliceModifier, createSelection, "Create selection (do not delete)");
SET_PROPERTY_FIELD_LABEL(SliceModifier, inverse, "Reverse orientation");
SET_PROPERTY_FIELD_LABEL(SliceModifier, applyToSelection, "Apply to selection only");
SET_PROPERTY_FIELD_UNITS(SliceModifier, normalController, WorldParameterUnit);
SET_PROPERTY_FIELD_UNITS(SliceModifier, distanceController, WorldParameterUnit);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(SliceModifier, widthController, WorldParameterUnit, 0);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
SliceModifier::SliceModifier(DataSet* dataset) : ParticleModifier(dataset),
	_createSelection(false),
	_inverse(false),
	_applyToSelection(false)
{
	INIT_PROPERTY_FIELD(normalController);
	INIT_PROPERTY_FIELD(distanceController);
	INIT_PROPERTY_FIELD(widthController);
	INIT_PROPERTY_FIELD(createSelection);
	INIT_PROPERTY_FIELD(inverse);
	INIT_PROPERTY_FIELD(applyToSelection);

	setNormalController(ControllerManager::createVector3Controller(dataset));
	setDistanceController(ControllerManager::createFloatController(dataset));
	setWidthController(ControllerManager::createFloatController(dataset));
	if(normalController()) normalController()->setVector3Value(0, Vector3(1,0,0));
}

/******************************************************************************
* Asks the modifier for its validity interval at the given time.
******************************************************************************/
TimeInterval SliceModifier::modifierValidity(TimePoint time)
{
	TimeInterval interval = ParticleModifier::modifierValidity(time);
	if(normalController()) interval.intersect(normalController()->validityInterval(time));
	if(distanceController()) interval.intersect(distanceController()->validityInterval(time));
	if(widthController()) interval.intersect(widthController()->validityInterval(time));
	return interval;
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool SliceModifier::isApplicableTo(const PipelineFlowState& input)
{
	UndoSuspender noUndo(this);

	for(const OvitoObjectType* clazz : PluginManager::instance().listClasses(SliceModifierFunction::OOType)) {
		// Create instance of the slice function class.
		OORef<SliceModifierFunction> sliceFunc = static_object_cast<SliceModifierFunction>(clazz->createInstance(dataset()));

		// Let the slice function decide if can handle the input data type.
		if(sliceFunc->isApplicableTo(input))
			return true;
	}

	return false;
}

/******************************************************************************
* Returns the slicing plane.
******************************************************************************/
Plane3 SliceModifier::slicingPlane(TimePoint time, TimeInterval& validityInterval)
{
	Plane3 plane;
	if(normalController()) normalController()->getVector3Value(time, plane.normal, validityInterval);
	if(plane.normal == Vector3::Zero()) plane.normal = Vector3(0,0,1);
	else plane.normal.normalize();
	if(distanceController()) plane.dist = distanceController()->getFloatValue(time, validityInterval);
	if(inverse())
		return -plane;
	else
		return plane;
}

/******************************************************************************
* Modifies the particle object.
******************************************************************************/
PipelineStatus SliceModifier::modifyParticles(TimePoint time, TimeInterval& validityInterval)
{
	// Retrieve modifier parameters.
	FloatType sliceWidth = 0;
	if(widthController()) sliceWidth = widthController()->getFloatValue(time, validityInterval);
	Plane3 plane = slicingPlane(time, validityInterval);

	// Apply all registered and activated slice functions to the input data.
	PipelineStatus status(PipelineStatus::Success);
	for(const OvitoObjectType* clazz : PluginManager::instance().listClasses(SliceModifierFunction::OOType)) {
		OVITO_ASSERT(!dataset()->undoStack().isRecording());

		// Create instance of the slice function class.
		OORef<SliceModifierFunction> sliceFunc = static_object_cast<SliceModifierFunction>(clazz->createInstance(dataset()));

		// Skip function if not applicable.
		if(!sliceFunc->isApplicableTo(input()))
			continue;

		// Call the slice function.
		PipelineStatus funcStatus = sliceFunc->apply(this, time, plane, sliceWidth);

		// Append status text and code returned by the slice function to the status returned to our caller.
		if(status.type() == PipelineStatus::Success)
			status.setType(funcStatus.type());
		if(funcStatus.text().isEmpty() == false) {
			if(status.text().isEmpty() == false)
				status.setText(status.text() + QStringLiteral("\n") + funcStatus.text());
			else
				status.setText(funcStatus.text());
		}
	}

	return status;
}

/******************************************************************************
* Performs the actual rejection of particles.
******************************************************************************/
PipelineStatus SliceParticlesFunction::apply(SliceModifier* modifier, TimePoint time, const Plane3& plane, FloatType sliceWidth)
{
	QString statusMessage = tr("%n input particles", 0, modifier->inputParticleCount());

	boost::dynamic_bitset<> mask(modifier->inputParticleCount());

	// Get the required input properties.
	ParticlePropertyObject* const posProperty = modifier->expectStandardProperty(ParticleProperty::PositionProperty);
	ParticlePropertyObject* const selProperty = modifier->applyToSelection() ? modifier->inputStandardProperty(ParticleProperty::SelectionProperty) : nullptr;
	OVITO_ASSERT(posProperty->size() == mask.size());
	OVITO_ASSERT(!selProperty || selProperty->size() == mask.size());

	sliceWidth *= FloatType(0.5);

	size_t numRejected = 0;
	boost::dynamic_bitset<>::size_type i = 0;
	const Point3* p = posProperty->constDataPoint3();
	const Point3* p_end = p + posProperty->size();

	if(sliceWidth <= 0) {
		if(selProperty) {
			const int* s = selProperty->constDataInt();
			for(; p != p_end; ++p, ++s, ++i) {
				if(*s && plane.pointDistance(*p) > 0) {
					mask.set(i);
					numRejected++;
				}
				else mask.reset(i);
			}
		}
		else {
			for(; p != p_end; ++p, ++i) {
				if(plane.pointDistance(*p) > 0) {
					mask.set(i);
					numRejected++;
				}
				else mask.reset(i);
			}
		}
	}
	else {
		bool invert = modifier->inverse();
		if(selProperty) {
			const int* s = selProperty->constDataInt();
			for(; p != p_end; ++p, ++s, ++i) {
				if(*s && invert == (plane.classifyPoint(*p, sliceWidth) == 0)) {
					mask.set(i);
					numRejected++;
				}
				else mask.reset(i);
			}
		}
		else {
			for(; p != p_end; ++p, ++i) {
				if(invert == (plane.classifyPoint(*p, sliceWidth) == 0)) {
					mask.set(i);
					numRejected++;
				}
				else mask.reset(i);
			}
		}
	}

	size_t numKept = modifier->inputParticleCount() - numRejected;

	if(modifier->createSelection() == false) {

		statusMessage += tr("\n%n particles deleted", 0, numRejected);
		statusMessage += tr("\n%n particles remaining", 0, numKept);
		if(numRejected == 0)
			return PipelineStatus(PipelineStatus::Success, statusMessage);

		// Delete the rejected particles.
		modifier->deleteParticles(mask, numRejected);
	}
	else {
		statusMessage += tr("\n%n particles selected", 0, numRejected);
		statusMessage += tr("\n%n particles unselected", 0, numKept);

		ParticlePropertyObject* selProperty = modifier->outputStandardProperty(ParticleProperty::SelectionProperty);
		OVITO_ASSERT(mask.size() == selProperty->size());
		boost::dynamic_bitset<>::size_type i = 0;
		for(int& s : selProperty->intRange())
			s = mask.test(i++);
		selProperty->changed();
	}

	return PipelineStatus(PipelineStatus::Success, statusMessage);
}

/******************************************************************************
* Lets the modifier render itself into the viewport.
******************************************************************************/
void SliceModifier::render(TimePoint time, ObjectNode* contextNode, ModifierApplication* modApp, SceneRenderer* renderer, bool renderOverlay)
{
	if(!renderOverlay && isObjectBeingEdited() && renderer->isInteractive() && !renderer->isPicking()) {
		renderVisual(time, contextNode, renderer);
	}
}

/******************************************************************************
* Computes the bounding box of the visual representation of the modifier.
******************************************************************************/
Box3 SliceModifier::boundingBox(TimePoint time, ObjectNode* contextNode, ModifierApplication* modApp)
{
	if(isObjectBeingEdited())
		return renderVisual(time, contextNode, nullptr);
	else
		return Box3();
}

/******************************************************************************
* Renders the modifier's visual representation and computes its bounding box.
******************************************************************************/
Box3 SliceModifier::renderVisual(TimePoint time, ObjectNode* contextNode, SceneRenderer* renderer)
{
	TimeInterval interval;

	Box3 bb = contextNode->localBoundingBox(time);
	if(bb.isEmpty())
		return Box3();

	Plane3 plane = slicingPlane(time, interval);

	FloatType sliceWidth = 0;
	if(widthController()) sliceWidth = widthController()->getFloatValue(time, interval);

	ColorA color(0.8f, 0.3f, 0.3f);
	if(sliceWidth <= 0) {
		return renderPlane(renderer, plane, bb, color);
	}
	else {
		plane.dist += sliceWidth / 2;
		Box3 box = renderPlane(renderer, plane, bb, color);
		plane.dist -= sliceWidth;
		box.addBox(renderPlane(renderer, plane, bb, color));
		return box;
	}
}

/******************************************************************************
* Renders the plane in the viewports.
******************************************************************************/
Box3 SliceModifier::renderPlane(SceneRenderer* renderer, const Plane3& plane, const Box3& bb, const ColorA& color) const
{
	// Compute intersection lines of slicing plane and bounding box.
	QVector<Point3> vertices;
	Point3 corners[8];
	for(int i = 0; i < 8; i++)
		corners[i] = bb[i];

	planeQuadIntersection(corners, {{0, 1, 5, 4}}, plane, vertices);
	planeQuadIntersection(corners, {{1, 3, 7, 5}}, plane, vertices);
	planeQuadIntersection(corners, {{3, 2, 6, 7}}, plane, vertices);
	planeQuadIntersection(corners, {{2, 0, 4, 6}}, plane, vertices);
	planeQuadIntersection(corners, {{4, 5, 7, 6}}, plane, vertices);
	planeQuadIntersection(corners, {{0, 2, 3, 1}}, plane, vertices);

	// If there is not intersection with the simulation box then
	// project the simulation box onto the plane.
	if(vertices.empty()) {
		const static int edges[12][2] = {
				{0,1},{1,3},{3,2},{2,0},
				{4,5},{5,7},{7,6},{6,4},
				{0,4},{1,5},{3,7},{2,6}
		};
		for(int edge = 0; edge < 12; edge++) {
			vertices.push_back(plane.projectPoint(corners[edges[edge][0]]));
			vertices.push_back(plane.projectPoint(corners[edges[edge][1]]));
		}
	}

	if(renderer) {
		// Render plane-box intersection lines.
		std::shared_ptr<LinePrimitive> buffer = renderer->createLinePrimitive();
		buffer->setVertexCount(vertices.size());
		buffer->setVertexPositions(vertices.constData());
		buffer->setLineColor(color);
		buffer->render(renderer);
	}

	// Compute bounding box.
	Box3 vertexBoundingBox;
	vertexBoundingBox.addPoints(vertices.constData(), vertices.size());
	return vertexBoundingBox;
}

/******************************************************************************
* Computes the intersection lines of a plane and a quad.
******************************************************************************/
void SliceModifier::planeQuadIntersection(const Point3 corners[8], const std::array<int,4>& quadVerts, const Plane3& plane, QVector<Point3>& vertices) const
{
	Point3 p1;
	bool hasP1 = false;
	for(int i = 0; i < 4; i++) {
		Ray3 edge(corners[quadVerts[i]], corners[quadVerts[(i+1)%4]]);
		FloatType t = plane.intersectionT(edge, FLOATTYPE_EPSILON);
		if(t < 0 || t > 1) continue;
		if(!hasP1) {
			p1 = edge.point(t);
			hasP1 = true;
		}
		else {
			Point3 p2 = edge.point(t);
			if(!p2.equals(p1)) {
				vertices.push_back(p1);
				vertices.push_back(p2);
				return;
			}
		}
	}
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a PipelineObject.
******************************************************************************/
void SliceModifier::initializeModifier(PipelineObject* pipeline, ModifierApplication* modApp)
{
	ParticleModifier::initializeModifier(pipeline, modApp);

	// Get the input simulation cell to initially place the slicing plane in
	// the center of the cell.
	PipelineFlowState input = getModifierInput(modApp);
	SimulationCellObject* cell = input.findObject<SimulationCellObject>();
	TimeInterval iv;
	if(distanceController() && cell && distanceController()->getFloatValue(0, iv) == 0) {
		Point3 centerPoint = cell->cellMatrix() * Point3(0.5, 0.5, 0.5);
		FloatType centerDistance = normal().dot(centerPoint - Point3::Origin());
		if(fabs(centerDistance) > FLOATTYPE_EPSILON && distanceController())
			distanceController()->setFloatValue(0, centerDistance);
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
