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

print("  lower_cutoff: {}".format(modifier.lower_cutoff))
modifier.lower_cutoff = 0.1

print("  mode: {}".format(modifier.mode))
modifier.mode = CreateBondsModifier.Mode.Uniform

node.compute()

print("Output:")
print(node.output.bonds)
print(node.output.bonds.array)
print(len(node.output.bonds.array))
assert(node.output.number_of_half_bonds == len(node.output.bonds.array))

assert(node.output.number_of_half_bonds == 21894)
assert(node.output.number_of_half_bonds == node.output.number_of_full_bonds*2)

bond_enumerator = Bonds.Enumerator(node.output.bonds)
for bond_index in bond_enumerator.bonds_of_particle(0):
    print("Bond index 0:", bond_index)
for bond_index in bond_enumerator.bonds_of_particle(1):
    print("Bond index 1:", bond_index)
    
node.source.load("../../files/POSCAR/Ti_n1_PBE.n54_G7_V15.000.poscar.000")
modifier.mode = CreateBondsModifier.Mode.Pairwise
modifier.set_pairwise_cutoff("W", "Ti", 3.0)
assert(modifier.get_pairwise_cutoff("Ti", "W") == 3.0)
node.compute()
assert(node.output.number_of_half_bonds == 16)
