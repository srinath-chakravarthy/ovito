from ovito.io import *
from ovito.data import *

node = import_file("../../files/LAMMPS/animation.dump.gz")

num_particles = node.source.number_of_particles

cutoff = 1.5
finder = CutoffNeighborFinder(cutoff, node.source)

for index in range(num_particles):
    for n in finder.find(index):
        assert(n.index >= 0 and n.index < num_particles)
        assert(n.distance <= cutoff)
        assert(n.distance_squared <= cutoff*cutoff)
        assert(len(n.delta) == 3)
        assert(len(n.pbc_shift) == 3)