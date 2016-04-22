from ovito.io import import_file
from ovito.modifiers import LoadTrajectoryModifier

# Load static topology data from a LAMMPS data file.
node = import_file('input.data', atom_style = 'bond')

# Load atom trajectories from separate LAMMPS dump file.
traj_mod = LoadTrajectoryModifier()
traj_mod.source.load('trajectory.dump', multiple_frames = True)

# Insert modifier into modification pipeline.
node.modifiers.append(traj_mod)