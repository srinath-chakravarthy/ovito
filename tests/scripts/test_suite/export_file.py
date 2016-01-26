import ovito
from ovito.io import (import_file, export_file)
import os
import os.path

test_data_dir = "../../files/"

node1 = import_file(test_data_dir + "LAMMPS/class2.data", atom_style = "full")
print(node1.source)
export_file(node1, "test.data", "lammps_data", atom_style = "full")
export_file(node1, "test.data", "lammps_dump", columns = ["Particle Identifier", "Particle Type", "Position.X", "Position.Y", "Position.Z"])
export_file(node1, "test.data", "fhi-aims")
export_file(node1, "test.data", "imd")
export_file(node1, "test.data", "vasp")
export_file(node1, "test.data", "xyz", columns = ["Position.X", "Position.Y", "Position.Z"])
os.remove("test.data")