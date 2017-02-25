from ovito.io import *
from ovito.data import *

node = import_file("../../files/LAMMPS/bonds.data.gz", atom_style = 'bond')

print("Attributes:")
for attr in node.source.attributes:
    print(attr, node.source.attributes[attr])

node.source.attributes['new_attribute'] = 2.8

print()
print("Particle properties:")
for p in node.source.particle_properties:
    print(p)

print()
print("Particles:")
print(node.source.number_of_particles)

print()
print("Bond properties:")
for p in node.source.bond_properties:
    print(p)

print()
print("Cell:")
print(node.source.cell.matrix)

print()
print("Bonds:")
print(node.source.number_of_full_bonds)
print(node.source.number_of_half_bonds)
print(node.source.bonds)

old_prop = node.source.particle_properties.molecule_identifier
new_prop = node.source.copy_if_needed(old_prop)
print(old_prop, new_prop)
assert(old_prop is new_prop)

node.compute()

old_prop = node.source.particle_properties.molecule_identifier
new_prop = node.source.copy_if_needed(old_prop)
print(old_prop, new_prop)
assert(old_prop is not new_prop)

mass_prop = node.source.create_particle_property(ParticleProperty.Type.Mass)
mass_prop.marray[0] = 0.5
assert(mass_prop in node.source.particle_properties.values())

color_prop = node.source.create_bond_property(BondProperty.Type.Color)
color_prop.marray[0] = (0.5, 0.1, 0.9)
assert(color_prop in node.source.bond_properties.values())
