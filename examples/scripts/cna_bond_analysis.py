# Import OVITO modules.
from ovito.io import *
from ovito.modifiers import *
from ovito.data import *

# Import standard Python and NumPy modules.
import sys
import numpy

# Load the simulation dataset to be analyzed.
node = import_file("../data/NanocrystallinePd.dump.gz")

# Create bonds.
node.modifiers.append(CreateBondsModifier(cutoff = 3.5))

# Compute CNA indices on the basis of the created bonds.
node.modifiers.append(
        CommonNeighborAnalysisModifier(mode = CommonNeighborAnalysisModifier.Mode.BondBased))
                      
# Let OVITO's data pipeline do the heavy work.
node.compute()

# A two-dimensional array containing the three CNA indices 
# computed for each bond in the system.
cna_indices = node.output.bond_properties['CNA Indices'].array

# This helper function takes a two-dimensional array and computes the frequency 
# histogram of the data rows using some NumPy magic. 
# It returns two arrays (of same length): 
#    1. The list of unique data rows from the input array
#    2. The number of occurences of each unique row
def row_histogram(a):
    ca = numpy.ascontiguousarray(a).view([('', a.dtype)] * a.shape[1])
    unique, indices, inverse = numpy.unique(ca, return_index=True, return_inverse=True)
    counts = numpy.bincount(inverse)
    return (a[indices], counts)

# Used below for enumerating the bonds of each particle:
bond_enumerator = Bonds.Enumerator(node.output.bonds)

# Loop over particles and print their CNA indices.
for particle_index in range(node.output.number_of_particles):
    
    # Print particle index (1-based).
    sys.stdout.write("%i " % (particle_index+1))
    
    # Create local list with CNA indices of the bonds of the current particle.
    bond_index_list = list(bond_enumerator.bonds_of_particle(particle_index))
    local_cna_indices = cna_indices[bond_index_list]

    # Count how often each type of CNA triplet occurred.
    unique_triplets, triplet_counts = row_histogram(local_cna_indices)
    
    # Print list of triplets with their respective counts.
    for triplet, count in zip(unique_triplets, triplet_counts):
        sys.stdout.write("%s:%i " % (triplet, count))
    
    # End of particle line
    sys.stdout.write("\n")
