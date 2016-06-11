from ovito.io import import_file
from ovito.modifiers import ClusterAnalysisModifier
import numpy

node = import_file("simulation.dump")
node.modifiers.append(ClusterAnalysisModifier(cutoff = 2.8, sort_by_size = True))
node.compute()

cluster_sizes = numpy.bincount(node.output.particle_properties['Cluster'].array)
numpy.savetxt("cluster_sizes.txt", cluster_sizes)
