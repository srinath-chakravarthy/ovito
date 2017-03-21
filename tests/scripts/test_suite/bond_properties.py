from ovito.io import *
from ovito.data import *
import numpy

node = import_file("../../files/LAMMPS/bonds.data.gz", atom_style = 'bond')

print(node.source)
node.source.create_bond_property(BondProperty.Type.Color)
node.source.create_user_bond_property("MyProperty", "int", 2)
values = numpy.ones(node.source.number_of_half_bonds)
node.source.create_user_bond_property("MyProperty2", "float", 1, values)

print("Number of data objects: ", len(node.source))
print(node.source.bond_properties)
print(node.source.bond_properties.bond_type)
print(list(node.source.bond_properties.keys()))
print(list(node.source.bond_properties.values()))
print(node.source.bond_properties["Bond Type"])
print(node.source.bond_properties.bond_type.array)
print(node.source.bond_properties["MyProperty2"].array)

