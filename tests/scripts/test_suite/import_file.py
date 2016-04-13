import ovito
from ovito.io import import_file

test_data_dir = "../../files/"

node1 = import_file(test_data_dir + "LAMMPS/animation.dump.gz")
assert(len(ovito.dataset.scene_nodes) == 0)
node2 = import_file(test_data_dir + "CFG/fcc_coherent_twin.0.cfg")
node3 = import_file(test_data_dir + "Parcas/movie.0000000.parcas")
node4 = import_file(test_data_dir + "CFG/shear.void.120.cfg")
node5 = import_file(test_data_dir + "IMD/nw2.imd.gz")
node6 = import_file(test_data_dir + "FHI-aims/3_geometry.in.next_step")
node = import_file(test_data_dir + "LAMMPS/multi_sequence_*.dump")
assert(ovito.dataset.anim.last_frame == 2)
node = import_file(test_data_dir + "LAMMPS/shear.void.dump.bin", 
                            columns = ["Particle Identifier", "Particle Type", "Position.X", "Position.Y", "Position.Z"])
try:
    # This should generate an error:
    node = import_file(test_data_dir + "LAMMPS/shear.void.dump.bin",  
                                columns = ["Particle Identifier", "Particle Type", "Position.X", "Position.Y", "Position.Z", "ExtraProperty"])
    assert False
except RuntimeError:
    pass

node = import_file(test_data_dir + "LAMMPS/animation*.dump")
assert(ovito.dataset.anim.last_frame == 0)
node = import_file(test_data_dir + "LAMMPS/animation*.dump", multiple_frames = True)
assert(ovito.dataset.anim.last_frame == 10)
