import ovito
from ovito.io import *
from ovito.data import *
from ovito.vis import *

node = import_file("../../files/LAMMPS/animation.dump.gz", multiple_frames = True)
node.add_to_scene()
node.source.particle_properties.position.display.radius = 0.1
print(node.source.num_frames)

traj_node = ovito.ObjectNode()
traj_node.add_to_scene()
traj_node.source = TrajectoryLineGenerator()

print(traj_node.source.source_node)
print(traj_node.source.only_selected)
print(traj_node.source.unwrap_trajectories)
print(traj_node.source.sampling_frequency)
print(traj_node.source.frame_interval)

traj_node.source.sampling_frequency = 2
traj_node.source.frame_interval = (0, 9)
assert(traj_node.source.frame_interval == (0, 9))

traj_node.source.source_node = node
traj_node.source.only_selected = False
assert(traj_node.source.generate())

dis = traj_node.source.display
print(dis.width)
print(dis.color)
print(dis.shading)
print(dis.upto_current_time)
assert(dis.shading == TrajectoryLineDisplay.Shading.Flat)

dis.shading = TrajectoryLineDisplay.Shading.Normal