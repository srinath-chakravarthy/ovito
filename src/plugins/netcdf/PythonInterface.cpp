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
#include <plugins/pyscript/binding/PythonBinding.h>
#include <plugins/netcdf/NetCDFImporter.h>
#include <plugins/particles/scripting/PythonBinding.h>
#include <core/plugins/PluginManager.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import) OVITO_BEGIN_INLINE_NAMESPACE(Formats) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

using namespace PyScript;

PYBIND11_PLUGIN(NetCDFPlugin)
{
	// Register the classes of this plugin with the global PluginManager.
	PluginManager::instance().registerLoadedPluginClasses();
	
	py::options options;
	options.disable_function_signatures();

	py::module m("NetCDFPlugin");

	ovito_class<NetCDFImporter, ParticleImporter>(m)
		.def_property("custom_column_mapping", &NetCDFImporter::customColumnMapping, &NetCDFImporter::setCustomColumnMapping)
		.def_property("use_custom_column_mapping", &NetCDFImporter::useCustomColumnMapping, &NetCDFImporter::setUseCustomColumnMapping)
	;

	return m.ptr();
}

OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(NetCDFPlugin);

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
