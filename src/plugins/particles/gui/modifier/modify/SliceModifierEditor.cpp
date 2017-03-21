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

#include <plugins/particles/gui/ParticlesGui.h>
#include <plugins/particles/modifier/modify/SliceModifier.h>
#include <core/animation/AnimationSettings.h>
#include <core/scene/ObjectNode.h>
#include <core/scene/SelectionSet.h>
#include <core/viewport/Viewport.h>
#include <core/viewport/ViewportConfiguration.h>
#include <core/plugins/PluginManager.h>
#include <gui/actions/ViewportModeAction.h>
#include <gui/mainwin/MainWindow.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/Vector3ParameterUI.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/rendering/ViewportSceneRenderer.h>
#include <gui/viewport/ViewportWindow.h>
#include "SliceModifierEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(SliceModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(SliceModifier, SliceModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
*******************************************************************************/
void SliceModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Slice"), rolloutParams, "particles.modifiers.slice.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QGridLayout* gridlayout = new QGridLayout();
	gridlayout->setContentsMargins(0,0,0,0);
	gridlayout->setColumnStretch(1, 1);

	// Distance parameter.
	FloatParameterUI* distancePUI = new FloatParameterUI(this, PROPERTY_FIELD(SliceModifier::distanceController));
	gridlayout->addWidget(distancePUI->label(), 0, 0);
	gridlayout->addLayout(distancePUI->createFieldLayout(), 0, 1);

	// Normal parameter.
	static const QString axesNames[3] = { QStringLiteral("X"), QStringLiteral("Y"), QStringLiteral("Z") };
	for(int i = 0; i < 3; i++) {
		Vector3ParameterUI* normalPUI = new Vector3ParameterUI(this, PROPERTY_FIELD(SliceModifier::normalController), i);
		normalPUI->label()->setTextFormat(Qt::RichText);
		normalPUI->label()->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
		normalPUI->label()->setText(QStringLiteral("<a href=\"%1\">%2</a>").arg(i).arg(normalPUI->label()->text()));
		normalPUI->label()->setToolTip(tr("Click here to align plane normal with %1 axis").arg(axesNames[i]));
		connect(normalPUI->label(), &QLabel::linkActivated, this, &SliceModifierEditor::onXYZNormal);
		gridlayout->addWidget(normalPUI->label(), i+1, 0);
		gridlayout->addLayout(normalPUI->createFieldLayout(), i+1, 1);
	}

	// Slice width parameter.
	FloatParameterUI* widthPUI = new FloatParameterUI(this, PROPERTY_FIELD(SliceModifier::widthController));
	gridlayout->addWidget(widthPUI->label(), 4, 0);
	gridlayout->addLayout(widthPUI->createFieldLayout(), 4, 1);

	layout->addLayout(gridlayout);
	layout->addSpacing(8);

	// Invert parameter.
	BooleanParameterUI* invertPUI = new BooleanParameterUI(this, PROPERTY_FIELD(SliceModifier::inverse));
	layout->addWidget(invertPUI->checkBox());

	// Create selection parameter.
	BooleanParameterUI* createSelectionPUI = new BooleanParameterUI(this, PROPERTY_FIELD(SliceModifier::createSelection));
	layout->addWidget(createSelectionPUI->checkBox());

	// Apply to selection only parameter.
	BooleanParameterUI* applyToSelectionPUI = new BooleanParameterUI(this, PROPERTY_FIELD(SliceModifier::applyToSelection));
	layout->addWidget(applyToSelectionPUI->checkBox());

	layout->addSpacing(8);
	QPushButton* centerPlaneBtn = new QPushButton(tr("Move plane to simulation box center"), rollout);
	connect(centerPlaneBtn, &QPushButton::clicked, this, &SliceModifierEditor::onCenterOfBox);
	layout->addWidget(centerPlaneBtn);

	// Add buttons for view alignment functions.
	QPushButton* alignViewToPlaneBtn = new QPushButton(tr("Align view direction to plane normal"), rollout);
	connect(alignViewToPlaneBtn, &QPushButton::clicked, this, &SliceModifierEditor::onAlignViewToPlane);
	layout->addWidget(alignViewToPlaneBtn);
	QPushButton* alignPlaneToViewBtn = new QPushButton(tr("Align plane normal to view direction"), rollout);
	connect(alignPlaneToViewBtn, &QPushButton::clicked, this, &SliceModifierEditor::onAlignPlaneToView);
	layout->addWidget(alignPlaneToViewBtn);

	_pickParticlePlaneInputMode = new PickParticlePlaneInputMode(this);
	_pickParticlePlaneInputModeAction = new ViewportModeAction(mainWindow(), tr("Pick three particles"), this, _pickParticlePlaneInputMode);
	layout->addWidget(_pickParticlePlaneInputModeAction->createPushButton());

	// Deactivate input mode when editor is reset.
	connect(this, &PropertiesEditor::contentsReplaced, _pickParticlePlaneInputModeAction, &ViewportModeAction::deactivateMode);

	// Status label.
	layout->addSpacing(12);
	layout->addWidget(statusLabel());
}

