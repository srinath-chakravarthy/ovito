from ovito import *
from ovito.vis import RenderSettings
from PyQt5.QtGui import QPainter

# Let OVITO render an image of the active viewport.
vp = dataset.viewports.active_vp
rs = RenderSettings(size = (320,240))
image = vp.render(rs)

# Paint something on top of the rendered image.
painter = QPainter(image)
painter.drawText(10, 20, "Hello world!")
del painter

# Save image to disk.
image.save("image.png")