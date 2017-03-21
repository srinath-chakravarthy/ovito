from ovito.io import *
from ovito.data import *
from ovito.modifiers import *

import numpy

node = import_file("../../files/CFG/fcc_coherent_twin.0.cfg")
node.modifiers.append(CreateBondsModifier(cutoff = 0.8))
modifier = CommonNeighborAnalysisModifier()

print("Parameter defaults:")
print("  mode: {}".format(modifier.mode))
print("  cutoff: {}".format(modifier.cutoff))
print("  only_selected: {}".format(modifier.only_selected))

modifier.mode = CommonNeighborAnalysisModifier.Mode.BondBased
modifier.mode = CommonNeighborAnalysisModifier.Mode.FixedCutoff
modifier.mode = CommonNeighborAnalysisModifier.Mode.AdaptiveCutoff

node.modifiers.append(modifier)

modifier.structures[CommonNeighborAnalysisModifier.Type.FCC].color = (1,0,0)

node.compute()
print("Computed structure types:")
print(node.output.structure_type.array)
print("Number of particles: {}".format(node.output.number_of_particles))
print("Number of FCC atoms: {}".format(node.output.attributes['CommonNeighborAnalysis.counts.FCC']))
print("Number of HCP atoms: {}".format(node.output.attributes['CommonNeighborAnalysis.counts.HCP']))
print("Number of BCC atoms: {}".format(node.output.attributes['CommonNeighborAnalysis.counts.BCC']))

assert(node.output.attributes['CommonNeighborAnalysis.counts.FCC'] == 128)
assert(node.output.particle_properties.structure_type.array[0] == 1)
assert(node.output.particle_properties.structure_type.array[0] == CommonNeighborAnalysisModifier.Type.FCC)
assert((node.output.particle_properties.color.array[0] == (1,0,0)).all())
assert(CommonNeighborAnalysisModifier.Type.OTHER == 0)
assert(CommonNeighborAnalysisModifier.Type.FCC == 1)
assert(CommonNeighborAnalysisModifier.Type.HCP == 2)
assert(CommonNeighborAnalysisModifier.Type.BCC == 3)
assert(CommonNeighborAnalysisModifier.Type.ICO == 4)

modifier.mode = CommonNeighborAnalysisModifier.Mode.BondBased
print(node.output)
node.compute()
print("Number of full bonds: ", node.output.number_of_full_bonds)
print(node.output)
print("Number of FCC atoms: {}".format(node.output.attributes['CommonNeighborAnalysis.counts.FCC']))
assert(node.output.attributes['CommonNeighborAnalysis.counts.FCC'] == 128)
print(node.output.bond_properties['CNA Indices'].array)
