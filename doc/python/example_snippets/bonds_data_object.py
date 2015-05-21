from ovito.io import import_file
from ovito.modifiers import CreateBondsModifier
from ovito.vis import BondsDisplay

# Load atoms and create bonds between pairs of atoms
# that are within a given cutoff distance of each other.
node = import_file('simulation.dump')
node.modifiers.append(CreateBondsModifier(cutoff = 3.4))
node.compute()

# Read out bonds list.
bonds_list = node.output.bonds.array
print("Number of bonds: ", len(bonds_list)//2)

# Change appearance of bonds
node.output.bonds.display.shading = BondsDisplay.Shading.Flat
node.output.bonds.display.width = 0.3