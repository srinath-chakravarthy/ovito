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
#include <plugins/particles/import/InputColumnMapping.h>
#include <plugins/particles/import/ParticleImporter.h>
#include <plugins/particles/import/cfg/CFGImporter.h>
#include <plugins/particles/import/imd/IMDImporter.h>
#include <plugins/particles/import/parcas/ParcasFileImporter.h>
#include <plugins/particles/import/vasp/POSCARImporter.h>
#include <plugins/particles/import/xyz/XYZImporter.h>
#include <plugins/particles/import/pdb/PDBImporter.h>
#include <plugins/particles/import/lammps/LAMMPSTextDumpImporter.h>
#include <plugins/particles/import/lammps/LAMMPSBinaryDumpImporter.h>
#include <plugins/particles/import/lammps/LAMMPSDataImporter.h>
#include <plugins/particles/import/fhi_aims/FHIAimsImporter.h>
#include <plugins/particles/import/fhi_aims/FHIAimsLogFileImporter.h>
#include <plugins/particles/import/gsd/GSDImporter.h>
#include "PythonBinding.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

using namespace PyScript;

void defineImportersSubmodule(py::module parentModule)
{
	py::module m = parentModule.def_submodule("Importers");

	ovito_abstract_class<ParticleImporter, FileSourceImporter>(m)
		.def_property("multiple_frames", &ParticleImporter::isMultiTimestepFile, &ParticleImporter::setMultiTimestepFile)
	;

	ovito_class<XYZImporter, ParticleImporter>(m)
		.def_property("columns", &XYZImporter::columnMapping, &XYZImporter::setColumnMapping)
		.def_property("rescale_reduced_coords", &XYZImporter::autoRescaleCoordinates, &XYZImporter::setAutoRescaleCoordinates)
	;

	ovito_class<LAMMPSTextDumpImporter, ParticleImporter>(m)
		.def_property("columns", &LAMMPSTextDumpImporter::customColumnMapping, [](LAMMPSTextDumpImporter& imp, const InputColumnMapping& mapping) {
				imp.setCustomColumnMapping(mapping);
				imp.setUseCustomColumnMapping(true);
		})
	;

	auto LAMMPSDataImporter_py = ovito_class<LAMMPSDataImporter, ParticleImporter>(m)
		.def_property("_atom_style", &LAMMPSDataImporter::atomStyle, &LAMMPSDataImporter::setAtomStyle)
	;
	py::enum_<LAMMPSDataImporter::LAMMPSAtomStyle>(LAMMPSDataImporter_py, "LAMMPSAtomStyle")
		.value("unknown", LAMMPSDataImporter::AtomStyle_Unknown)
		.value("angle", LAMMPSDataImporter::AtomStyle_Angle)
		.value("atomic", LAMMPSDataImporter::AtomStyle_Atomic)
		.value("body", LAMMPSDataImporter::AtomStyle_Body)
		.value("bond", LAMMPSDataImporter::AtomStyle_Bond)
		.value("charge", LAMMPSDataImporter::AtomStyle_Charge)
		.value("full", LAMMPSDataImporter::AtomStyle_Full)
		.value("dipole", LAMMPSDataImporter::AtomStyle_Dipole)
		.value("molecular", LAMMPSDataImporter::AtomStyle_Molecular)
		.value("sphere", LAMMPSDataImporter::AtomStyle_Sphere)
	;

	ovito_class<LAMMPSBinaryDumpImporter, ParticleImporter>(m)
		.def_property("columns", &LAMMPSBinaryDumpImporter::columnMapping, &LAMMPSBinaryDumpImporter::setColumnMapping)
	;

	ovito_class<CFGImporter, ParticleImporter>{m}
	;

	ovito_class<IMDImporter, ParticleImporter>{m}
	;

	ovito_class<ParcasFileImporter, ParticleImporter>{m}
	;

	ovito_class<PDBImporter, ParticleImporter>{m}
	;

	ovito_class<POSCARImporter, ParticleImporter>{m}
	;

	ovito_class<FHIAimsImporter, ParticleImporter>{m}
	;

	ovito_class<FHIAimsLogFileImporter, ParticleImporter>{m}
	;

	ovito_class<GSDImporter, ParticleImporter>{m}
	;
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
