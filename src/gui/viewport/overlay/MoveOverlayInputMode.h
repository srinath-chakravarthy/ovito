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


#include <gui/GUI.h>
#include <core/reference/RefTarget.h>
#include <core/scene/pipeline/PipelineStatus.h>
#include <gui/viewport/input/ViewportInputMode.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(View)

/**
 * Viewport mouse input mode, which allows the user to interactively move a viewport overlay
 * using the mouse.
 */
class OVITO_GUI_EXPORT MoveOverlayInputMode : public ViewportInputMode
{
public:

	/// Constructor.
	MoveOverlayInputMode(PropertiesEditor* editor);

	/// \brief This is called by the system after the input handler is
	///        no longer the active handler.
	virtual void deactivated(bool temporary) override;

	/// Handles the mouse down events for a Viewport.
	virtual void mousePressEvent(ViewportWindow* vpwin, QMouseEvent* event) override;

	/// Handles the mouse move events for a Viewport.
	virtual void mouseMoveEvent(ViewportWindow* vpwin, QMouseEvent* event) override;

	/// Handles the mouse up events for a Viewport.
	virtual void mouseReleaseEvent(ViewportWindow* vpwin, QMouseEvent* event) override;

	/// Returns the current viewport we are working in.
	Viewport* viewport() const { return _viewport; }

private:

	/// The current viewport we are working in.
	Viewport* _viewport = nullptr;

	/// The properties editor of the viewport overlay being moved.
	PropertiesEditor* _editor;

	/// Mouse position at first click.
	QPointF _startPoint;

	/// The current mouse position.
	QPointF _currentPoint;

	/// The cursor shown when the overlay can be moved.
	QCursor _moveCursor;

	/// The cursor shown when in the wrong viewport.
	QCursor _forbiddenCursor;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace


