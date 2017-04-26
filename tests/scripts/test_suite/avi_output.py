import ovito
from ovito.io import *
from ovito.vis import *
import os
import os.path

node = import_file("../../files/LAMMPS/animation.dump.gz", multiple_frames = True)
node.add_to_scene()

vp = ovito.dataset.viewports.active_vp

output_file = "_movie.avi"

if os.path.isfile(output_file):
    os.remove(output_file)
assert(not os.path.isfile(output_file))

settings = RenderSettings(
    filename = output_file,
    size = (64, 64),
    range = RenderSettings.Range.ANIMATION        
)
if ovito.headless_mode: 
    settings.renderer = TachyonRenderer(ambient_occlusion = False, antialiasing = False)
vp.render(settings)

assert(os.path.isfile(output_file))
os.remove(output_file)
