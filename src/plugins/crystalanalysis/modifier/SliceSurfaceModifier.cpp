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

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <core/viewport/Viewport.h>
#include <core/viewport/ViewportConfiguration.h>
#include <core/scene/ObjectNode.h>
#include <core/scene/SelectionSet.h>
#include <core/animation/controller/Controller.h>
#include <core/animation/AnimationSettings.h>
#include <core/scene/pipeline/PipelineObject.h>
#include <core/gui/actions/ActionManager.h>
#include <core/gui/actions/ViewportModeAction.h>
#include <core/gui/mainwin/MainWindow.h>
#include <core/gui/properties/FloatParameterUI.h>
#include <core/gui/properties/Vector3ParameterUI.h>
#include <core/gui/properties/BooleanParameterUI.h>
#include <core/rendering/viewport/ViewportSceneRenderer.h>
#include <plugins/particles/objects/SimulationCellObject.h>
#include <plugins/particles/objects/SurfaceMesh.h>
#include <plugins/crystalanalysis/objects/dislocations/DislocationNetworkObject.h>
#include "SliceSurfaceModifier.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(CrystalAnalysis, SliceSurfaceModifier, Modifier);
SET_OVITO_OBJECT_EDITOR(SliceSurfaceModifier, SliceSurfaceModifierEditor);
DEFINE_REFERENCE_FIELD(SliceSurfaceModifier, _normalCtrl, "PlaneNormal", Controller);
DEFINE_REFERENCE_FIELD(SliceSurfaceModifier, _distanceCtrl, "PlaneDistance", Controller);
DEFINE_REFERENCE_FIELD(SliceSurfaceModifier, _widthCtrl, "SliceWidth", Controller);
DEFINE_PROPERTY_FIELD(SliceSurfaceModifier, _inverse, "Inverse");
DEFINE_PROPERTY_FIELD(SliceSurfaceModifier, _modifySurfaces, "ModifySurfaces");
DEFINE_PROPERTY_FIELD(SliceSurfaceModifier, _modifyDislocations, "ModifyDislocations");
SET_PROPERTY_FIELD_LABEL(SliceSurfaceModifier, _normalCtrl, "Normal");
SET_PROPERTY_FIELD_LABEL(SliceSurfaceModifier, _distanceCtrl, "Distance");
SET_PROPERTY_FIELD_LABEL(SliceSurfaceModifier, _widthCtrl, "Slice width");
SET_PROPERTY_FIELD_LABEL(SliceSurfaceModifier, _inverse, "Invert");
SET_PROPERTY_FIELD_LABEL(SliceSurfaceModifier, _modifySurfaces, "Apply to surfaces");
SET_PROPERTY_FIELD_LABEL(SliceSurfaceModifier, _modifyDislocations, "Apply to dislocations");
SET_PROPERTY_FIELD_UNITS(SliceSurfaceModifier, _normalCtrl, WorldParameterUnit);
SET_PROPERTY_FIELD_UNITS(SliceSurfaceModifier, _distanceCtrl, WorldParameterUnit);
SET_PROPERTY_FIELD_UNITS(SliceSurfaceModifier, _widthCtrl, WorldParameterUnit);

IMPLEMENT_OVITO_OBJECT(CrystalAnalysis, SliceSurfaceModifierEditor, PropertiesEditor);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
SliceSurfaceModifier::SliceSurfaceModifier(DataSet* dataset) : Modifier(dataset),
	_inverse(false), _modifySurfaces(true), _modifyDislocations(true)
{
	INIT_PROPERTY_FIELD(SliceSurfaceModifier::_normalCtrl);
	INIT_PROPERTY_FIELD(SliceSurfaceModifier::_distanceCtrl);
	INIT_PROPERTY_FIELD(SliceSurfaceModifier::_widthCtrl);
	INIT_PROPERTY_FIELD(SliceSurfaceModifier::_inverse);
	INIT_PROPERTY_FIELD(SliceSurfaceModifier::_modifySurfaces);
	INIT_PROPERTY_FIELD(SliceSurfaceModifier::_modifyDislocations);

	_normalCtrl = ControllerManager::instance().createVector3Controller(dataset);
	_distanceCtrl = ControllerManager::instance().createFloatController(dataset);
	_widthCtrl = ControllerManager::instance().createFloatController(dataset);
	if(_normalCtrl) _normalCtrl->setVector3Value(0, Vector3(1,0,0));
}

