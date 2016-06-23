from ovito.io import import_file
from ovito.vis import ParticleDisplay

node = import_file("simulation.dump")
node.add_to_scene()
particle_display = node.source.particle_properties.position.display
particle_display.shape = ParticleDisplay.Shape.Square