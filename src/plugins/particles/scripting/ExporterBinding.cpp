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

#include <plugins/particles/Particles.h>
#include <plugins/pyscript/binding/PythonBinding.h>
#include <plugins/particles/data/ParticleProperty.h>
#include <plugins/particles/objects/ParticlePropertyObject.h>
#include <plugins/particles/objects/ParticleTypeProperty.h>
#include <plugins/particles/objects/SimulationCellObject.h>
#include <plugins/particles/export/ParticleExporter.h>
#include <plugins/particles/export/imd/IMDExporter.h>
#include <plugins/particles/export/vasp/POSCARExporter.h>
#include <plugins/particles/export/xyz/XYZExporter.h>
#include <plugins/particles/export/lammps/LAMMPSDumpExporter.h>
#include <plugins/particles/export/lammps/LAMMPSDataExporter.h>
#include <plugins/particles/export/fhi_aims/FHIAimsExporter.h>

#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

using namespace boost::python;
using namespace PyScript;

BOOST_PYTHON_MODULE(ParticlesExporter)
{
	docstring_options docoptions(true, false);

	class_<OutputColumnMapping>("OutputColumnMapping", init<>())
		.def(vector_indexing_suite<OutputColumnMapping>())
	;
	python_to_container_conversion<OutputColumnMapping>();

	ovito_abstract_class<ParticleExporter, FileExporter>()
	;

	ovito_abstract_class<FileColumnParticleExporter, ParticleExporter>()
		.add_property("columns", make_function(&FileColumnParticleExporter::columnMapping, return_value_policy<copy_const_reference>()), &FileColumnParticleExporter::setColumnMapping)
	;

	ovito_class<IMDExporter, FileColumnParticleExporter>()
	;

	ovito_class<POSCARExporter, ParticleExporter>()
	;

	ovito_class<LAMMPSDataExporter, ParticleExporter>()
		.add_property("atomStyle", &LAMMPSDataExporter::atomStyle, &LAMMPSDataExporter::setAtomStyle)
	;

	ovito_class<LAMMPSDumpExporter, FileColumnParticleExporter>()
	;

	ovito_class<XYZExporter, FileColumnParticleExporter>()
		.add_property("subFormat", &XYZExporter::subFormat, &XYZExporter::setSubFormat)
	;

	enum_<XYZExporter::XYZSubFormat>("XYZSubFormat")
		.value("Parcas", XYZExporter::ParcasFormat)
		.value("Extended", XYZExporter::ExtendedFormat)
	;

	ovito_class<FHIAimsExporter, ParticleExporter>()
	;
}

OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(ParticlesExporter);

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
