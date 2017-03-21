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

#pragma once


#include <gui/GUI.h>
#include <core/rendering/FrameBuffer.h>
#include "FrameBufferWidget.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Widgets)

/**
 * This window displays the contents of a FrameBuffer.
 */
class OVITO_GUI_EXPORT FrameBufferWindow : public QMainWindow
{
public:

	/// Constructor.
	FrameBufferWindow(QWidget* parent = nullptr);

	/// Return the FrameBuffer that is currently shown in the widget (can be NULL).
	const std::shared_ptr<FrameBuffer>& frameBuffer() const { return _frameBufferWidget->frameBuffer(); }

	/// Sets the FrameBuffer that is currently shown in the widget.
	void setFrameBuffer(const std::shared_ptr<FrameBuffer>& frameBuffer) { _frameBufferWidget->setFrameBuffer(frameBuffer); }

	/// Creates a frame buffer of the requested size and adjusts the size of the window.
	const std::shared_ptr<FrameBuffer>& createFrameBuffer(int w, int h);

	/// Shows and activates the frame buffer window.
	void showAndActivateWindow();

public Q_SLOTS:

	/// This opens the file dialog and lets the user save the current contents of the frame buffer
	/// to an image file.
	void saveImage();

	/// This copies the current image to the clipboard.
	void copyImageToClipboard();

	/// Removes unnecessary pixels at the outer edges of the rendered image.
	void autoCrop();

private:

	/// The widget that displays the FrameBuffer.
	FrameBufferWidget* _frameBufferWidget;

private:

	Q_OBJECT
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