/******************************************************************************
* Aligns the normal of the slicing plane with the X, Y, or Z axis.
******************************************************************************/
void SliceModifierEditor::onXYZNormal(const QString& link)
{
	SliceModifier* mod = static_object_cast<SliceModifier>(editObject());
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
void SliceModifierEditor::onAlignPlaneToView()
{
	TimeInterval interval;

	Viewport* vp = dataset()->viewportConfig()->activeViewport();
	if(!vp) return;

	// Get the object to world transformation for the currently selected object.
	ObjectNode* node = dynamic_object_cast<ObjectNode>(dataset()->selection()->front());
	if(!node) return;
	const AffineTransformation& nodeTM = node->getWorldTransform(dataset()->animationSettings()->time(), interval);

	// Get the base point of the current slicing plane in local coordinates.
	SliceModifier* mod = static_object_cast<SliceModifier>(editObject());
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
void SliceModifierEditor::onAlignViewToPlane()
{
	TimeInterval interval;

	Viewport* vp = dataset()->viewportConfig()->activeViewport();
	if(!vp) return;

	// Get the object to world transformation for the currently selected object.
	ObjectNode* node = dynamic_object_cast<ObjectNode>(dataset()->selection()->front());
	if(!node) return;
	const AffineTransformation& nodeTM = node->getWorldTransform(dataset()->animationSettings()->time(), interval);

	// Transform the current slicing plane to the world coordinate system.
	SliceModifier* mod = static_object_cast<SliceModifier>(editObject());
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
void SliceModifierEditor::onCenterOfBox()
{
	SliceModifier* mod = static_object_cast<SliceModifier>(editObject());
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

/******************************************************************************
* This is called by the system after the input handler has become the active handler.
******************************************************************************/
void PickParticlePlaneInputMode::activated(bool temporary)
{
	ViewportInputMode::activated(temporary);
	inputManager()->mainWindow()->statusBar()->showMessage(tr("Pick three particles to define a new slicing plane."));
}

/******************************************************************************
* This is called by the system after the input handler is no longer the active handler.
******************************************************************************/
void PickParticlePlaneInputMode::deactivated(bool temporary)
{
	if(!temporary)
		_pickedParticles.clear();
	inputManager()->mainWindow()->statusBar()->clearMessage();
	ViewportInputMode::deactivated(temporary);
}

/******************************************************************************
* Handles the mouse events for a Viewport.
******************************************************************************/
void PickParticlePlaneInputMode::mouseReleaseEvent(ViewportWindow* vpwin, QMouseEvent* event)
{
	if(event->button() == Qt::LeftButton) {

		if(_pickedParticles.size() >= 3) {
			_pickedParticles.clear();
			vpwin->viewport()->dataset()->viewportConfig()->updateViewports();
		}

		PickResult pickResult;
		if(pickParticle(vpwin, event->pos(), pickResult)) {

			// Do not select the same particle twice.
			bool ignore = false;
			if(_pickedParticles.size() >= 1 && _pickedParticles[0].worldPos.equals(pickResult.worldPos, FLOATTYPE_EPSILON)) ignore = true;
			if(_pickedParticles.size() >= 2 && _pickedParticles[1].worldPos.equals(pickResult.worldPos, FLOATTYPE_EPSILON)) ignore = true;

			if(!ignore) {
				_pickedParticles.push_back(pickResult);
				vpwin->viewport()->dataset()->viewportConfig()->updateViewports();

				if(_pickedParticles.size() == 3) {

					// Get the slice modifier that is currently being edited.
					SliceModifier* mod = dynamic_object_cast<SliceModifier>(_editor->editObject());
					if(mod)
						alignPlane(mod);
					_pickedParticles.clear();
				}
			}
		}
	}

	ViewportInputMode::mouseReleaseEvent(vpwin, event);
}

/******************************************************************************
* Aligns the modifier's slicing plane to the three selected particles.
******************************************************************************/
void PickParticlePlaneInputMode::alignPlane(SliceModifier* mod)
{
	OVITO_ASSERT(_pickedParticles.size() == 3);

	try {
		Plane3 worldPlane(_pickedParticles[0].worldPos, _pickedParticles[1].worldPos, _pickedParticles[2].worldPos, true);
		if(worldPlane.normal.equals(Vector3::Zero(), FLOATTYPE_EPSILON))
			mod->throwException(tr("Cannot set the new slicing plane. The three selected particle are colinear."));

		// Get the object to world transformation for the currently selected node.
		ObjectNode* node = _pickedParticles[0].objNode;
		TimeInterval interval;
		const AffineTransformation& nodeTM = node->getWorldTransform(mod->dataset()->animationSettings()->time(), interval);

		// Transform new plane from world to object space.
		Plane3 localPlane = nodeTM.inverse() * worldPlane;

		// Flip new plane orientation if necessary to align it with old orientation.
		if(localPlane.normal.dot(mod->normal()) < 0)
			localPlane = -localPlane;

		localPlane.normalizePlane();
		UndoableTransaction::handleExceptions(mod->dataset()->undoStack(), tr("Align plane to particles"), [mod, &localPlane]() {
			mod->setNormal(localPlane.normal);
			mod->setDistance(localPlane.dist);
		});
	}
	catch(const Exception& ex) {
		ex.reportError();
	}
}

/******************************************************************************
* Lets the input mode render its overlay content in a viewport.
******************************************************************************/
void PickParticlePlaneInputMode::renderOverlay3D(Viewport* vp, ViewportSceneRenderer* renderer)
{
	ViewportInputMode::renderOverlay3D(vp, renderer);

	Q_FOREACH(const PickResult& pa, _pickedParticles) {
		renderSelectionMarker(vp, renderer, pa);
	}
}

/******************************************************************************
* Computes the bounding box of the 3d visual viewport overlay rendered by the input mode.
******************************************************************************/
Box3 PickParticlePlaneInputMode::overlayBoundingBox(Viewport* vp, ViewportSceneRenderer* renderer)
{
	Box3 bbox = ViewportInputMode::overlayBoundingBox(vp, renderer);
	Q_FOREACH(const PickResult& pa, _pickedParticles) {
		bbox.addBox(selectionMarkerBoundingBox(vp, pa));
	}
	return bbox;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
