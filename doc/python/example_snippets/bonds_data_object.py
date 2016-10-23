from ovito.io import import_file
from ovito.modifiers import CreateBondsModifier
from ovito.vis import BondsDisplay

# Load a set of atoms and create bonds between pairs of atoms
# that are within a given cutoff distance of each other
# using the Create Bonds modifier.
node = import_file('simulation.dump')
node.modifiers.append(CreateBondsModifier(cutoff = 3.4))
node.compute()

# Read out list of generated bonds.
bonds_list = node.output.bonds.array
print("Number of generated bonds: ", len(bonds_list)//2)

# Change appearance of bonds.
node.output.bonds.display.enabled = True
node.output.bonds.display.shading = BondsDisplay.Shading.Flat
node.output.bonds.display.width = 0.3

# Compute bond vectors.
particle_positions = node.output.particle_properties.position.array
bonds_array = node.output.bonds.array
bond_vectors = particle_positions[bonds_array[:,1]] - particle_positions[bonds_array[:,0]]
bond_vectors += numpy.dot(node.output.cell.matrix[:,:3], node.output.bonds.pbc_vectors.T).T
