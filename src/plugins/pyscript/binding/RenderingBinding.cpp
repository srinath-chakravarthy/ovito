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
#include <core/rendering/RenderSettings.h>
#include <core/rendering/SceneRenderer.h>
#include <core/rendering/noninteractive/NonInteractiveSceneRenderer.h>
#include <core/rendering/ParticlePrimitive.h>
#include <core/rendering/ArrowPrimitive.h>
#include <core/rendering/FrameBuffer.h>
#include <core/scene/objects/DisplayObject.h>
#include <core/scene/objects/geometry/TriMeshDisplay.h>
#include <opengl_renderer/StandardSceneRenderer.h>
#include "PythonBinding.h"

namespace PyScript {

using namespace Ovito;

void defineRenderingSubmodule(py::module parentModule)
{
	py::module m = parentModule.def_submodule("Rendering");

	py::class_<FrameBuffer, std::shared_ptr<FrameBuffer>>(m, "FrameBuffer")
		.def(py::init<>())
		.def(py::init<int, int>())
		.def_property_readonly("width", &FrameBuffer::width)
		.def_property_readonly("height", &FrameBuffer::height)
		.def_property_readonly("_image", [](const FrameBuffer& fb) { return reinterpret_cast<std::uintptr_t>(&fb.image()); })
	;

	py::object RenderSettings_py = ovito_class<RenderSettings, RefTarget>(m,
			"Stores settings and parameters for rendering images and movies."
			"\n\n"
			"A instance of this class can be passed to the :py:func:`~Viewport.render` function "
			"of the :py:class:`Viewport` class to control various aspects such as the resolution of the generated image. "
			"The ``RenderSettings`` object contains a :py:attr:`.renderer`, which is the rendering engine "
			"that will be used to generate images of the three-dimensional scene. OVITO comes with two different "
			"rendering engines:"
			"\n\n"
			"  * :py:class:`OpenGLRenderer` -- An OpenGL-based renderer, which is also used for the interactive display in OVITO's viewports.\n"
			"  * :py:class:`TachyonRenderer` -- A software-based, high-quality raytracing renderer.\n"
			"  * :py:class:`POVRayRenderer` -- A rendering backend that invokes the external POV-Ray raytracing program.\n"
			"\n"
			"Usage example::"
			"\n\n"
			"    rs = RenderSettings(\n"
			"        filename = 'image.png',\n"
			"        size = (1024,768),\n"
			"        background_color = (0.8,0.8,1.0)\n"
			"    )\n"
			"    rs.renderer.antialiasing = False\n"
			"    dataset.viewports.active_vp.render(rs)\n")
		.def_property("renderer", &RenderSettings::renderer, &RenderSettings::setRenderer,
				"The renderer that is used to generate the image or movie. Depending on the selected renderer you "
				"can use this to set additional parameters such as the anti-aliasing level."
				"\n\n"
				"See the :py:class:`OpenGLRenderer`, :py:class:`TachyonRenderer` and :py:class:`POVRayRenderer` classes "
				"for the list of parameters specific to each rendering backend.")
		.def_property("range", &RenderSettings::renderingRangeType, &RenderSettings::setRenderingRangeType,
				"Selects the animation frames to be rendered."
				"\n\n"
				"Possible values:\n"
				"  * ``RenderSettings.Range.CURRENT_FRAME`` (default): Renders a single image at the current animation time.\n"
				"  * ``RenderSettings.Range.ANIMATION``: Renders a movie of the entire animation sequence.\n"
				"  * ``RenderSettings.Range.CUSTOM_INTERVAL``: Renders a movie of the animation interval given by the :py:attr:`.custom_range` attribute.\n")
		.def_property("outputImageWidth", &RenderSettings::outputImageWidth, &RenderSettings::setOutputImageWidth)
		.def_property("outputImageHeight", &RenderSettings::outputImageHeight, &RenderSettings::setOutputImageHeight)
		.def_property_readonly("outputImageAspectRatio", &RenderSettings::outputImageAspectRatio)
		.def_property("imageFilename", &RenderSettings::imageFilename, &RenderSettings::setImageFilename)
		.def_property("background_color", &RenderSettings::backgroundColor, &RenderSettings::setBackgroundColor,
				"Controls the background color of the rendered image."
				"\n\n"
				":Default: ``(1,1,1)`` -- white")
		.def_property("generate_alpha", &RenderSettings::generateAlphaChannel, &RenderSettings::setGenerateAlphaChannel,
				"When saving the generated image to a file format that can store transparency information (e.g. PNG), this option will make "
				"those parts of the output image transparent that are not covered by an object."
				"\n\n"
				":Default: ``False``")
		.def_property("saveToFile", &RenderSettings::saveToFile, &RenderSettings::setSaveToFile)
		.def_property("skip_existing_images", &RenderSettings::skipExistingImages, &RenderSettings::setSkipExistingImages,
				"Controls whether animation frames for which the output image file already exists will be skipped "
				"when rendering an animation sequence. This flag is ignored when directly rendering to a movie file and not an image file sequence. "
				"Use this flag when the image sequence has already been partially rendered and you want to render just the missing frames. "
				"\n\n"
				":Default: ``False``")
		.def_property("customRangeStart", &RenderSettings::customRangeStart, &RenderSettings::setCustomRangeStart)
		.def_property("customRangeEnd", &RenderSettings::customRangeEnd, &RenderSettings::setCustomRangeEnd)
		.def_property("everyNthFrame", &RenderSettings::everyNthFrame, &RenderSettings::setEveryNthFrame)
		.def_property("fileNumberBase", &RenderSettings::fileNumberBase, &RenderSettings::setFileNumberBase)
	;

	py::enum_<RenderSettings::RenderingRangeType>(RenderSettings_py, "Range")
		.value("CURRENT_FRAME", RenderSettings::CURRENT_FRAME)
		.value("ANIMATION", RenderSettings::ANIMATION_INTERVAL)
		.value("CUSTOM_INTERVAL", RenderSettings::CUSTOM_INTERVAL)
	;

	ovito_abstract_class<SceneRenderer, RefTarget>(m)
		.def_property_readonly("isInteractive", &SceneRenderer::isInteractive)
	;

	ovito_abstract_class<NonInteractiveSceneRenderer, SceneRenderer>{m}
	;

	ovito_class<StandardSceneRenderer, SceneRenderer>(m,
			"The standard OpenGL-based renderer."
			"\n\n"
			"This is the default built-in rendering engine that is also used by OVITO to render the contents of the interactive viewports. "
			"Since it accelerates the generation of images by using the computer's graphics hardware, it is very fast.",
			"OpenGLRenderer")
		.def_property("antialiasing_level", &StandardSceneRenderer::antialiasingLevel, &StandardSceneRenderer::setAntialiasingLevel,
				"A positive integer controlling the level of supersampling. If 1, no supersampling is performed. For larger values, "
				"the image in rendered at a higher resolution and then scaled back to the output size to reduce aliasing artifacts."
				"\n\n"
				":Default: 3")
	;

	ovito_abstract_class<DisplayObject, RefTarget>(m,
			"Abstract base class for display setting objects that control the visual appearance of data. "
			":py:class:`DataObjects <ovito.data.DataObject>` may be associated with an instance of this class, which can be accessed via "
			"their :py:attr:`~ovito.data.DataObject.display` property.",
			// Python class name:
			"Display")
		.def_property("enabled", &DisplayObject::isEnabled, &DisplayObject::setEnabled,
				"Boolean flag controlling the visibility of the data. If set to ``False``, the "
				"data will not be visible in the viewports or in rendered images."
				"\n\n"
				":Default: ``True``\n")
	;

	ovito_class<TriMeshDisplay, DisplayObject>(m)
		.def_property("color", &TriMeshDisplay::color, &TriMeshDisplay::setColor)
		.def_property("transparency", &TriMeshDisplay::transparency, &TriMeshDisplay::setTransparency)
	;

	py::enum_<ParticlePrimitive::ShadingMode>(m, "ParticleShadingMode")
		.value("Normal", ParticlePrimitive::NormalShading)
		.value("Flat", ParticlePrimitive::FlatShading)
	;

	py::enum_<ParticlePrimitive::RenderingQuality>(m, "ParticleRenderingQuality")
		.value("LowQuality", ParticlePrimitive::LowQuality)
		.value("MediumQuality", ParticlePrimitive::MediumQuality)
		.value("HighQuality", ParticlePrimitive::HighQuality)
		.value("AutoQuality", ParticlePrimitive::AutoQuality)
	;

	py::enum_<ParticlePrimitive::ParticleShape>(m, "ParticleShape")
		.value("Spherical", ParticlePrimitive::SphericalShape)	// Deprecated since v2.4.5
		.value("Round", ParticlePrimitive::SphericalShape)
		.value("Square", ParticlePrimitive::SquareShape)
	;

	py::enum_<ArrowPrimitive::ShadingMode>(m, "ArrowShadingMode")
		.value("Normal", ArrowPrimitive::NormalShading)
		.value("Flat", ArrowPrimitive::FlatShading)
	;

	py::enum_<ArrowPrimitive::RenderingQuality>(m, "ArrowRenderingQuality")
		.value("LowQuality", ArrowPrimitive::LowQuality)
		.value("MediumQuality", ArrowPrimitive::MediumQuality)
		.value("HighQuality", ArrowPrimitive::HighQuality)
	;

	py::enum_<ArrowPrimitive::Shape>(m, "ArrowShape")
		.value("CylinderShape", ArrowPrimitive::CylinderShape)
		.value("ArrowShape", ArrowPrimitive::ArrowShape)
	;
}

};
