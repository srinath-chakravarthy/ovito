from ovito.io import *
from ovito.data import *
from ovito.modifiers import *
import numpy as np

node = import_file("simulation.*.dump")

# Perform Wigner-Seitz analysis:
ws = WignerSeitzAnalysisModifier(
    per_type_occupancies = True, 
    eliminate_cell_deformation = True)
ws.reference.load("simulation.0.dump")
node.modifiers.append(ws)

# Define a modifier function that selects sites of type A=1 which
# are occupied by exactly one atom of type B=2.
def modify(frame, input, output):

    # Retrieve the two-dimensional Numpy array with the site occupancy numbers.
    occupancies = input.particle_properties['Occupancy'].array
    
    # Get the site types as additional input:
    site_type = input.particle_properties.particle_type.array

    # Calculate total occupancy of every site:
    total_occupancy = np.sum(occupancies, axis=1)

    # Set up a particle selection by creating the Selection property:
    selection = output.create_particle_property(ParticleProperty.Type.Selection).marray
    
    # Select A-sites occupied by exactly one B-atom (the second entry of the Occupancy
    # array must be 1, and all others 0). Note that the Occupancy array uses 0-based
    # indexing, while atom type IDs are typically 1-based.
    selection[:] = (site_type == 1) & (occupancies[:,1] == 1) & (total_occupancy == 1)
    
    # Additionally output the total number of antisites as a global attribute:
    output.attributes['Antisite_count'] = np.count_nonzero(selection)

# Insert Python modifier into the data pipeline.
node.modifiers.append(PythonScriptModifier(function = modify))

# Let OVITO do the computation and export the number of identified 
# antisites as a function of simulation time to a text file:
export_file(node, "antisites.txt", "txt", 
    columns = ['Timestep', 'Antisite_count'],
    multiple_frames = True)

# Export the XYZ coordinates of just the antisites by removing all other atoms.
node.modifiers.append(InvertSelectionModifier())
node.modifiers.append(DeleteSelectedParticlesModifier())
export_file(node, "antisites.xyz", "xyz", 
    columns = ['Position.X', 'Position.Y', 'Position.Z'],
    multiple_frames = True)
