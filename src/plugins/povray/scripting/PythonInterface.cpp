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

#include <plugins/pyscript/PyScript.h>
#include <plugins/pyscript/binding/PythonBinding.h>
#include <plugins/povray/renderer/POVRayRenderer.h>
#include <plugins/povray/exporter/POVRayExporter.h>
#include <core/plugins/PluginManager.h>

namespace Ovito { namespace POVRay { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

using namespace Ovito;
using namespace PyScript;

PYBIND11_PLUGIN(POVRay)
{
	// Register the classes of this plugin with the global PluginManager.
	PluginManager::instance().registerLoadedPluginClasses();

	py::options options;
	options.disable_function_signatures();

	py::module m("POVRay");

	ovito_class<POVRayRenderer, NonInteractiveSceneRenderer>(m,
			"This is one of the rendering backends of OVITO. "
			"\n\n"
			"POV-Ray (The Persistence of Vision Raytracer) is an open-source raytracing program. "
			"The POV-Ray rendering backend streams the scene data to a temporary file, which is then processed and rendered by the "
			"external POV-Ray program. The final rendered image is read back into OVITO. "
			"\n\n"
			"The rendering backend requires the POV-Ray executable to be installed on the system. It will automatically "
			"look for the executable ``povray`` in the system path. If the executable is not in the default search path, "
			"its location must be explicitly specified by setting the :py:attr:`.povray_executable` attribute. "
			"\n\n"
			"For a more detailed description of the rendering parameters exposed by this Python class, please consult the "
			"`official POV-Ray documentation <http://www.povray.org/documentation/>`_.")
		.def_property("povray_executable", &POVRayRenderer::povrayExecutable, &POVRayRenderer::setPovrayExecutable,
				"The absolute path to the external POV-Ray executable on the local computer, which is called by this rendering backend to render an image. "
				"If no path is set, OVITO will look for ``povray`` in the default executable search path. "
				"\n\n"
				":Default: ``\"\"``")
		.def_property("quality_level", &POVRayRenderer::qualityLevel, &POVRayRenderer::setQualityLevel,
				"The `image rendering quality <http://www.povray.org/documentation/3.7.0/r3_2.html#r3_2_8_3>`_ parameter passed to POV-Ray."
				"\n\n"
				":Default: 9")
		.def_property("antialiasing", &POVRayRenderer::antialiasingEnabled, &POVRayRenderer::setAntialiasingEnabled,
				"Enables supersampling to reduce aliasing effects."
				"\n\n"
				":Default: ``True``")
		.def_property("show_window", &POVRayRenderer::povrayDisplayEnabled, &POVRayRenderer::setPovrayDisplayEnabled,
				"Controls whether the POV-Ray window is shown during rendering. This allows you to follow the image generation process. "
				"\n\n"
				":Default: ``True``")
		.def_property("radiosity", &POVRayRenderer::radiosityEnabled, &POVRayRenderer::setRadiosityEnabled,
				"Enables `radiosity light calculations <http://www.povray.org/documentation/3.7.0/r3_4.html#r3_4_4_3>`_."
				"\n\n"
				":Default: ``False``")
		.def_property("radiosity_raycount", &POVRayRenderer::radiosityRayCount, &POVRayRenderer::setRadiosityRayCount,
				"The number of rays that are sent out whenever a new radiosity value has to be calculated."
				"\n\n"
				":Default: 50")
		.def_property("depth_of_field", &POVRayRenderer::depthOfFieldEnabled, &POVRayRenderer::setDepthOfFieldEnabled,
				"This flag enables `focus blur <http://www.povray.org/documentation/3.7.0/r3_4.html#r3_4_2_3>`_ (depth-of-field) rendering."
				"\n\n"
				":Default: ``False``")
		.def_property("focal_length", &POVRayRenderer::dofFocalLength, &POVRayRenderer::setDofFocalLength,
				"Controls the focal length of the camera, which is used for depth-of-field rendering."
				"\n\n"
				":Default: 40.0")
		.def_property("aperture", &POVRayRenderer::dofAperture, &POVRayRenderer::setDofAperture,
				"Controls the aperture of the camera, which is used for depth-of-field rendering."
				"\n\n"
				":Default: 1.0")
		.def_property("blur_samples", &POVRayRenderer::dofSampleCount, &POVRayRenderer::setDofSampleCount,
				"Controls the maximum number of rays to use for each pixel to compute focus blur (depth-of-field)."
				"\n\n"
				":Default: 1.0")
		.def_property("omni_stereo", &POVRayRenderer::odsEnabled, &POVRayRenderer::setODSEnabled,
				"This flag enables `omni­directional stereo projection <http://wiki.povray.org/content/HowTo:ODS>`_ for stereoscopic 360-degree VR videos and images. "
				"Note that this requires POV-Ray 3.7.1 or newer. The eye separation distance is controlled by the :py:attr:`.interpupillary_distance` parameter. "
				"\n\n"
				":Default: ``False``")
		.def_property("interpupillary_distance", &POVRayRenderer::interpupillaryDistance, &POVRayRenderer::setInterpupillaryDistance,
				"Controls interpupillary distance (eye separation) for stereoscopic rendering. This setting is only used "
				"if the :py:attr:`.omni_stereo` option has been set. "
				"\n\n"
				":Default: 0.5")		
	;

	ovito_class<POVRayExporter, FileExporter>{m}
	;

	return m.ptr();
}

OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(POVRay);

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
