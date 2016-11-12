import ovito
from ovito.io import (import_file, export_file)
import os
import os.path

test_data_dir = "../../files/"

node1 = import_file(test_data_dir + "LAMMPS/class2.data", atom_style = "full")
print(node1.source)
export_file(node1, "test.data", "lammps_data", atom_style = "full")
export_file(node1, "test.data", "lammps_data", atom_style = "bond")
export_file(node1, "test.data", "lammps_dump", columns = ["Particle Identifier", "Particle Type", "Position.X", "Position.Y", "Position.Z"])
export_file(node1, "test.data", "fhi-aims")
export_file(node1, "test.data", "imd")
export_file(node1, "test.data", "vasp")
export_file(node1, "test.data", "xyz", columns = ["Position.X", "Position.Y", "Position.Z"])
ovito.dataset.anim.last_frame = 7
export_file(node1, "test.dump", "lammps_dump", columns = ["Position.X", "Position.Y", "Position.Z"], multiple_frames = True)
export_file(node1, "test.*.dump", "lammps_dump", columns = ["Position.X", "Position.Y", "Position.Z"], multiple_frames = True, start_frame = 1, end_frame = 5, every_nth_frame = 2)
os.remove("test.data")
os.remove("test.dump")
os.remove("test.1.dump")
os.remove("test.3.dump")
os.remove("test.5.dump")
for i in range(ovito.dataset.anim.last_frame + 1):
    export_file(node1, "test.%i.dump" % i, "lammps_dump", columns = ["Position.X", "Position.Y", "Position.Z"], frame = i)
    os.remove("test.%i.dump" % i)