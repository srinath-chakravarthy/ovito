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

#include <plugins/pyscript/PyScript.h>
#include <core/viewport/ViewportConfiguration.h>
#include <core/viewport/Viewport.h>
#include <core/viewport/overlay/ViewportOverlay.h>
#include <core/viewport/overlay/CoordinateTripodOverlay.h>
#include <core/scene/SceneNode.h>
#include <plugins/pyscript/extensions/PythonViewportOverlay.h>
#include "PythonBinding.h"

namespace PyScript {

using namespace boost::python;
using namespace Ovito;

BOOST_PYTHON_MODULE(PyScriptViewport)
{
	docstring_options docoptions(true, false);

	{
		scope s = ovito_class<Viewport, RefTarget>(
				"A viewport defines the view on the three-dimensional scene. "
				"\n\n"
				"You can create an instance of this class to define a camera position from which "
				"a picture of the three-dimensional scene should be generated. After the camera "
				"has been set up, you can render an image or movie using the viewport's "
				":py:meth:`.render` method::"
				"\n\n"
				"    vp = Viewport()\n"
				"    vp.type = Viewport.Type.PERSPECTIVE\n"
				"    vp.camera_pos = (100, 50, 50)\n"
				"    vp.camera_dir = (-100, -50, -50)\n"
				"\n"
				"    rs = RenderSettings(size=(800,600), filename=\"image.png\")\n"
				"    vp.render(rs)\n"
				"\n"
				"Note that the four interactive viewports in OVITO's main window are instances of this class. If you want to "
				"manipulate these existing viewports, you can access them through the "
				":py:attr:`DataSet.viewports <ovito.DataSet.viewports>` attribute.")
			.add_property("isRendering", &Viewport::isRendering)
			.add_property("isPerspective", &Viewport::isPerspectiveProjection)
            .add_property("type", &Viewport::viewType, lambda_address([](Viewport& vp, Viewport::ViewType vt) { vp.setViewType(vt); }),
					"The type of projection:"
					"\n\n"
					"  * ``Viewport.Type.PERSPECTIVE``\n"
					"  * ``Viewport.Type.ORTHO``\n"
					"  * ``Viewport.Type.TOP``\n"
					"  * ``Viewport.Type.BOTTOM``\n"
					"  * ``Viewport.Type.FRONT``\n"
					"  * ``Viewport.Type.BACK``\n"
					"  * ``Viewport.Type.LEFT``\n"
					"  * ``Viewport.Type.RIGHT``\n"
					"  * ``Viewport.Type.NONE``\n"
					"\n"
					"The first two types (``PERSPECTIVE`` and ``ORTHO``) allow you to set up custom views with arbitrary camera orientation.\n")
			.add_property("fov", &Viewport::fieldOfView, &Viewport::setFieldOfView,
					"The field of view of the viewport's camera. "
					"For perspective projections this is the camera's angle in the vertical direction (in radians). For orthogonal projections this is the visible range in the vertical direction (in world units).")
			.add_property("cameraTransformation", make_function(&Viewport::cameraTransformation, return_value_policy<copy_const_reference>()), &Viewport::setCameraTransformation)
			.add_property("camera_dir", &Viewport::cameraDirection, &Viewport::setCameraDirection,
					"The viewing direction vector of the viewport's camera. This can be an arbitrary vector with non-zero length.")
			.add_property("camera_pos", &Viewport::cameraPosition, &Viewport::setCameraPosition,
					"\nThe position of the viewport's camera. For example, to move the camera of the active viewport in OVITO's main window to a new location in space::"
					"\n\n"
					"    dataset.viewports.active_vp.camera_pos = (100, 80, -30)\n"
					"\n\n")
			.add_property("viewMatrix", make_function(lambda_address([](Viewport& vp) -> const AffineTransformation& { return vp.projectionParams().viewMatrix; }), return_value_policy<copy_const_reference>()))
			.add_property("inverseViewMatrix", make_function(lambda_address([](Viewport& vp) -> const AffineTransformation& { return vp.projectionParams().inverseViewMatrix; }), return_value_policy<copy_const_reference>()))
			.add_property("projectionMatrix", make_function(lambda_address([](Viewport& vp) -> const Matrix4& { return vp.projectionParams().projectionMatrix; }), return_value_policy<copy_const_reference>()))
			.add_property("inverseProjectionMatrix", make_function(lambda_address([](Viewport& vp) -> const Matrix4& { return vp.projectionParams().inverseProjectionMatrix; }), return_value_policy<copy_const_reference>()))
			.add_property("renderPreviewMode", &Viewport::renderPreviewMode, &Viewport::setRenderPreviewMode)
			.add_property("gridVisible", &Viewport::isGridVisible, &Viewport::setGridVisible)
			.add_property("viewNode", make_function(&Viewport::viewNode, return_value_policy<ovito_object_reference>()), &Viewport::setViewNode)
			.add_property("gridMatrix", make_function(&Viewport::gridMatrix, return_value_policy<copy_const_reference>()), &Viewport::setGridMatrix)
			.add_property("title", make_function(&Viewport::viewportTitle, return_value_policy<copy_const_reference>()),
					"The title string of the viewport shown in its top left corner (read-only).")
			.def("updateViewport", &Viewport::updateViewport)
			.def("redrawViewport", &Viewport::redrawViewport)
			.def("nonScalingSize", &Viewport::nonScalingSize)
			.def("zoom_all", &Viewport::zoomToSceneExtents,
					"Repositions the viewport camera such that all objects in the scene become completely visible. "
					"The camera direction is not changed.")
			.def("zoomToSelectionExtents", &Viewport::zoomToSelectionExtents)
			.def("zoomToBox", &Viewport::zoomToBox)
			.add_property("_overlays", make_function(&Viewport::overlays, return_internal_reference<>()))
			.def("insertOverlay", &Viewport::insertOverlay)
			.def("removeOverlay", &Viewport::removeOverlay)
		;

		enum_<Viewport::ViewType>("Type")
			.value("NONE", Viewport::VIEW_NONE)
			.value("TOP", Viewport::VIEW_TOP)
			.value("BOTTOM", Viewport::VIEW_BOTTOM)
			.value("FRONT", Viewport::VIEW_FRONT)
			.value("BACK", Viewport::VIEW_BACK)
			.value("LEFT", Viewport::VIEW_LEFT)
			.value("RIGHT", Viewport::VIEW_RIGHT)
			.value("ORTHO", Viewport::VIEW_ORTHO)
			.value("PERSPECTIVE", Viewport::VIEW_PERSPECTIVE)
			.value("SCENENODE", Viewport::VIEW_SCENENODE)
		;
	}

	class_<ViewProjectionParameters>("ViewProjectionParameters")
		.def_readwrite("aspectRatio", &ViewProjectionParameters::aspectRatio)
		.def_readwrite("isPerspective", &ViewProjectionParameters::isPerspective)
		.def_readwrite("znear", &ViewProjectionParameters::znear)
		.def_readwrite("zfar", &ViewProjectionParameters::zfar)
		.def_readwrite("fieldOfView", &ViewProjectionParameters::fieldOfView)
		.def_readwrite("viewMatrix", &ViewProjectionParameters::viewMatrix)
		.def_readwrite("inverseViewMatrix", &ViewProjectionParameters::inverseViewMatrix)
		.def_readwrite("projectionMatrix", &ViewProjectionParameters::projectionMatrix)
		.def_readwrite("inverseProjectionMatrix", &ViewProjectionParameters::inverseProjectionMatrix)
	;

	ovito_class<ViewportConfiguration, RefTarget>(
			"Manages the viewports in OVITO's main window."
			"\n\n"
			"This list-like object can be accessed through the :py:attr:`~ovito.DataSet.viewports` attribute of the :py:attr:`~ovito.DataSet` class. "
			"It contains all viewports in OVITO's main window::"
			"\n\n"
			"    for viewport in dataset.viewports:\n"
			"        print(viewport.title)\n"
			"\n"
			"By default OVITO creates four predefined :py:class:`Viewport` instances. Note that in the current program version it is not possible to add or remove "
			"viewports from the main window. "
			"The ``ViewportConfiguration`` object also manages the :py:attr:`active <.active_vp>` and the :py:attr:`maximized <.maximized_vp>` viewport.")
		.add_property("active_vp", make_function(&ViewportConfiguration::activeViewport, return_value_policy<ovito_object_reference>()), &ViewportConfiguration::setActiveViewport,
				"The viewport that is currently active. It is marked with a colored border in OVITO's main window.")
		.add_property("maximized_vp", make_function(&ViewportConfiguration::maximizedViewport, return_value_policy<ovito_object_reference>()), &ViewportConfiguration::setMaximizedViewport,
				"The viewport that is currently maximized; or ``None`` if no viewport is maximized.\n"
				"Assign a viewport to this attribute to maximize it, e.g.::"
				"\n\n"
				"    dataset.viewports.maximized_vp = dataset.viewports.active_vp\n")
		.def("zoomToSelectionExtents", &ViewportConfiguration::zoomToSelectionExtents)
		.def("zoomToSceneExtents", &ViewportConfiguration::zoomToSceneExtents)
		.def("updateViewports", &ViewportConfiguration::updateViewports)
		.add_property("viewports", make_function(&ViewportConfiguration::viewports, return_internal_reference<>()))
	;

	ovito_abstract_class<ViewportOverlay, RefTarget>();

	ovito_class<CoordinateTripodOverlay, ViewportOverlay>(
			"Displays a coordinate tripod in the rendered image of a viewport. "
			"You can attach an instance of this class to a viewport by adding it to the viewport's "
			":py:attr:`~ovito.vis.Viewport.overlays` collection:"
			"\n\n"
			".. literalinclude:: ../example_snippets/coordinate_tripod_overlay.py"
			"\n\n"
			".. note::\n\n"
			"  Some properties of this class interface have not been exposed and are not accessible from Python yet. "
			"  Please let the developer know if you would like them to be added.\n")
		.add_property("alignment", &CoordinateTripodOverlay::alignment, &CoordinateTripodOverlay::setAlignment,
				"Selects the corner of the viewport where the tripod is displayed. This must be a valid ``Qt::Alignment`` value.")
		.add_property("size", &CoordinateTripodOverlay::tripodSize, &CoordinateTripodOverlay::setTripodSize,
				"The scaling factor that controls the size of the tripod. The size is specified as a fraction of the output image height."
				"\n\n"
				":Default: 0.075\n")
		.add_property("line_width", &CoordinateTripodOverlay::lineWidth, &CoordinateTripodOverlay::setLineWidth,
				"Controls the width of axis arrows. The line width is specified relative to the tripod size."
				"\n\n"
				":Default: 0.06\n")
		.add_property("offset_x", &CoordinateTripodOverlay::offsetX, &CoordinateTripodOverlay::setOffsetX,
				"This parameter allows to displace the tripod horizontally. The offset is specified as a fraction of the output image width."
				"\n\n"
				":Default: 0.0\n")
		.add_property("offset_y", &CoordinateTripodOverlay::offsetY, &CoordinateTripodOverlay::setOffsetY,
				"This parameter allows to displace the tripod vertically. The offset is specified as a fraction of the output image height."
				"\n\n"
				":Default: 0.0\n")
		.add_property("font_size", &CoordinateTripodOverlay::fontSize, &CoordinateTripodOverlay::setFontSize,
				"The font size for rendering the text labels of the tripod. The font size is specified in terms of the tripod size."
				"\n\n"
				":Default: 0.4\n")
	;

	ovito_class<PythonViewportOverlay, ViewportOverlay>(
			"This overlay type can be attached to a viewport to run a Python script every time an "
			"image of the viewport is rendered. The Python script can execute arbitrary drawing commands to "
			"paint on top of the rendered image."
			"\n\n"
			"Note that an alternative to using the :py:class:`!PythonViewportOverlay` class is to directly manipulate the "
			"static image returned by the :py:meth:`Viewport.render` method before saving it to disk. "
			"\n\n"
			"You can attach a Python overlay to a viewport by adding an instance of this class to the viewport's "
			":py:attr:`~ovito.vis.Viewport.overlays` collection:"
			"\n\n"
			".. literalinclude:: ../example_snippets/python_viewport_overlay.py")
		.add_property("script", make_function(&PythonViewportOverlay::script, return_value_policy<copy_const_reference>()), &PythonViewportOverlay::setScript,
				"The source code of the user-defined Python script that defines the ``render()`` function. "
				"Note that this property returns the source code entered by the user through the graphical user interface, not the callable Python function. "
				"\n\n"
				"If you want to set the render function from an already running Python script, you should set "
				"the :py:attr:`.function` property instead as demonstrated in the example above.")
		.add_property("function", &PythonViewportOverlay::scriptFunction, &PythonViewportOverlay::setScriptFunction,
				"The Python function to be called every time the viewport is repainted or when an output image is being rendered."
				"\n\n"
				"The function must have a signature as shown in the example above. The *painter* parameter "
				"passed to the user-defined function contains a `QPainter <http://pyqt.sourceforge.net/Docs/PyQt5/api/qpainter.html>`_ object, which provides "
				"painting methods to draw arbitrary 2D graphics on top of the image rendered by OVITO. "
				"\n\n"
				"Additional keyword arguments are passed to the function in the *args* dictionary. "
				"The following keys are defined: \n\n"
				"   * ``viewport``: The :py:class:`~ovito.vis.Viewport` being rendered.\n"
				"   * ``render_settings``: The active :py:class:`~ovito.vis.RenderSettings`.\n"
				"   * ``is_perspective``: Flag indicating whether projection is perspective or parallel.\n"
				"   * ``fov``: The field of view.\n"
				"   * ``view_tm``: The camera transformation matrix.\n"
				"   * ``proj_tm``: The projection matrix.\n"
				"\n\n"
				"Implementation note: Exceptions raised by the custom rendering function are not propagated to the calling context. "
				"\n\n"
				":Default: ``None``\n")
		.add_property("output", make_function(&PythonViewportOverlay::scriptOutput, return_value_policy<copy_const_reference>()),
				"The output text generated when compiling/running the Python function. "
				"Contain the error message when the most recent execution of the custom rendering function failed.")
	;
}

OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(PyScriptViewport);

};
