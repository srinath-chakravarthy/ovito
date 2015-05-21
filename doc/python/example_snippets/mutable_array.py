from ovito.io import import_file
from ovito.modifiers import *

# Load a simulation file.
node = import_file('simulation.dump')
# Set up modification pipeline, which selects and deletes all particle of type 2.
node.modifiers.append(SelectParticleTypeModifier(property='Particle Type', types={2}))
node.modifiers.append(DeleteSelectedParticlesModifier())

# Evaluate modification pipeline the first time.
output1 = node.compute()
print('Number of remaining particles: ', output1.position.size)

# Modify pipeline input by changing the type of the first particle to 2.
node.source.particle_type.mutable_array[0] = 2

# Inform pipeline that input has changed.
# Failing to do so would lead to incorrect results below. OVITO would assume the 
# cached pipeline output is  still valid and wouldn't re-evaluate the modifiers.
node.source.position.changed()

# Evaluate modification pipeline a second time.
# Note that compute() may return cached results if it thinks the 
# pipeline's input hasn't changed since the last call.
output2 = node.compute()
print('Number of remaining particles: ', output2.position.size)
assert(output2.position.size == output1.position.size - 1)