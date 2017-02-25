from ovito.io import *
from ovito.modifiers import *

import numpy

node = import_file("../../files/LAMMPS/animation.dump.gz")
modifier = ColorCodingModifier()

node.modifiers.append(modifier)

print("Parameter defaults:")

print("  start_value: {}".format(modifier.start_value))
print("  end_value: {}".format(modifier.end_value))
print("  gradient: {}".format(modifier.gradient))
print("  only_selected: {}".format(modifier.only_selected))
print("  assign_to: {}".format(modifier.assign_to))
print("  particle_property: {}".format(modifier.particle_property))
print("  bond_property: {}".format(modifier.bond_property))

modifier.gradient = ColorCodingModifier.Rainbow()
modifier.gradient = ColorCodingModifier.Jet()
modifier.gradient = ColorCodingModifier.Hot()
modifier.gradient = ColorCodingModifier.Grayscale()
modifier.gradient = ColorCodingModifier.BlueWhiteRed()
modifier.gradient = ColorCodingModifier.Viridis()
modifier.gradient = ColorCodingModifier.Magma()
modifier.gradient = ColorCodingModifier.Custom("../../../doc/manual/images/modifiers/color_coding_custom_map.png")

print(node.compute().particle_properties.color.array)

modifier.particle_property = "Position.X"
node.compute()

modifier = ColorCodingModifier(
    particle_property = "Position.Z",
    bond_property = "Length",
    start_value = 0.0,
    end_value = 0.0,
    only_selected = True,
    gradient = ColorCodingModifier.Grayscale(),
    assign_to = ColorCodingModifier.AssignmentMode.Bonds
)
