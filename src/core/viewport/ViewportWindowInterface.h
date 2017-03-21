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


#include <core/Core.h>

class QOpenGLContext;	// Defined in QtGui headers

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * \brief Abstract interface for viewport windows, which provide the connection between the Viewport
 *        class and the GUI.
 */
class OVITO_CORE_EXPORT ViewportWindowInterface
{
public:

    /// Puts an update request event for this window on the event loop.
	virtual void renderLater() = 0;

	/// Immediately redraws the contents of this window.
	virtual void renderNow() = 0;

	/// If an update request is pending for this viewport window, immediately
	/// processes it and redraw the window contents.
	virtual void processViewportUpdate() = 0;

	/// Returns the current size of the viewport window (in device pixels).
	virtual QSize viewportWindowDeviceSize() = 0;

	/// Returns the current size of the viewport window (in device-independent pixels).
	virtual QSize viewportWindowDeviceIndependentSize() = 0;

	/// Lets the viewport window delete itself.
	/// This is called by the Viewport class destructor.
	virtual void destroyViewportWindow() = 0;

	/// Renders custom GUI elements in the viewport on top of the scene.
	virtual void renderGui() = 0;

	/// Provides access to the OpenGL context used by the viewport window for rendering.
	virtual QOpenGLContext* glcontext() = 0;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


