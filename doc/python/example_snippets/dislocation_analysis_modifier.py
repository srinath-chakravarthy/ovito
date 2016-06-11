from ovito.io import import_file, export_file
from ovito.modifiers import DislocationAnalysisModifier

node = import_file("simulation.dump")

# Extract dislocation lines from a crystal with diamond structure:
modifier = DislocationAnalysisModifier()
modifier.input_crystal_structure = DislocationAnalysisModifier.Lattice.CubicDiamond
node.modifiers.append(modifier)
node.compute()

total_line_length = node.output.attributes['DislocationAnalysis.total_line_length']
print("Dislocation density: %f" % (total_line_length / node.output.cell.volume))

# Print list of dislocation lines:
network = node.output.dislocations
print("Found %i dislocation segments" % len(network.segments))
for segment in network.segments:
    print("Segment %i: length=%f, Burgers vector=%s" % (segment.id, segment.length, segment.true_burgers_vector))
    print(segment.points)

# Export dislocation lines to a CA file:
export_file(node, "dislocations.ca", "ca")
