///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2014) Alexander Stukowski
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

#include <core/Core.h>
#include <core/viewport/Viewport.h>
#include <core/viewport/input/ViewportInputManager.h>
#include <core/viewport/overlay/ViewportOverlay.h>
#include <core/gui/mainwin/MainWindow.h>
#include <core/gui/properties/PropertiesEditor.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(View)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(Core, ViewportOverlay, RefTarget);

/******************************************************************************
* Constructor.
******************************************************************************/
ViewportOverlay::ViewportOverlay(DataSet* dataset) : RefTarget(dataset)
{
}

/******************************************************************************
* Constructor.
******************************************************************************/
MoveOverlayInputMode::MoveOverlayInputMode(PropertiesEditor* editor) :
		ViewportInputMode(editor),
		_editor(editor),
		_moveCursor(QCursor(QPixmap(QStringLiteral(":/core/cursor/editing/cursor_mode_move.png")))),
		_forbiddenCursor(Qt::ForbiddenCursor)
{
}

/******************************************************************************
* This is called by the system after the input handler is
* no longer the active handler.
******************************************************************************/
void MoveOverlayInputMode::deactivated(bool temporary)
{
	if(_viewport) {
		// Restore old state if change has not been committed.
		_viewport->dataset()->undoStack().endCompoundOperation(false);
		_viewport = nullptr;
	}
	ViewportInputMode::deactivated(temporary);
}

/******************************************************************************
* Handles the mouse down events for a Viewport.
******************************************************************************/
void MoveOverlayInputMode::mousePressEvent(Viewport* vp, QMouseEvent* event)
{
	if(event->button() == Qt::LeftButton) {
		if(_viewport == nullptr) {
			ViewportOverlay* overlay = dynamic_object_cast<ViewportOverlay>(_editor->editObject());
			if(overlay && vp->overlays().contains(overlay)) {
				_viewport = vp;
				_startPoint = event->localPos();
				vp->dataset()->undoStack().beginCompoundOperation(tr("Move overlay"));
			}
		}
		return;
	}
	else if(event->button() == Qt::RightButton) {
		if(_viewport != nullptr) {
			// Restore old state when aborting the move operation.
			_viewport->dataset()->undoStack().endCompoundOperation(false);
			_viewport = nullptr;
			return;
		}
	}
	ViewportInputMode::mousePressEvent(vp, event);
}

/******************************************************************************
* Handles the mouse move events for a Viewport.
******************************************************************************/
void MoveOverlayInputMode::mouseMoveEvent(Viewport* vp, QMouseEvent* event)
{
	// Get the viewport overlay being moved.
	ViewportOverlay* overlay = dynamic_object_cast<ViewportOverlay>(_editor->editObject());
	if(overlay && vp->overlays().contains(overlay)) {
		setCursor(_moveCursor);

		if(viewport() == vp) {
			// Take the current mouse cursor position to make the input mode
			// look more responsive. The cursor position recorded when the mouse event was
			// generates may be too old.
			_currentPoint = vp->widget()->mapFromGlobal(QCursor::pos());

			// Reset the overlay's position first before moving it again below.
			vp->dataset()->undoStack().resetCurrentCompoundOperation();

			// Compute the displacement based on the new mouse position.
			Box2 renderFrameRect = vp->renderFrameRect();
			QSize vpSize = vp->size();
			Vector2 delta;
			delta.x() =  (FloatType)(_currentPoint.x() - _startPoint.x()) / vpSize.width() / renderFrameRect.width() * 2;
			delta.y() = -(FloatType)(_currentPoint.y() - _startPoint.y()) / vpSize.height() / renderFrameRect.height() * 2;

			// Move the overlay.
			try {
				overlay->moveOverlayInViewport(delta);
			}
			catch(const Exception& ex) {
				inputManager()->removeInputMode(this);
				ex.showError();
			}

			// Force immediate viewport repaints.
			vp->dataset()->mainWindow()->processViewportUpdates();
		}
	}
	else {
		setCursor(_forbiddenCursor);
	}
	ViewportInputMode::mouseMoveEvent(vp, event);
}

/******************************************************************************
* Handles the mouse up events for a Viewport.
******************************************************************************/
void MoveOverlayInputMode::mouseReleaseEvent(Viewport* vp, QMouseEvent* event)
{
	if(_viewport) {
		// Commit change.
		_viewport->dataset()->undoStack().endCompoundOperation();
		_viewport = nullptr;
	}
	ViewportInputMode::mouseReleaseEvent(vp, event);
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
