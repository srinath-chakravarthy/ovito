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
#include <plugins/particles/modifier/selection/ManualSelectionModifier.h>
#include <core/viewport/ViewportConfiguration.h>
#include <core/scene/pipeline/ModifierApplication.h>
#include <core/animation/AnimationSettings.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include <gui/actions/ViewportModeAction.h>
#include <gui/mainwin/MainWindow.h>
#include <gui/viewport/ViewportWindow.h>
#include <gui/viewport/input/ViewportInputManager.h>
#include <gui/viewport/input/ViewportInputMode.h>
#include "ManualSelectionModifierEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Selection) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(ManualSelectionModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(ManualSelectionModifier, ManualSelectionModifierEditor);

/**
 * Viewport input mode that allows to pick individual particles and add and remove them
 * from the selection set.
 */
class SelectParticleInputMode : public ViewportInputMode, ParticlePickingHelper
{
public:

	/// Constructor.
	SelectParticleInputMode(ManualSelectionModifierEditor* editor) : ViewportInputMode(editor), _editor(editor) {}

	/// Handles the mouse up events for a Viewport.
	virtual void mouseReleaseEvent(ViewportWindow* vpwin, QMouseEvent* event) override {
		if(event->button() == Qt::LeftButton) {
			PickResult pickResult;
			pickParticle(vpwin, event->pos(), pickResult);
			if(pickResult.objNode) {
				_editor->onParticlePicked(pickResult);
			}
			else {
				inputManager()->mainWindow()->statusBar()->showMessage(tr("You did not click on a particle."), 1000);
			}
		}
		ViewportInputMode::mouseReleaseEvent(vpwin, event);
	}

	ManualSelectionModifierEditor* _editor;
};

/**
 * Viewport input mode that allows to select a group of particles
 * by drawing a fence around them.
 */
class FenceParticleInputMode : public ViewportInputMode
{
public:

	/// Constructor.
	FenceParticleInputMode(ManualSelectionModifierEditor* editor) : ViewportInputMode(editor), _editor(editor) {}

	/// Handles the mouse down events for a Viewport.
	virtual void mousePressEvent(ViewportWindow* vpwin, QMouseEvent* event) override {
		_fence.clear();
		if(event->button() == Qt::LeftButton) {
			_fence.push_back(Point2(event->localPos().x(), event->localPos().y())
					* (FloatType)vpwin->devicePixelRatio());
			vpwin->viewport()->updateViewport();
		}
		else ViewportInputMode::mousePressEvent(vpwin, event);
	}

	/// Handles the mouse move events for a Viewport.
	virtual void mouseMoveEvent(ViewportWindow* vpwin, QMouseEvent* event) override {
		if(!_fence.isEmpty()) {
			_fence.push_back(Point2(event->localPos().x(), event->localPos().y())
					* (FloatType)vpwin->devicePixelRatio());
			vpwin->viewport()->updateViewport();
		}
		ViewportInputMode::mouseMoveEvent(vpwin, event);
	}

	/// Handles the mouse up events for a Viewport.
	virtual void mouseReleaseEvent(ViewportWindow* vpwin, QMouseEvent* event) override {
		if(!_fence.isEmpty()) {
			if(_fence.size() >= 3) {
				ParticleSelectionSet::SelectionMode mode = ParticleSelectionSet::SelectionReplace;
				if(event->modifiers().testFlag(Qt::ControlModifier))
					mode = ParticleSelectionSet::SelectionAdd;
				else if(event->modifiers().testFlag(Qt::AltModifier))
					mode = ParticleSelectionSet::SelectionSubtract;
				_editor->onFence(_fence, vpwin->viewport(), mode);
			}
			_fence.clear();
			vpwin->viewport()->updateViewport();
		}
		ViewportInputMode::mouseReleaseEvent(vpwin, event);
	}

	/// Indicates whether this input mode renders 3d geometry into the viewports.
	virtual bool hasOverlay() override { return true; }

	/// Lets the input mode render its 2d overlay content in a viewport.
	virtual void renderOverlay2D(Viewport* vp, ViewportSceneRenderer* renderer) override {
		if(isActive() && vp == vp->dataset()->viewportConfig()->activeViewport() && _fence.size() >= 2) {
			renderer->render2DPolyline(_fence.constData(), _fence.size(), ViewportSettings::getSettings().viewportColor(ViewportSettings::COLOR_SELECTION), true);
		}
		ViewportInputMode::renderOverlay2D(vp, renderer);
	}

protected:

