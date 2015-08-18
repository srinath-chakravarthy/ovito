from ovito.io import import_file
from ovito.modifiers import DislocationAnalysisModifier

# Extract dislocation lines from the crystal:
node = import_file("simulation.dump")
modifier =  DislocationAnalysisModifier()
modifier.input_crystal_structure = DislocationAnalysisModifier.Lattice.CubicDiamond
node.modifiers.append(modifier)
node.compute()

# Print list of dislocation lines:
network = node.output.dislocations
print("Found %i dislocation segments" % len(network.segments))
for segment in network.segments:
    print("Segment %i: length=%f, Burgers vector=%s" % (segment.id, segment.length, segment.true_burgers_vector))
    print(segment.points)