/******************************************************************************
* Asks the modifier for its validity interval at the given time.
******************************************************************************/
TimeInterval SliceSurfaceModifier::modifierValidity(TimePoint time)
{
	TimeInterval interval = Modifier::modifierValidity(time);
	interval.intersect(_normalCtrl->validityInterval(time));
	interval.intersect(_distanceCtrl->validityInterval(time));
	interval.intersect(_widthCtrl->validityInterval(time));
	return interval;
}

/******************************************************************************
* Asks the modifier whether it can be applied to the given input data.
******************************************************************************/
bool SliceSurfaceModifier::isApplicableTo(const PipelineFlowState& input)
{
	return (input.findObject<SurfaceMesh>() != nullptr) || (input.findObject<DislocationNetworkObject>() != nullptr);
}

/******************************************************************************
* Returns the slicing plane.
******************************************************************************/
Plane3 SliceSurfaceModifier::slicingPlane(TimePoint time, TimeInterval& validityInterval)
{
	Plane3 plane;
	_normalCtrl->getVector3Value(time, plane.normal, validityInterval);
	if(plane.normal == Vector3::Zero()) plane.normal = Vector3(0,0,1);
	else plane.normal.normalize();
	plane.dist = _distanceCtrl->getFloatValue(time, validityInterval);
	if(inverse())
		return -plane;
	else
		return plane;
}

/******************************************************************************
* This modifies the input object.
******************************************************************************/
PipelineStatus SliceSurfaceModifier::modifyObject(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state)
{
	TimeInterval validityInterval = state.stateValidity();
	Plane3 plane = slicingPlane(time, validityInterval);

	FloatType sliceWidth = 0;
	if(_widthCtrl) sliceWidth = _widthCtrl->getFloatValue(time, validityInterval);
	sliceWidth *= 0.5;

	CloneHelper cloneHelper;

	for(int i = 0; i < state.objects().size(); i++) {
		DataObject* obj = state.objects()[i];
		if(_modifySurfaces) {
			if(SurfaceMesh* inputMesh = dynamic_object_cast<SurfaceMesh>(obj)) {
				OORef<SurfaceMesh> outputMesh = cloneHelper.cloneObject(inputMesh, false);
				QVector<Plane3> planes = inputMesh->cuttingPlanes();
				if(sliceWidth <= 0) {
					planes.push_back(plane);
				}
				else {
					planes.push_back(Plane3(plane.normal, plane.dist + sliceWidth));
					planes.push_back(Plane3(-plane.normal, -plane.dist + sliceWidth));
				}
				outputMesh->setCuttingPlanes(planes);
				state.replaceObject(inputMesh, outputMesh);
				state.intersectStateValidity(validityInterval);
				continue;
			}
		}
		if(_modifyDislocations) {
			if(DislocationNetworkObject* inputDislocations = dynamic_object_cast<DislocationNetworkObject>(obj)) {
				OORef<DislocationNetworkObject> outputDislocations = cloneHelper.cloneObject(inputDislocations, false);
				QVector<Plane3> planes = inputDislocations->cuttingPlanes();
				if(sliceWidth <= 0) {
					planes.push_back(plane);
				}
				else {
					planes.push_back(Plane3(plane.normal, plane.dist + sliceWidth));
					planes.push_back(Plane3(-plane.normal, -plane.dist + sliceWidth));
				}
				outputDislocations->setCuttingPlanes(planes);
				state.replaceObject(inputDislocations, outputDislocations);
				state.intersectStateValidity(validityInterval);
				continue;
			}
		}
	}

	return PipelineStatus::Success;
}


