from ovito.io import import_file
from ovito.modifiers import PythonScriptModifier
from ovito.data import ParticleProperty

# Load input data and create an ObjectNode with a modification pipeline.
node = import_file("simulation.dump")

# Define our custom modifier function, which assigns a uniform color 
# to all particles, just like the built-in Assign Color modifier. 
def assign_color(frame, input, output):
    color_property = output.create_particle_property(ParticleProperty.Type.Color)
    color_property.marray[:] = (0.0, 0.5, 1.0)

# Insert custom modifier into the data pipeline.
modifier = PythonScriptModifier(function = assign_color)
node.modifiers.append(modifier)