	/// This is called by the system when the input handler has become active.
	virtual void activated(bool temporary) override {
		ViewportInputMode::activated(temporary);
#ifndef Q_OS_MACX
		inputManager()->mainWindow()->statusBar()->showMessage(
				tr("Draw a fence around a group of particles. Use CONTROL and ALT keys to extend and reduce existing selection."));
#else
		inputManager()->mainWindow()->statusBar()->showMessage(
				tr("Draw a fence around a group of particles. Use COMMAND and ALT keys to extend and reduce existing selection."));
#endif
	}

	/// This is called by the system after the input handler is no longer the active handler.
	virtual void deactivated(bool temporary) override {
		_fence.clear();
		inputManager()->mainWindow()->statusBar()->clearMessage();
		ViewportInputMode::deactivated(temporary);
	}

private:

	ManualSelectionModifierEditor* _editor;
	QVector<Point2> _fence;
};

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void ManualSelectionModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	QWidget* rollout = createRollout(tr("Manual particle selection"), rolloutParams, "particles.modifiers.manual_selection.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(6);

	QGroupBox* mouseSelectionGroup = new QGroupBox(tr("Viewport modes"));
	QVBoxLayout* sublayout = new QVBoxLayout(mouseSelectionGroup);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(6);
	layout->addWidget(mouseSelectionGroup);

	ViewportInputMode* selectParticleMode = new SelectParticleInputMode(this);
	ViewportModeAction* pickModeAction = new ViewportModeAction(mainWindow(), tr("Pick particles"), this, selectParticleMode);
	sublayout->addWidget(pickModeAction->createPushButton());

	ViewportInputMode* fenceParticleMode = new FenceParticleInputMode(this);
	ViewportModeAction* fenceModeAction = new ViewportModeAction(mainWindow(), tr("Fence selection"), this, fenceParticleMode);
	sublayout->addWidget(fenceModeAction->createPushButton());

	// Deactivate input modes when editor is reset.
	connect(this, &PropertiesEditor::contentsReplaced, pickModeAction, &ViewportModeAction::deactivateMode);
	connect(this, &PropertiesEditor::contentsReplaced, fenceModeAction, &ViewportModeAction::deactivateMode);

	QGroupBox* globalSelectionGroup = new QGroupBox(tr("Actions"));
	sublayout = new QVBoxLayout(globalSelectionGroup);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(6);
	layout->addWidget(globalSelectionGroup);

	QPushButton* selectAllBtn = new QPushButton(tr("Select all particles"));
	connect(selectAllBtn, &QPushButton::clicked, this, &ManualSelectionModifierEditor::selectAll);
	sublayout->addWidget(selectAllBtn);

	QPushButton* clearSelectionBtn = new QPushButton(tr("Clear selection"));
	connect(clearSelectionBtn, &QPushButton::clicked, this, &ManualSelectionModifierEditor::clearSelection);
	sublayout->addWidget(clearSelectionBtn);

	QPushButton* resetSelectionBtn = new QPushButton(tr("Reset selection"));
	connect(resetSelectionBtn, &QPushButton::clicked, this, &ManualSelectionModifierEditor::resetSelection);
	sublayout->addWidget(resetSelectionBtn);

	// Status label.
	layout->addSpacing(12);
	layout->addWidget(statusLabel());
}

/******************************************************************************
* Adopts the selection state from the modifier's input.
******************************************************************************/
void ManualSelectionModifierEditor::resetSelection()
{
	ManualSelectionModifier* mod = static_object_cast<ManualSelectionModifier>(editObject());
	if(!mod) return;

	undoableTransaction(tr("Reset selection"), [mod]() {
		for(const auto& modInput : mod->getModifierInputs(mod->dataset()->animationSettings()->time()))
			mod->resetSelection(modInput.first, modInput.second);
	});
}

/******************************************************************************
* Selects all particles.
******************************************************************************/
void ManualSelectionModifierEditor::selectAll()
{
	ManualSelectionModifier* mod = static_object_cast<ManualSelectionModifier>(editObject());
	if(!mod) return;

	undoableTransaction(tr("Select all"), [mod]() {
		for(const auto& modInput : mod->getModifierInputs(mod->dataset()->animationSettings()->time()))
			mod->selectAll(modInput.first, modInput.second);
	});
}

/******************************************************************************
* Clears the selection.
******************************************************************************/
void ManualSelectionModifierEditor::clearSelection()
{
	ManualSelectionModifier* mod = static_object_cast<ManualSelectionModifier>(editObject());
	if(!mod) return;

	undoableTransaction(tr("Clear selection"), [mod]() {
		for(const auto& modInput : mod->getModifierInputs(mod->dataset()->animationSettings()->time()))
			mod->clearSelection(modInput.first, modInput.second);
	});
}

