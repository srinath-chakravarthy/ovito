from ovito.io import *

node = import_file("../../files/CFG/shear.void.120.cfg")

print(node.source)
print("Number of data objects: ", len(node.source))
print(node.source.particle_properties)
print(node.source.position)
print(list(node.source.particle_properties.keys()))
print(list(node.source.particle_properties.values()))
print(node.source.particle_properties["Position"])
print(node.source.particle_properties.position)