/******************************************************************************
* Lets the modifier render itself into the viewport.
******************************************************************************/
void SliceSurfaceModifier::render(TimePoint time, ObjectNode* contextNode, ModifierApplication* modApp, SceneRenderer* renderer, bool renderOverlay)
{
	if(!renderOverlay && PropertiesEditor::isObjectBeingEdited(this) && renderer->isInteractive() && !renderer->isPicking())
		renderVisual(time, contextNode, renderer);
}

/******************************************************************************
* Computes the bounding box of the visual representation of the modifier.
******************************************************************************/
Box3 SliceSurfaceModifier::boundingBox(TimePoint time, ObjectNode* contextNode, ModifierApplication* modApp)
{
	if(PropertiesEditor::isObjectBeingEdited(this))
		return renderVisual(time, contextNode, nullptr);
	else
		return Box3();
}

/******************************************************************************
* Renders the modifier's visual representation and computes its bounding box.
******************************************************************************/
Box3 SliceSurfaceModifier::renderVisual(TimePoint time, ObjectNode* contextNode, SceneRenderer* renderer)
{
	TimeInterval interval;

	Box3 bb = contextNode->localBoundingBox(time);
	if(bb.isEmpty())
		return Box3();

	Plane3 plane = slicingPlane(time, interval);

	FloatType sliceWidth = 0;
	if(_widthCtrl) sliceWidth = _widthCtrl->getFloatValue(time, interval);

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
Box3 SliceSurfaceModifier::renderPlane(SceneRenderer* renderer, const Plane3& plane, const Box3& bb, const ColorA& color) const
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
void SliceSurfaceModifier::planeQuadIntersection(const Point3 corners[8], const std::array<int,4>& quadVerts, const Plane3& plane, QVector<Point3>& vertices) const
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
void SliceSurfaceModifier::initializeModifier(PipelineObject* pipeline, ModifierApplication* modApp)
{
	Modifier::initializeModifier(pipeline, modApp);

	// Get the input simulation cell to initially place the slicing plane in
	// the center of the cell.
	PipelineFlowState input = pipeline->evaluatePipeline(dataset()->animationSettings()->time(), modApp, false);
	SimulationCellObject* cell = input.findObject<SimulationCellObject>();
	TimeInterval iv;
	if(distanceController() && cell && distanceController()->getFloatValue(0, iv) == 0) {
		Point3 centerPoint = cell->cellMatrix() * Point3(0.5, 0.5, 0.5);
		FloatType centerDistance = normal().dot(centerPoint - Point3::Origin());
		if(fabs(centerDistance) > FLOATTYPE_EPSILON && distanceController())
			distanceController()->setFloatValue(0, centerDistance);
	}
}

OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void SliceSurfaceModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Slice surface and dislocations"), rolloutParams);

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QGridLayout* gridlayout = new QGridLayout();
	gridlayout->setContentsMargins(0,0,0,0);
	gridlayout->setColumnStretch(1, 1);

	// Distance parameter.
	FloatParameterUI* distancePUI = new FloatParameterUI(this, PROPERTY_FIELD(SliceSurfaceModifier::_distanceCtrl));
	gridlayout->addWidget(distancePUI->label(), 0, 0);
	gridlayout->addLayout(distancePUI->createFieldLayout(), 0, 1);

	// Normal parameter.
	for(int i = 0; i < 3; i++) {
		Vector3ParameterUI* normalPUI = new Vector3ParameterUI(this, PROPERTY_FIELD(SliceSurfaceModifier::_normalCtrl), i);
		normalPUI->label()->setTextFormat(Qt::RichText);
		normalPUI->label()->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
		normalPUI->label()->setText(QStringLiteral("<a href=\"%1\">%2</a>").arg(i).arg(normalPUI->label()->text()));
		connect(normalPUI->label(), &QLabel::linkActivated, this, &SliceSurfaceModifierEditor::onXYZNormal);
		gridlayout->addWidget(normalPUI->label(), i+1, 0);
		gridlayout->addLayout(normalPUI->createFieldLayout(), i+1, 1);
	}

	// Slice width parameter.
	FloatParameterUI* widthPUI = new FloatParameterUI(this, PROPERTY_FIELD(SliceSurfaceModifier::_widthCtrl));
	gridlayout->addWidget(widthPUI->label(), 4, 0);
	gridlayout->addLayout(widthPUI->createFieldLayout(), 4, 1);
	widthPUI->setMinValue(0);

	layout->addLayout(gridlayout);
	layout->addSpacing(8);

	// Invert parameter.
	BooleanParameterUI* invertPUI = new BooleanParameterUI(this, PROPERTY_FIELD(SliceSurfaceModifier::_inverse));
	layout->addWidget(invertPUI->checkBox());

	layout->addSpacing(8);

	// Application parameters.
	BooleanParameterUI* applyToSurfacesPUI = new BooleanParameterUI(this, PROPERTY_FIELD(SliceSurfaceModifier::_modifySurfaces));
	layout->addWidget(applyToSurfacesPUI->checkBox());
	BooleanParameterUI* applyToDislocationsPUI = new BooleanParameterUI(this, PROPERTY_FIELD(SliceSurfaceModifier::_modifyDislocations));
	layout->addWidget(applyToDislocationsPUI->checkBox());

	layout->addSpacing(8);
	QPushButton* centerPlaneBtn = new QPushButton(tr("Move plane to simulation box center"), rollout);
	connect(centerPlaneBtn, &QPushButton::clicked, this, &SliceSurfaceModifierEditor::onCenterOfBox);
	layout->addWidget(centerPlaneBtn);

	// Add buttons for view alignment functions.
	QPushButton* alignViewToPlaneBtn = new QPushButton(tr("Align view direction to plane normal"), rollout);
	connect(alignViewToPlaneBtn, &QPushButton::clicked, this, &SliceSurfaceModifierEditor::onAlignViewToPlane);
	layout->addWidget(alignViewToPlaneBtn);
	QPushButton* alignPlaneToViewBtn = new QPushButton(tr("Align plane normal to view direction"), rollout);
	connect(alignPlaneToViewBtn, &QPushButton::clicked, this, &SliceSurfaceModifierEditor::onAlignPlaneToView);
	layout->addWidget(alignPlaneToViewBtn);
}

