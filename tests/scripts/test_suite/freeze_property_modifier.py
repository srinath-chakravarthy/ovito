from ovito.io import *
from ovito.modifiers import *

node = import_file("../../files/CFG/shear.void.120.cfg")

modifier = FreezePropertyModifier()
node.modifiers.append(modifier)

print("Parameter defaults:")

print("  source_property: {}".format(modifier.source_property))
modifier.source_property = "Force"

print("  destination_property: {}".format(modifier.destination_property))
modifier.destination_property = "Force0"

modifier.take_snapshot()

node.compute()

print(node.output)
print(node.output.particle_properties['Force'].array)
print(node.output.particle_properties['Force0'].array)
