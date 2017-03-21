from ovito.io import *
from ovito.modifiers import *
import numpy as np

node = import_file("../../files/CFG/shear.void.120.cfg")

node.modifiers.append(CreateBondsModifier(cutoff = 3.1))
node.modifiers.append(SliceModifier(select=True))

modifier = ExpandSelectionModifier()
node.modifiers.append(modifier)

print("Parameter defaults:")

print("  mode: {}".format(modifier.mode))
modifier.mode = ExpandSelectionModifier.ExpansionMode.Bonded

print("  cutoff: {}".format(modifier.cutoff))
modifier.cutoff = 3.0

print("  num_neighbors: {}".format(modifier.num_neighbors))
modifier.num_neighbors = 3

print("  iterations: {}".format(modifier.iterations))
modifier.iterations = 3

node.compute()

selection_count = np.count_nonzero(node.output.selection.array)
print(selection_count)
assert(selection_count == 1212)

