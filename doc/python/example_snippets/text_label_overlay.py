import ovito
from ovito.vis import TextLabelOverlay
from PyQt5 import QtCore

# Create an overlay.
overlay = TextLabelOverlay(
    text = 'Some text', 
    alignment = QtCore.Qt.AlignHCenter ^ QtCore.Qt.AlignBottom,
    offset_y = 0.1,
    font_size = 0.03,
    text_color = (0,0,0))

# Attach overlay to the active viewport.
viewport = ovito.dataset.viewports.active_vp
viewport.overlays.append(overlay)