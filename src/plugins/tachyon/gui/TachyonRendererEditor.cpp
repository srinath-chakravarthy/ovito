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

#include <gui/GUI.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/properties/BooleanGroupBoxParameterUI.h>
#include <gui/properties/IntegerParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/viewport/input/ViewportInputManager.h>
#include <gui/viewport/input/ViewportInputMode.h>
#include <gui/viewport/ViewportWindow.h>
#include <gui/actions/ViewportModeAction.h>
#include <gui/mainwin/MainWindow.h>
#include <plugins/tachyon/renderer/TachyonRenderer.h>
#include "TachyonRendererEditor.h"

namespace Ovito { namespace Tachyon { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(TachyonRendererEditor, PropertiesEditor);
SET_OVITO_OBJECT_EDITOR(TachyonRenderer, TachyonRendererEditor);

/**
 * Viewport input mode that allows to pick a focal length.
 */
class PickFocalLengthInputMode : public ViewportInputMode
{
public:

	/// Constructor.
	PickFocalLengthInputMode(TachyonRendererEditor* editor) : ViewportInputMode(editor), _editor(editor) {}

	/// Handles the mouse move events for a Viewport.
	virtual void mouseMoveEvent(ViewportWindow* vpwin, QMouseEvent* event) override {

		// Change mouse cursor while hovering over an object.
		setCursor(vpwin->pick(event->localPos()) ? SelectionMode::selectionCursor() : QCursor());

		ViewportInputMode::mouseMoveEvent(vpwin, event);
	}

	/// Handles the mouse up events for a Viewport.
	virtual void mouseReleaseEvent(ViewportWindow* vpwin, QMouseEvent* event) override {
		if(event->button() == Qt::LeftButton) {
			ViewportPickResult pickResult = vpwin->pick(event->localPos());
			if(pickResult && vpwin->viewport()->isPerspectiveProjection()) {
				FloatType distance = (pickResult.worldPosition - vpwin->viewport()->cameraPosition()).length();

				if(TachyonRenderer* renderer = static_object_cast<TachyonRenderer>(_editor->editObject())) {
					_editor->undoableTransaction(tr("Set focal length"), [renderer, distance]() {
						renderer->setDofFocalLength(distance);
					});
				}
			}
			inputManager()->removeInputMode(this);
		}
		ViewportInputMode::mouseReleaseEvent(vpwin, event);
	}

protected:

	/// This is called by the system when the input handler has become active.
	virtual void activated(bool temporary) override {
		ViewportInputMode::activated(temporary);
		inputManager()->mainWindow()->statusBar()->showMessage(
				tr("Click on an object in the viewport to set the camera's focal length."));
	}

	/// This is called by the system after the input handler is no longer the active handler.
	virtual void deactivated(bool temporary) override {
		inputManager()->mainWindow()->statusBar()->clearMessage();
		ViewportInputMode::deactivated(temporary);
	}

private:

	TachyonRendererEditor* _editor;
};

/******************************************************************************
* Creates the UI controls for the editor.
******************************************************************************/
void TachyonRendererEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create the rollout.
	QWidget* rollout = createRollout(tr("Tachyon renderer settings"), rolloutParams, "rendering.tachyon_renderer.html");

	QVBoxLayout* mainLayout = new QVBoxLayout(rollout);
	mainLayout->setContentsMargins(4,4,4,4);

	// Antialiasing
	BooleanGroupBoxParameterUI* enableAntialiasingUI = new BooleanGroupBoxParameterUI(this, PROPERTY_FIELD(TachyonRenderer::_antialiasingEnabled));
	QGroupBox* aaGroupBox = enableAntialiasingUI->groupBox();
	mainLayout->addWidget(aaGroupBox);

	QGridLayout* layout = new QGridLayout(enableAntialiasingUI->childContainer());
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1, 1);

	IntegerParameterUI* aaSamplesUI = new IntegerParameterUI(this, PROPERTY_FIELD(TachyonRenderer::_antialiasingSamples));
	layout->addWidget(aaSamplesUI->label(), 0, 0);
	layout->addLayout(aaSamplesUI->createFieldLayout(), 0, 1);

	BooleanGroupBoxParameterUI* enableDirectLightUI = new BooleanGroupBoxParameterUI(this, PROPERTY_FIELD(TachyonRenderer::_directLightSourceEnabled));
	QGroupBox* lightsGroupBox = enableDirectLightUI->groupBox();
	mainLayout->addWidget(lightsGroupBox);

