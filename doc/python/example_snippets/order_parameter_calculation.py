from ovito.io import import_file
from ovito.modifiers import PythonScriptModifier

# Load input data and create an ObjectNode with a data pipeline.
node = import_file("simulation.dump")

from ovito.data import NearestNeighborFinder
import numpy as np

# The lattice constant of the FCC crystal:
lattice_parameter = 3.6 

# The list of <110> ideal neighbor vectors of the reference lattice (FCC):
reference_vectors = np.asarray([
    (0.5, 0.5, 0.0),
    (-0.5, 0.5, 0.0),
    (0.5, -0.5, 0.0),
    (-0.5, -0.5, 0.0),
    (0.0, 0.5, 0.5),
    (0.0, -0.5, 0.5),
    (0.0, 0.5, -0.5),
    (0.0, -0.5, -0.5),
    (0.5, 0.0, 0.5),
    (-0.5, 0.0, 0.5),
    (0.5, 0.0, -0.5),
    (-0.5, 0.0, -0.5)
])
# Rescale ideal lattice vectors with lattice constant.
reference_vectors *= lattice_parameter

# The number of neighbors to take into account per atom:
num_neighbors = len(reference_vectors)

def modify(frame, input, output):

    # Show a text in the status bar:
    yield "Calculating order parameters"

    # Create output property.
    order_param = output.create_user_particle_property(
        "Order Parameter", "float").marray
    
    # Prepare neighbor lists.
    neigh_finder = NearestNeighborFinder(num_neighbors, input)
    
    # Loop over all input particles
    nparticles = input.number_of_particles
    for i in range(nparticles):
        
        # Update progress percentage indicator
        yield (i/nparticles)
        
        oparam = 0.0	# The order parameter of the current atom		
        
        # Loop over neighbors of current atom.
        for neigh in neigh_finder.find(i):
            
            # Compute squared deviation of neighbor vector from every 
            # reference vector.
            squared_deviations = np.linalg.norm(
                reference_vectors - neigh.delta, axis=1) ** 2
            
            # Sum up the contribution from the best-matching vector.
            oparam += np.min(squared_deviations)
            
        # Store result in output particle property.
        order_param[i] = oparam / num_neighbors		

node.modifiers.append(PythonScriptModifier(function = modify))
node.compute()
