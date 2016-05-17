from ovito.io import import_file
from ovito.modifiers import CommonNeighborAnalysisModifier

node = import_file("simulation.dump")

node.modifiers.append(CommonNeighborAnalysisModifier())
node.compute()
print("Number of FCC atoms: %i" % node.output.attributes['CommonNeighborAnalysis.counts.FCC'])
