from ovito import ObjectNode
from ovito.io import import_file
from ovito.data import TrajectoryLineGenerator
from ovito.vis import TrajectoryLineDisplay

# Load a particle simulation sequence:
particle_node = import_file('simulation.*.dump')
particle_node.add_to_scene()

# Create a second scene node for the trajectory lines:
traj_node = ObjectNode()
traj_node.add_to_scene()

# Create data source and assign it to the new scene node:
traj_node.source = TrajectoryLineGenerator(
    source_node = particle_node,
    only_selected = False
)

# Generate the trajectory lines by sampling the 
# particle positions over the entire animation interval.
traj_node.source.generate()

# Adjust trajectory display settings:
traj_node.source.display.width = 0.4
traj_node.source.display.color = (1,0,0)
traj_node.source.display.shading = TrajectoryLineDisplay.Shading.Flat
