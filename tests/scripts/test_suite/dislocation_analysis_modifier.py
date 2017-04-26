from ovito.io import *
from ovito.modifiers import *
import numpy as np

import sys
if "ovito.plugins.CrystalAnalysis" not in sys.modules: sys.exit()

node = import_file("../../files/LAMMPS/frank_read.dump.gz")

modifier = DislocationAnalysisModifier()
node.modifiers.append(modifier)

print("Parameter defaults:")

print("  trial_circuit_length: {}".format(modifier.trial_circuit_length))
print("  circuit_stretchability: {}".format(modifier.circuit_stretchability))
print("  input_crystal_structure: {}".format(modifier.input_crystal_structure))
#print("  reconstruct_edge_vectors: {}".format(modifier.reconstruct_edge_vectors))
print("  line_smoothing_enabled: {}".format(modifier.line_smoothing_enabled))
print("  line_coarsening_enabled: {}".format(modifier.line_coarsening_enabled))
print("  line_smoothing_level: {}".format(modifier.line_smoothing_level))
print("  line_point_separation: {}".format(modifier.line_point_separation))
print("  defect_mesh_smoothing_level: {}".format(modifier.defect_mesh_smoothing_level))

modifier.input_crystal_structure = DislocationAnalysisModifier.Lattice.FCC

node.compute()

print("Output:")
print(node.output)
dislocations = node.output.dislocations

print(dislocations.segments)
print("Number of dislocation segments: {}".format(len(dislocations.segments)))

for segment in dislocations.segments:
    print(segment.id, segment.length, segment.is_loop, segment.is_loop, segment.true_burgers_vector, segment.spatial_burgers_vector, segment.cluster_id)
    print(segment.points)
    