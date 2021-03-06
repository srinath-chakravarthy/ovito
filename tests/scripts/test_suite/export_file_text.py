import ovito
from ovito.io import import_file, export_file
import os
import os.path

test_data_dir = "../../files/"

node = import_file(test_data_dir + "LAMMPS/animation1.dump", multiple_frames = True)
export_file(node, "test.data", "txt", columns = ["Timestep", "SourceFrame"], multiple_frames = True)
os.remove("test.data")