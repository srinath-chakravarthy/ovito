# Import OVITO modules.
from ovito import *
from ovito.io import *
from ovito.vis import *

from PyQt5.QtGui import QPainter

# Import a data file.
node = import_file("../../files/CFG/shear.void.120.cfg")
settings = RenderSettings(size = (100,100))
if ovito.headless_mode:
    settings.renderer = TachyonRenderer(ambient_occlusion = False, antialiasing = False)
img = dataset.viewports.active_vp.render(settings)
assert(img.width() == 100)
painter = QPainter(img)
painter.eraseRect(20, 10, 40, 50)
del painter