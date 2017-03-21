from ovito.io import *
from ovito.data import *
from ovito.modifiers import *

import sys
if "ovito.plugins.CrystalAnalysis" not in sys.modules: sys.exit()

import numpy

node = import_file("../../files/CFG/fcc_coherent_twin.0.cfg")
modifier = ElasticStrainModifier()

print("Parameter defaults:")
print("  calculate_deformation_gradients: {}".format(modifier.calculate_deformation_gradients))
print("  calculate_strain_tensors: {}".format(modifier.calculate_strain_tensors))
print("  push_strain_tensors_forward: {}".format(modifier.push_strain_tensors_forward))
print("  lattice_constant: {}".format(modifier.lattice_constant))
print("  axial_ratio: {}".format(modifier.axial_ratio))
print("  input_crystal_structure: {}".format(modifier.input_crystal_structure))
node.modifiers.append(modifier)

modifier.input_crystal_structure = ElasticStrainModifier.Lattice.FCC
modifier.lattice_constant = 0.99
modifier.calculate_deformation_gradients = True

node.compute()
print("Computed structure types:")
print(node.output.particle_properties.structure_type.array)
print("Computed strain tensors:")
print(node.output.particle_properties['Elastic Strain'].array)
print("Computed deformation gradient tensors:")
print(node.output.particle_properties['Elastic Deformation Gradient'].array)
print("Computed volumetric strain:")
print(node.output.particle_properties['Volumetric Strain'].array)
