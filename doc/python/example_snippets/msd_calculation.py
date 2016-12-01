from ovito.io import import_file, export_file
from ovito.modifiers import PythonScriptModifier, CalculateDisplacementsModifier
import numpy

# Load input data and create an ObjectNode with a data pipeline.
node = import_file("simulation.dump", multiple_frames = True)

# Calculate per-particle displacements with respect to initial simulation frame:
dmod = CalculateDisplacementsModifier()
dmod.reference.load("simulation.dump")
node.modifiers.append(dmod)

# Define the custom modifier function:
def modify(frame, input, output):

    # Access the per-particle displacement magnitudes computed by an existing 
    # Displacement Vectors modifier that precedes this custom modifier in the 
    # data pipeline:
    displacement_magnitudes = input.particle_properties.displacement_magnitude.array

    # Compute MSD:
    msd = numpy.sum(displacement_magnitudes ** 2) / len(displacement_magnitudes)

    # Output MSD value as a global attribute: 
    output.attributes["MSD"] = msd 

# Insert custom modifier into the data pipeline.
node.modifiers.append(PythonScriptModifier(function = modify))

# Export calculated MSD value to a text file and let OVITO's data pipeline do the rest:
export_file(node, "msd_data.txt", 
    format = "txt",
    columns = ["Timestep", "MSD"],
    multiple_frames = True)
