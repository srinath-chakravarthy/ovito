from ovito.io import import_file
from ovito.data import CutoffNeighborFinder

# Load input simulation file.
node = import_file("simulation.dump")
data = node.source

# Initialize neighbor finder object:
cutoff = 3.5
finder = CutoffNeighborFinder(cutoff, data)

# Iterate over all input particles:
for index in range(data.number_of_particles):
    print("Neighbors of particle %i:" % index)
    # Iterate over the neighbors of the current particle:
    for neigh in finder.find(index):
        print(neigh)