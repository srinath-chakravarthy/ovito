import ovito
from ovito.io import *
from ovito.vis import OpenGLRenderer, RenderSettings

if not ovito.headless_mode:

    # Import a data file.
    node = import_file("../../files/CFG/shear.void.120.cfg")

    renderer = OpenGLRenderer()

    print("Parameter defaults:")
    print("  antialiasing_level: {}".format(renderer.antialiasing_level))
    renderer.antialiasing_level = 2

    settings = RenderSettings(size = (100,100), renderer = renderer)
    ovito.dataset.viewports.active_vp.render(settings)
