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

#pragma once


#include <plugins/particles/gui/ParticlesGui.h>
#include <plugins/particles/gui/modifier/ParticleModifierEditor.h>
#include <plugins/particles/gui/util/ParticlePickingHelper.h>
#include <plugins/particles/modifier/modify/SliceModifier.h>
#include <gui/viewport/input/ViewportInputMode.h>
#include <gui/viewport/input/ViewportInputManager.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

class PickParticlePlaneInputMode;	// Defined below

/**
 * A properties editor for the SliceModifier class.
 */
class SliceModifierEditor : public ParticleModifierEditor
{
public:

	/// Default constructor.
	Q_INVOKABLE SliceModifierEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

protected Q_SLOTS:

	/// Aligns the slicing plane to the viewing direction.
	void onAlignPlaneToView();

	/// Aligns the current viewing direction to the slicing plane.
	void onAlignViewToPlane();

	/// Aligns the normal of the slicing plane with the X, Y, or Z axis.
	void onXYZNormal(const QString& link);

	/// Moves the plane to the center of the simulation box.
	void onCenterOfBox();

private:

	PickParticlePlaneInputMode* _pickParticlePlaneInputMode;
	ViewportModeAction* _pickParticlePlaneInputModeAction;

	Q_OBJECT
	OVITO_OBJECT
};

/******************************************************************************
* The viewport input mode that lets the user select three particles
* to define the slicing plane.
******************************************************************************/
class PickParticlePlaneInputMode : public ViewportInputMode, ParticlePickingHelper
{
public:

	/// Constructor.
	PickParticlePlaneInputMode(SliceModifierEditor* editor) : ViewportInputMode(editor), _editor(editor) {}

	/// Handles the mouse events for a Viewport.
	virtual void mouseReleaseEvent(ViewportWindow* vpwin, QMouseEvent* event) override;

	/// Lets the input mode render its overlay content in a viewport.
	virtual void renderOverlay3D(Viewport* vp, ViewportSceneRenderer* renderer) override;

	/// Computes the bounding box of the 3d visual viewport overlay rendered by the input mode.
	virtual Box3 overlayBoundingBox(Viewport* vp, ViewportSceneRenderer* renderer) override;

	/// Indicates whether this input mode renders into the viewports.
	virtual bool hasOverlay() override { return true; }

protected:

	/// This is called by the system after the input handler has become the active handler.
	virtual void activated(bool temporary) override;

	/// This is called by the system after the input handler is no longer the active handler.
	virtual void deactivated(bool temporary) override;

private:

	/// Aligns the modifier's slicing plane to the three selected particles.
	void alignPlane(SliceModifier* mod);

	/// The list of particles picked by the user so far.
	QVector<PickResult> _pickedParticles;

	/// The properties editor of the Slice modifier.
	SliceModifierEditor* _editor;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


