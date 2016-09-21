from ovito.io import *
from ovito.modifiers import CombineParticleSetsModifier

# Load a first set of particles.
node = import_file('first_file.dump')

# Insert the particles from a second file into the dataset. 
modifier = CombineParticleSetsModifier()
modifier.source.load('second_file.dump')
node.modifiers.append(modifier)

# Export combined dataset to a new file.
export_file(node, 'output.dump', 'lammps_dump',
            columns = ['Position.X', 'Position.Y', 'Position.Z'])

# The particle set leaving the pipeline is the union of the two inputs.
assert(node.compute().number_of_particles == node.source.number_of_particles + modifier.source.number_of_particles)