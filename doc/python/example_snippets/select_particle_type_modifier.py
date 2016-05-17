from ovito.io import import_file
from ovito.modifiers import SelectParticleTypeModifier, CommonNeighborAnalysisModifier

node = import_file("simulation.dump")
cnamod = CommonNeighborAnalysisModifier()
node.modifiers.append(cnamod)

modifier = SelectParticleTypeModifier(property = "Structure Type")
modifier.types = { CommonNeighborAnalysisModifier.Type.FCC,
                   CommonNeighborAnalysisModifier.Type.HCP }
node.modifiers.append(modifier)
node.compute()
print("Number of FCC/HCP atoms: %i" % node.output.attributes['SelectParticleType.num_selected'])