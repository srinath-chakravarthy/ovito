import ovito
from ovito.vis import PythonViewportOverlay

# Define a function that paints on top of the rendered image.
def render_overlay(painter, **args):
    painter.drawText(10, 10, "Hello world")

# Create the overlay.
overlay = PythonViewportOverlay(function = render_overlay)

# Attach overlay to the active viewport.
viewport = ovito.dataset.viewports.active_vp
viewport.overlays.append(overlay)
