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
#include <core/viewport/ViewportWindowInterface.h>
#include <core/rendering/TextPrimitive.h>
#include <core/rendering/ImagePrimitive.h>
#include <core/rendering/LinePrimitive.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/******************************************************************************
* This data structure is returned by the Viewport::pick() method.
*******************************************************************************/
struct OVITO_GUI_EXPORT ViewportPickResult
{
	/// Indicates whether an object was picked.
	explicit operator bool() const { return objectNode != nullptr; }

	/// The object node that was picked.
	OORef<ObjectNode> objectNode;

	/// The object-specific information attached to the pick record.
	OORef<ObjectPickInfo> pickInfo;

	/// The coordinates of the hit point in world space.
	Point3 worldPosition;

	/// The subobject that was picked.
	quint32 subobjectId = 0;
};

/**
 * \brief The internal render window/widget used by the Viewport class.
 */
class OVITO_GUI_EXPORT ViewportWindow : public QOpenGLWidget, public ViewportWindowInterface
{
public:

	/// Constructor.
	ViewportWindow(Viewport* owner, QWidget* parentWidget);

	/// Destructor.
	virtual ~ViewportWindow();

	/// Returns the owning viewport of this window.
	Viewport* viewport() const { return _viewport; }

    /// \brief Puts an update request event for this window on the event loop.
	virtual void renderLater() override;

	/// \brief Immediately redraws the contents of this window.
	virtual void renderNow() override;

	/// If an update request is pending for this viewport window, immediately
	/// processes it and redraw the window contents.
	virtual void processViewportUpdate() override;

	/// Returns the current size of the viewport window (in device pixels).
	virtual QSize viewportWindowDeviceSize() override {
#if QT_VERSION < QT_VERSION_CHECK(5, 6, 0)
		return size() * devicePixelRatio();
#else
		return size() * devicePixelRatioF();
#endif
	}

	/// Returns the current size of the viewport window (in device-independent pixels).
	virtual QSize viewportWindowDeviceIndependentSize() override {
		return size();
	}

	/// Lets the viewport window delete itself.
	/// This is called by the Viewport class destructor.
	virtual void destroyViewportWindow() override {
		// Detach from viewport.
		_viewport = nullptr;
		deleteLater();
	}

	/// Renders custom GUI elements in the viewport on top of the scene.
	virtual void renderGui() override;

	/// Provides access to the OpenGL context used by the viewport window for rendering.
	virtual QOpenGLContext* glcontext() override { return this->context(); }

	/// If the return value is true, the viewport window receives all mouse events until
	/// setMouseGrabEnabled(false) is called; other windows get no mouse events at all.
	bool setMouseGrabEnabled(bool grab);

	/// \brief Determines the object that is visible under the given mouse cursor position.
	ViewportPickResult pick(const QPointF& pos);

	/// \brief Displays the context menu for the viewport.
	/// \param pos The position in where the context menu should be displayed.
	void showViewportMenu(const QPoint& pos = QPoint(0,0));

protected:

	/// Is called whenever the widget needs to be painted.
	virtual void paintGL() override;

	/// Is called whenever the GL context needs to be initialized.
	virtual void initializeGL() override;
	
	/// Is called when the mouse cursor leaves the widget.
	virtual void leaveEvent(QEvent* event) override;

	/// Is called when the viewport becomes visible.
	virtual void showEvent(QShowEvent* event) override;

	/// Handles double click events.
	virtual void mouseDoubleClickEvent(QMouseEvent* event) override;

	/// Handles mouse press events.
	virtual void mousePressEvent(QMouseEvent* event) override;

	/// Handles mouse release events.
	virtual void mouseReleaseEvent(QMouseEvent* event) override;

	/// Handles mouse move events.
	virtual void mouseMoveEvent(QMouseEvent* event) override;

	/// Handles mouse wheel events.
	virtual void wheelEvent(QWheelEvent* event) override;

	/// Is called when the widgets looses the input focus.
	virtual void focusOutEvent(QFocusEvent* event) override;

private:

	/// Renders the contents of the viewport window.
	void renderViewport();

	/// Renders the viewport caption text.
	void renderViewportTitle();

	/// Render the axis tripod symbol in the corner of the viewport that indicates
	/// the coordinate system orientation.
	void renderOrientationIndicator();

	/// Renders the frame on top of the scene that indicates the visible rendering area.
	void renderRenderFrame();

private:

	/// The owning viewport of this window.
	Viewport* _viewport;

	/// A flag that indicates that a viewport update has been requested.
	bool _updateRequested;

	/// The zone in the upper left corner of the viewport where
	/// the context menu can be activated by the user.
	QRect _contextMenuArea;

	/// Indicates that the mouse cursor is currently positioned inside the
	/// viewport area that activates the viewport context menu.
	bool _cursorInContextMenuArea;

	/// The parent window of this viewport window.
	MainWindow* _mainWindow;

	/// Counts how often this viewport has been rendered.
	int _renderDebugCounter;

	/// The rendering buffer maintained to render the viewport's caption text.
	std::shared_ptr<TextPrimitive> _captionBuffer;

	/// The geometry buffer used to render the viewport's orientation indicator.
	std::shared_ptr<LinePrimitive> _orientationTripodGeometry;

	/// The rendering buffer used to render the viewport's orientation indicator labels.
	std::shared_ptr<TextPrimitive> _orientationTripodLabels[3];

	/// This is used to render the render frame around the viewport.
	std::shared_ptr<ImagePrimitive> _renderFrameOverlay;

	/// This is the renderer of the interactive viewport.
	OORef<ViewportSceneRenderer> _viewportRenderer;

	/// This renderer generates an offscreen rendering of the scene that allows picking of objects.
	OORef<PickingSceneRenderer> _pickingRenderer;

private:

	Q_OBJECT
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


