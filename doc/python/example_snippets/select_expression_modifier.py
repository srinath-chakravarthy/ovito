from ovito.io import import_file
from ovito.modifiers import SelectExpressionModifier
import numpy
node = import_file("../../../examples/data/NanocrystallinePd.dump.gz")

# Select all atoms with potential energy above -3.6 eV: 
node.modifiers.append(SelectExpressionModifier(expression = 'PotentialEnergy > -3.6'))
node.compute()
# Demonstrating two ways to get the number of selected atoms:
print(node.output.attributes['SelectExpression.num_selected'])
print(numpy.count_nonzero(node.output.particle_properties['Selection']))