	layout = new QGridLayout(enableDirectLightUI->childContainer());
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1, 1);

	// Default light brightness.
	FloatParameterUI* defaultLightIntensityUI = new FloatParameterUI(this, PROPERTY_FIELD(TachyonRenderer::_defaultLightSourceIntensity));
	defaultLightIntensityUI->label()->setText(tr("Brightness:"));
	layout->addWidget(defaultLightIntensityUI->label(), 0, 0);
	layout->addLayout(defaultLightIntensityUI->createFieldLayout(), 0, 1);

	// Shadows.
	BooleanParameterUI* enableShadowsUI = new BooleanParameterUI(this, PROPERTY_FIELD(TachyonRenderer::_shadowsEnabled));
	layout->addWidget(enableShadowsUI->checkBox(), 1, 0, 1, 2);

	// Ambient occlusion.
	BooleanGroupBoxParameterUI* enableAmbientOcclusionUI = new BooleanGroupBoxParameterUI(this, PROPERTY_FIELD(TachyonRenderer::_ambientOcclusionEnabled));
	QGroupBox* aoGroupBox = enableAmbientOcclusionUI->groupBox();
	mainLayout->addWidget(aoGroupBox);

	layout = new QGridLayout(enableAmbientOcclusionUI->childContainer());
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1, 1);

	// Ambient occlusion brightness.
	FloatParameterUI* aoBrightnessUI = new FloatParameterUI(this, PROPERTY_FIELD(TachyonRenderer::_ambientOcclusionBrightness));
	aoBrightnessUI->label()->setText(tr("Brightness:"));
	layout->addWidget(aoBrightnessUI->label(), 0, 0);
	layout->addLayout(aoBrightnessUI->createFieldLayout(), 0, 1);

	// Ambient occlusion samples.
	IntegerParameterUI* aoSamplesUI = new IntegerParameterUI(this, PROPERTY_FIELD(TachyonRenderer::_ambientOcclusionSamples));
	aoSamplesUI->label()->setText(tr("Sample count:"));
	layout->addWidget(aoSamplesUI->label(), 1, 0);
	layout->addLayout(aoSamplesUI->createFieldLayout(), 1, 1);

	// Depth of field
	BooleanGroupBoxParameterUI* enableDepthOfFieldUI = new BooleanGroupBoxParameterUI(this, PROPERTY_FIELD(TachyonRenderer::_depthOfFieldEnabled));
	QGroupBox* dofGroupBox = enableDepthOfFieldUI->groupBox();
	mainLayout->addWidget(dofGroupBox);

	layout = new QGridLayout(enableDepthOfFieldUI->childContainer());
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1, 1);

	// Focal length
	FloatParameterUI* focalLengthUI = new FloatParameterUI(this, PROPERTY_FIELD(TachyonRenderer::_dofFocalLength));
	layout->addWidget(focalLengthUI->label(), 0, 0);
	layout->addLayout(focalLengthUI->createFieldLayout(), 0, 1);

	// Focal length picking mode.
	ViewportInputMode* pickFocalLengthMode = new PickFocalLengthInputMode(this);
	ViewportModeAction* modeAction = new ViewportModeAction(mainWindow(), tr("Pick in viewport"), this, pickFocalLengthMode);
	layout->addWidget(modeAction->createPushButton(), 0, 2);

	// Aperture
	FloatParameterUI* apertureUI = new FloatParameterUI(this, PROPERTY_FIELD(TachyonRenderer::_dofAperture));
	layout->addWidget(apertureUI->label(), 1, 0);
	layout->addLayout(apertureUI->createFieldLayout(), 1, 1, 1, 2);

	// Copyright notice
	QWidget* copyrightRollout = createRollout(tr("About"), rolloutParams.collapse().after(rollout));
	mainLayout = new QVBoxLayout(copyrightRollout);
	mainLayout->setContentsMargins(4,4,4,4);
	QLabel* label = new QLabel(tr("This rendering plugin is based on:<br>Tachyon Parallel / Multiprocessor Ray Tracing System<br>Copyright 1994-2013 John E. Stone<br><a href=\"http://jedi.ks.uiuc.edu/~johns/raytracer\">See Tachyon website for more information</a>"));
	label->setWordWrap(true);
	label->setOpenExternalLinks(true);
	mainLayout->addWidget(label);
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
