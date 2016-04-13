from ovito.io import import_file

# This creates a node with a FileSource, which also is a DataCollection.
node = import_file('simulation.dump')

# Access data cached in the DataCollection.
print(node.source.number_of_particles)
print(node.source.cell.matrix)