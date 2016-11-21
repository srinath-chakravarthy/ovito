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

namespace Ovito { namespace POVRay { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

using namespace Ovito;
using namespace PyScript;

PYBIND11_PLUGIN(POVRay)
{
	py::options options;
	options.disable_function_signatures();

	py::module m("POVRay");

	ovito_class<POVRayRenderer, NonInteractiveSceneRenderer>{m}
		.def_property("povray_executable", &POVRayRenderer::povrayExecutable, &POVRayRenderer::setPovrayExecutable)
		.def_property("quality_level", &POVRayRenderer::qualityLevel, &POVRayRenderer::setQualityLevel)
		.def_property("antialiasing", &POVRayRenderer::antialiasingEnabled, &POVRayRenderer::setAntialiasingEnabled)
		.def_property("show_window", &POVRayRenderer::povrayDisplayEnabled, &POVRayRenderer::setPovrayDisplayEnabled)
		.def_property("radiosity", &POVRayRenderer::radiosityEnabled, &POVRayRenderer::setRadiosityEnabled)
		.def_property("radiosity_raycount", &POVRayRenderer::radiosityRayCount, &POVRayRenderer::setRadiosityRayCount)
	;

	ovito_class<POVRayExporter, FileExporter>{m}
	;

	return m.ptr();
}

OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(POVRay);

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
