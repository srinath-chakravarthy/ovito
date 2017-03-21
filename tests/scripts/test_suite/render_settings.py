import ovito
from ovito.io import *
from ovito.vis import *

# Import a data file.
node = import_file("../../files/CFG/shear.void.120.cfg")
node.add_to_scene()

settings = RenderSettings(size = (20,20))
if ovito.headless_mode: 
    settings.renderer = TachyonRenderer(ambient_occlusion = False, antialiasing = False)
settings.background_color = (0.8,0.8,1.0)
ovito.dataset.viewports.active_vp.render(settings)
