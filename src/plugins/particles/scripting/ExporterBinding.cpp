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
#include "PythonBinding.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

using namespace PyScript;

void defineExportersSubmodule(py::module parentModule)
{
	py::module m = parentModule.def_submodule("Exporters");

	ovito_abstract_class<ParticleExporter, FileExporter>{m}
	;

	ovito_abstract_class<FileColumnParticleExporter, ParticleExporter>(m)
		.def_property("columns", &FileColumnParticleExporter::columnMapping, &FileColumnParticleExporter::setColumnMapping)
	;

	ovito_class<IMDExporter, FileColumnParticleExporter>{m}
	;

	ovito_class<POSCARExporter, ParticleExporter>{m}
	;

	ovito_class<LAMMPSDataExporter, ParticleExporter>{m}
		.def_property("_atom_style", &LAMMPSDataExporter::atomStyle, &LAMMPSDataExporter::setAtomStyle)
	;

	ovito_class<LAMMPSDumpExporter, FileColumnParticleExporter>{m}
	;

	auto XYZExporter_py = ovito_class<XYZExporter, FileColumnParticleExporter>{m}
		.def_property("sub_format", &XYZExporter::subFormat, &XYZExporter::setSubFormat)
	;

	py::enum_<XYZExporter::XYZSubFormat>(XYZExporter_py, "XYZSubFormat")
		.value("Parcas", XYZExporter::ParcasFormat)
		.value("Extended", XYZExporter::ExtendedFormat)
	;

	ovito_class<FHIAimsExporter, ParticleExporter>{m}
	;
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