/******************************************************************************
* Aligns the normal of the slicing plane with the X, Y, or Z axis.
******************************************************************************/
void SliceSurfaceModifierEditor::onXYZNormal(const QString& link)
{
	SliceSurfaceModifier* mod = static_object_cast<SliceSurfaceModifier>(editObject());
	if(!mod) return;

	undoableTransaction(tr("Set plane normal"), [mod, &link]() {
		if(link == "0")
			mod->setNormal(Vector3(1,0,0));
		else if(link == "1")
			mod->setNormal(Vector3(0,1,0));
		else if(link == "2")
			mod->setNormal(Vector3(0,0,1));
	});
}

/******************************************************************************
* Aligns the slicing plane to the viewing direction.
******************************************************************************/
void SliceSurfaceModifierEditor::onAlignPlaneToView()
{
	TimeInterval interval;

	Viewport* vp = dataset()->viewportConfig()->activeViewport();
	if(!vp) return;

	// Get the object to world transformation for the currently selected object.
	ObjectNode* node = dynamic_object_cast<ObjectNode>(dataset()->selection()->front());
	if(!node) return;
	const AffineTransformation& nodeTM = node->getWorldTransform(dataset()->animationSettings()->time(), interval);

	// Get the base point of the current slicing plane in local coordinates.
	SliceSurfaceModifier* mod = static_object_cast<SliceSurfaceModifier>(editObject());
	if(!mod) return;
	Plane3 oldPlaneLocal = mod->slicingPlane(dataset()->animationSettings()->time(), interval);
	Point3 basePoint = Point3::Origin() + oldPlaneLocal.normal * oldPlaneLocal.dist;

	// Get the orientation of the projection plane of the current viewport.
	Vector3 dirWorld = -vp->cameraDirection();
	Plane3 newPlaneLocal(basePoint, nodeTM.inverse() * dirWorld);
	if(std::abs(newPlaneLocal.normal.x()) < FLOATTYPE_EPSILON) newPlaneLocal.normal.x() = 0;
	if(std::abs(newPlaneLocal.normal.y()) < FLOATTYPE_EPSILON) newPlaneLocal.normal.y() = 0;
	if(std::abs(newPlaneLocal.normal.z()) < FLOATTYPE_EPSILON) newPlaneLocal.normal.z() = 0;

	undoableTransaction(tr("Align plane to view"), [mod, &newPlaneLocal]() {
		mod->setNormal(newPlaneLocal.normal.normalized());
		mod->setDistance(newPlaneLocal.dist);
	});
}

