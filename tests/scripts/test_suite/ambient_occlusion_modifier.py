import ovito
from ovito.io import *
from ovito.modifiers import *

node = import_file("../../files/CFG/shear.void.120.cfg")

modifier = AmbientOcclusionModifier()
node.modifiers.append(modifier)

print(modifier.buffer_resolution)
modifier.buffer_resolution = 4

print(modifier.intensity)
modifier.intensity = 0.9

print(modifier.sample_count)
modifier.sample_count = 30

if not ovito.headless_mode: # Ambient occlusion modifier requires OpenGL support.
    node.compute()
