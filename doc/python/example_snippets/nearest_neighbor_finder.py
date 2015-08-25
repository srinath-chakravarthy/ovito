from ovito.io import import_file
from ovito.data import NearestNeighborFinder

# Load input simulation file.
node = import_file("simulation.dump")
data = node.source

# Initialize neighbor finder object.
# Visit the 12 nearest neighbors of each particle.
N = 12
finder = NearestNeighborFinder(N, data)

# Loop over all input particles:
for index in range(data.number_of_particles):
    print("Nearest neighbors of particle %i:" % index)
    # Iterate over the neighbors of the current particle, starting with the closest:
    for neigh in finder.find(index):
        print(neigh.index, neigh.distance, neigh.delta)