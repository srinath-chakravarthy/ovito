import ovito
from ovito.vis import PythonViewportOverlay

# Create the overlay.
overlay = PythonViewportOverlay()
overlay.script = """
def render(painter, **args):    
    painter.drawText(10, 10, "Hello world")
"""

# Attach overlay to the active viewport.
viewport = ovito.dataset.viewports.active_vp
viewport.overlays.append(overlay)
