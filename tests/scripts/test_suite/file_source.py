from ovito.io import *
from ovito.modifiers import *
from ovito.data import *
import numpy as np

node = import_file("../../files/CFG/shear.void.120.cfg")
file_source = node.source

print(file_source.source_path)
print(file_source)
assert(isinstance(file_source, DataCollection))
assert(file_source.loaded_frame == 0)
assert(file_source.num_frames == True)
assert(file_source.adjust_animation_interval == True)
