from ovito.io import import_file
from ovito.data import Bonds

# Load a system of atoms and bonds.
node = import_file('bonds.data.gz', atom_style = 'bond')

# Create bond enumerator object.
bonds_enum = Bonds.Enumerator(node.source.bonds)

# Loop over atoms.
bonds_array = node.source.bonds.array
for particle_index in range(node.source.number_of_particles):
    # Loop over half-bonds of current atom.
    for bond_index in bonds_enum.bonds_of_particle(particle_index):
        atomA = bonds_array[bond_index][0]
        atomB = bonds_array[bond_index][1]
        assert(atomA == particle_index)
        print("Atom %i has a bond to atom %i" % (atomA, atomB))
