from ovito.io import *
from ovito.modifiers import *

# Load a simulation sequence. 
node = import_file("simulation.*.dump")

# Add modifier for computing the coordination numbers of particles.
node.modifiers.append(CoordinationNumberModifier(cutoff = 2.9))

# Save the initial coordination numbers at frame 0 under a different name.
modifier = FreezePropertyModifier(source_property = 'Coordination', 
                                  destination_property = 'Coord0')
node.modifiers.append(modifier)

# This will evaluate the modification pipeline at the current animation and up to 
# the FreezePropertyModifier. The current values of the 'Coordination' property are
# stored and inserted back into the pipeline under the new name 'Coord0'.
modifier.take_snapshot()

# Select all particles whose coordination number has changed since frame 0
# by comapring the dynamically computed coordination numbers with the stored ones.
node.modifiers.append(SelectExpressionModifier(expression='Coordination != Coord0'))

# Write a table with the number of particles having a coordination number variation.
export_file(node, 'output.txt', 'txt', 
            columns = ['Timestep', 'SelectExpression.num_selected'], 
            multiple_frames = True)