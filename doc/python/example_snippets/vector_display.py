from ovito.io import import_file
from ovito.vis import VectorDisplay

node = import_file("simulation.dump")
node.add_to_scene()
vector_display = node.source.particle_properties.force.display
vector_display.color = (1.0, 0.5, 0.5)