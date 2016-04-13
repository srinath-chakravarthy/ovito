from ovito import dataset
from ovito.io import import_file

# Import a sequence of files.
node = import_file('simulation.*.dump')

# Loop over all frames of the sequence.
for frame in range(node.source.num_frames):    
    # Let the node load the corresponding file into its cache.
    node.compute(frame)
    # Access the loaded data of the current frame,
    print("Number of atoms at frame %i is %i." % 
          (node.source.loaded_frame, node.source.number_of_particles))
