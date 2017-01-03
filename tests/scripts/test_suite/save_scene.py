import ovito
from ovito.io import *
import os
import os.path

node = import_file("../../files/LAMMPS/animation.dump.gz")
node.add_to_scene()

output_test_file = "_save_scene.ovito"

if os.path.isfile(output_test_file): os.remove(output_test_file)

ovito.dataset.save(output_test_file)
assert(os.path.isfile(output_test_file))

os.remove(output_test_file)
