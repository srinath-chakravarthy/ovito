import ovito
from ovito.vis import CoordinateTripodOverlay

# Create an overlay.
tripod = CoordinateTripodOverlay()
tripod.size = 0.07

# Attach overlay to the active viewport.
viewport = ovito.dataset.viewports.active_vp
viewport.overlays.append(tripod)