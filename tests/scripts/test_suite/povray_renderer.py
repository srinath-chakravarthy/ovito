import ovito
from ovito.io import import_file
from ovito.vis import *

test_data_dir = "../../files/"
node1 = import_file(test_data_dir + "LAMMPS/class2.data", atom_style = "full")
node1.add_to_scene()
node1.source.particle_properties.position.display.radius = 0.3

settings = RenderSettings(size = (100,100))
settings.renderer = POVRayRenderer(show_window = False)
ovito.dataset.viewports.active_vp.render(settings)
