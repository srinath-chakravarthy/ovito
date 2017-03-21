from ovito.io import *
from ovito.modifiers import *
import numpy

node = import_file("../../files/LAMMPS/bonds.data.gz", atom_style = 'bond')
node.modifiers.append(WrapPeriodicImagesModifier())
print(node.compute().bonds.pbc_vectors)
