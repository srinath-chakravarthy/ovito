import ovito.io
node = ovito.io.import_file('simulation.dump')

# Get the ParticleTypeProperty instance:
type_property = node.source.particle_properties.particle_type

# Print the numeric type of each atom:
print(type_property.array)

# For each atom, look up its numeric type ID in the
# type list. Then print the human-readable name of that type.
for atom_index in range(node.source.number_of_particles):
    ptype_id = type_property.array[atom_index]
    for ptype in type_property.type_list:
        if ptype.id == ptype_id:
            print(ptype.name)
            break
