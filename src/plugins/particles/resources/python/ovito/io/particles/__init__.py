# Load dependencies
import ovito.io

# Load the native code modules
import Particles
from ParticlesImporter import *
from ParticlesExporter import *

# Register export formats.
ovito.io.export_file._formatTable["lammps_dump"] = LAMMPSDumpExporter
ovito.io.export_file._formatTable["lammps_data"] = LAMMPSDataExporter
ovito.io.export_file._formatTable["imd"] = IMDExporter
ovito.io.export_file._formatTable["vasp"] = POSCARExporter
ovito.io.export_file._formatTable["xyz"] = XYZExporter
ovito.io.export_file._formatTable["fhi-aims"] = FHIAimsExporter

# Implement the 'atom_style' property of LAMMPSDataImporter.
def _get_LAMMPSDataImporter_atom_style(self):
    return str(self.atomStyle)
def _set_LAMMPSDataImporter_atom_style(self, style):
    self.atomStyle = LAMMPSDataImporter.LAMMPSAtomStyle.__dict__[str(style)]
LAMMPSDataImporter.atom_style = property(_get_LAMMPSDataImporter_atom_style, _set_LAMMPSDataImporter_atom_style)

# Implement the 'atom_style' property of LAMMPSDataExporter.
LAMMPSDataExporter.atom_style = property(_get_LAMMPSDataImporter_atom_style, _set_LAMMPSDataImporter_atom_style)