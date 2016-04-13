from ovito.io import import_file
from ovito.modifiers import SliceModifier

# Import a simulation file.
node = import_file('simulation.dump')

# Print original number of particles.
input = node.source
print("Input particle count: %i" % input.number_of_particles)

# Set up modification pipeline.
node.modifiers.append(SliceModifier(normal = (0,0,1), distance = 0))

# Compute effect of slice modifier.
output = node.compute()
print("Output particle count: %i" % output.number_of_particles)
