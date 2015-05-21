from ovito.io import import_file

# Load a simulation file. 
# This creates a node with a FileSource, which also is a DataCollection.
node = import_file('simulation.dump')
file_source = node.source

# Access particle data cached in the DataCollection.
print('Simulation cell:')
print(file_source.cell.matrix)
print('Particle coordinates:')
print(file_source.position.array)
