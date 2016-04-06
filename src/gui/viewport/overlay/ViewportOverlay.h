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

#ifndef __OVITO_VIEWPORT_OVERLAY_H
#define __OVITO_VIEWPORT_OVERLAY_H

#include <core/Core.h>
#include <core/reference/RefTarget.h>
#include <core/scene/pipeline/PipelineStatus.h>
#include <core/viewport/input/ViewportInputMode.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(View)

/**
 * \brief Abstract base class for all viewport overlays.
 */
class OVITO_CORE_EXPORT ViewportOverlay : public RefTarget
{
protected:

	/// \brief Constructor.
	ViewportOverlay(DataSet* dataset);

public:

	/// \brief This method asks the overlay to paint its contents over the given viewport.
	virtual void render(Viewport* viewport, QPainter& painter, const ViewProjectionParameters& projParams, RenderSettings* renderSettings) = 0;

	/// \brief Returns the status of the object, which may indicate an error condition.
	///
	/// The default implementation of this method returns an empty status object.
	/// The object should generate a ReferenceEvent::ObjectStatusChanged event when its status changes.
	virtual PipelineStatus status() const { return PipelineStatus(); }

	/// \brief Moves the position of the overlay in the viewport by the given amount,
	///        which is specified as a fraction of the viewport render size.
	///
	/// Overlay implementations should override this method if they support positioning.
	/// The default method implementation does nothing.
	virtual void moveOverlayInViewport(const Vector2& delta) {};

private:

	Q_OBJECT
	OVITO_OBJECT
};


/**
 * Viewport mouse input mode, which allows the user to interactively move a viewport overlay
 * using the mouse.
 */
class OVITO_CORE_EXPORT MoveOverlayInputMode : public ViewportInputMode
{
public:

	/// Constructor.
	MoveOverlayInputMode(PropertiesEditor* editor);

	/// \brief This is called by the system after the input handler is
	///        no longer the active handler.
	virtual void deactivated(bool temporary) override;

	/// Handles the mouse down events for a Viewport.
	virtual void mousePressEvent(Viewport* vp, QMouseEvent* event) override;

	/// Handles the mouse move events for a Viewport.
	virtual void mouseMoveEvent(Viewport* vp, QMouseEvent* event) override;

	/// Handles the mouse up events for a Viewport.
	virtual void mouseReleaseEvent(Viewport* vp, QMouseEvent* event) override;

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

#endif // __OVITO_VIEWPORT_OVERLAY_H
