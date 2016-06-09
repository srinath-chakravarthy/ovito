import ovito
from ovito.vis import CoordinateTripodOverlay
from PyQt5 import QtCore

# Create an overlay.
tripod = CoordinateTripodOverlay()
tripod.size = 0.07
tripod.alignment = QtCore.Qt.AlignRight ^ QtCore.Qt.AlignBottom

# Attach overlay to the active viewport.
viewport = ovito.dataset.viewports.active_vp
viewport.overlays.append(tripod)