/******************************************************************************
* This is called when the user has selected a particle.
******************************************************************************/
void ManualSelectionModifierEditor::onParticlePicked(const ParticlePickingHelper::PickResult& pickResult)
{
	ManualSelectionModifier* mod = static_object_cast<ManualSelectionModifier>(editObject());
	if(!mod) return;

	undoableTransaction(tr("Toggle particle selection"), [mod, &pickResult]() {
		for(const auto& modInput : mod->getModifierInputs(mod->dataset()->animationSettings()->time())) {

			// Lookup the right particle in the modifier's input.
			// Since we cannot rely on the particle's index or identifier, we use the
			// particle location to unambiguously find the picked particle.
			ParticlePropertyObject* posProperty = ParticlePropertyObject::findInState(modInput.second, ParticleProperty::PositionProperty);
			if(!posProperty) continue;

			size_t index = 0;
			for(const Point3& p : posProperty->constPoint3Range()) {
				if(p == pickResult.localPos) {
					mod->toggleParticleSelection(modInput.first, modInput.second, index);
					break;
				}
				index++;
			}
		}
	});
}

/******************************************************************************
* This is called when the user has drawn a fence around particles.
******************************************************************************/
void ManualSelectionModifierEditor::onFence(const QVector<Point2>& fence, Viewport* viewport, ParticleSelectionSet::SelectionMode mode)
{
	ManualSelectionModifier* mod = static_object_cast<ManualSelectionModifier>(editObject());
	if(!mod) return;

	undoableTransaction(tr("Select particles"), [mod, &fence, viewport, mode]() {
		for(const auto& modInput : mod->getModifierInputs(mod->dataset()->animationSettings()->time())) {

			// Lookup the right particle in the modifier's input.
			// Since we cannot rely on the particle's index or identifier, we use the
			// particle location to unambiguously find the picked particle.
			ParticlePropertyObject* posProperty = ParticlePropertyObject::findInState(modInput.second, ParticleProperty::PositionProperty);
			if(!posProperty) continue;

			for(ObjectNode* node : modInput.first->objectNodes()) {

				// Create projection matrix that transforms particle positions to screen space.
				TimeInterval interval;
				const AffineTransformation& nodeTM = node->getWorldTransform(mod->dataset()->animationSettings()->time(), interval);
				Matrix4 ndcToScreen = Matrix4::Identity();
				ndcToScreen(0,0) = 0.5 * viewport->windowSize().width();
				ndcToScreen(1,1) = 0.5 * viewport->windowSize().height();
				ndcToScreen(0,3) = ndcToScreen(0,0);
				ndcToScreen(1,3) = ndcToScreen(1,1);
				ndcToScreen(1,1) = -ndcToScreen(1,1);	// Vertical flip.
				Matrix4 tm = ndcToScreen * viewport->projectionParams().projectionMatrix * (viewport->projectionParams().viewMatrix * nodeTM);

				// Determine which particles are within the closed fence polygon.
				QBitArray fullSelection(posProperty->size());
				QMutex mutex;
				parallelForChunks(posProperty->size(), [posProperty, tm, &fence, &mutex, &fullSelection](size_t startIndex, size_t chunkSize) {
					QBitArray selection(posProperty->size());
					for(int index = startIndex; chunkSize != 0; chunkSize--, index++) {

						// Project particle center to screen coordinates.
						Point3 projPos = tm * posProperty->getPoint3(index);

						// Perform z-clipping.
						if(std::fabs(projPos.z()) >= 1.0f)
							continue;

						// Perform point in polygon test.
						int intersectionsLeft = 0;
						int intersectionsRight = 0;
						for(auto p2 = fence.constBegin(), p1 = p2 + (fence.size()-1); p2 != fence.constEnd(); p1 = p2++) {
							if(p1->y() == p2->y()) continue;
							if(projPos.y() >= p1->y() && projPos.y() >= p2->y()) continue;
							if(projPos.y() < p1->y() && projPos.y() < p2->y()) continue;
							FloatType xint = (projPos.y() - p2->y()) / (p1->y() - p2->y()) * (p1->x() - p2->x()) + p2->x();
							if(xint >= projPos.x())
								intersectionsRight++;
							else
								intersectionsLeft++;
						}
						if(intersectionsRight & 1)
							selection.setBit(index);
					}
					// Transfer thread-local results to output bit array.
					QMutexLocker locker(&mutex);
					fullSelection |= selection;
				});

				mod->setParticleSelection(modInput.first, modInput.second, fullSelection, mode);
				break;
			}
		}
	});
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