/******************************************************************************
* Aligns the current viewing direction to the slicing plane.
******************************************************************************/
void SliceSurfaceModifierEditor::onAlignViewToPlane()
{
	TimeInterval interval;

	Viewport* vp = dataset()->viewportConfig()->activeViewport();
	if(!vp) return;

	// Get the object to world transformation for the currently selected object.
	ObjectNode* node = dynamic_object_cast<ObjectNode>(dataset()->selection()->front());
	if(!node) return;
	const AffineTransformation& nodeTM = node->getWorldTransform(dataset()->animationSettings()->time(), interval);

	// Transform the current slicing plane to the world coordinate system.
	SliceSurfaceModifier* mod = static_object_cast<SliceSurfaceModifier>(editObject());
	if(!mod) return;
	Plane3 planeLocal = mod->slicingPlane(dataset()->animationSettings()->time(), interval);
	Plane3 planeWorld = nodeTM * planeLocal;

	// Calculate the intersection point of the current viewing direction with the current slicing plane.
	Ray3 viewportRay(vp->cameraPosition(), vp->cameraDirection());
	FloatType t = planeWorld.intersectionT(viewportRay);
	Point3 intersectionPoint;
	if(t != FLOATTYPE_MAX)
		intersectionPoint = viewportRay.point(t);
	else
		intersectionPoint = Point3::Origin() + nodeTM.translation();

	if(vp->isPerspectiveProjection()) {
		FloatType distance = (vp->cameraPosition() - intersectionPoint).length();
		vp->setViewType(Viewport::VIEW_PERSPECTIVE);
		vp->setCameraDirection(-planeWorld.normal);
		vp->setCameraPosition(intersectionPoint + planeWorld.normal * distance);
	}
	else {
		vp->setViewType(Viewport::VIEW_ORTHO);
		vp->setCameraDirection(-planeWorld.normal);
	}

	vp->zoomToSelectionExtents();
}

/******************************************************************************
* Moves the plane to the center of the simulation box.
******************************************************************************/
void SliceSurfaceModifierEditor::onCenterOfBox()
{
	SliceSurfaceModifier* mod = static_object_cast<SliceSurfaceModifier>(editObject());
	if(!mod) return;

	// Get the simulation cell from the input object to center the slicing plane in
	// the center of the simulation cell.
	PipelineFlowState input = mod->getModifierInput();
	SimulationCellObject* cell = input.findObject<SimulationCellObject>();
	if(!cell) return;

	Point3 centerPoint = cell->cellMatrix() * Point3(0.5, 0.5, 0.5);
	FloatType centerDistance = mod->normal().dot(centerPoint - Point3::Origin());

	undoableTransaction(tr("Set plane position"), [mod, centerDistance]() {
		mod->setDistance(centerDistance);
	});
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
