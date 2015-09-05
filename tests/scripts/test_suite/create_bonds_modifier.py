from ovito import *
from ovito.io import *
from ovito.data import *
from ovito.modifiers import *
import numpy as np

node = import_file("../../files/CFG/shear.void.120.cfg")

modifier = CreateBondsModifier()
node.modifiers.append(modifier)

print("Parameter defaults:")

print("  cutoff: {}".format(modifier.cutoff))
modifier.cutoff = 3.1

print("  intra_molecule_only: {}".format(modifier.intra_molecule_only))
modifier.intra_molecule_only = True

node.compute()

print("Output:")
print(node.output.bonds)
print(node.output.bonds.array)
print(len(node.output.bonds.array))
assert(node.output.number_of_bonds == len(node.output.bonds.array))

assert(node.output.number_of_bonds == 21894)

bond_enumerator = Bonds.Enumerator(node.output.bonds)
for bond_index in bond_enumerator.bonds_of_particle(0):
    print("Bond index 0:", bond_index)
for bond_index in bond_enumerator.bonds_of_particle(1):
    print("Bond index 1:", bond_index)