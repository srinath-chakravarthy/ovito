import ovito
from ovito.io import (import_file, export_file)
import os
import os.path

test_data_dir = "../../files/"

node1 = import_file(test_data_dir + "LAMMPS/class2.data", atom_style = "full")
print(node1.source)
export_file(node1, "test.data", "lammps_data", atom_style = "full")
os.remove("test.data")