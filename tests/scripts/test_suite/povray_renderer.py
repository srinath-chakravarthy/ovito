import ovito
from ovito.io import import_file
from ovito.vis import *

import sys
if "ovito.plugins.POVRay" not in sys.modules: sys.exit()

test_data_dir = "../../files/"
node1 = import_file(test_data_dir + "LAMMPS/class2.data", atom_style = "full")
node1.add_to_scene()
node1.source.particle_properties.position.display.radius = 0.3

settings = RenderSettings(size = (100,100))
settings.renderer = POVRayRenderer(show_window = False)

print("POV-Ray executable:", settings.renderer.povray_executable)
print("quality_level:", settings.renderer.quality_level)
print("antialiasing:", settings.renderer.antialiasing)
print("show_window:", settings.renderer.show_window)
print("radiosity:", settings.renderer.radiosity)
print("radiosity_raycount:", settings.renderer.radiosity_raycount)
print("depth_of_field:", settings.renderer.depth_of_field)
print("focal_length:", settings.renderer.focal_length)
print("aperture:", settings.renderer.aperture)
print("blur_samples:", settings.renderer.blur_samples)

ovito.dataset.viewports.active_vp.render(settings)
