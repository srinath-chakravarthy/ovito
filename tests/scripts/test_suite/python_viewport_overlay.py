from ovito import *
from ovito.io import *
from ovito.modifiers import *
from ovito.vis import *

import numpy

node = import_file("../../files/CFG/fcc_coherent_twin.0.cfg")
node.modifiers.append(CommonNeighborAnalysisModifier())

vp = dataset.viewports.active_vp

new_overlay = ovito.vis.PythonViewportOverlay()
new_overlay.script = """
def render(painter, **args):    
    painter.drawText(10, 10, "Hello world")
"""
vp.overlays.append(new_overlay)

assert(len(vp.overlays) == 1)
assert(vp.overlays[0] == new_overlay)
assert(new_overlay.output == "")

overlay2 = ovito.vis.PythonViewportOverlay()
overlay2.script = "jhgjhg"
vp.overlays.append(overlay2)

assert(len(vp.overlays) == 2)
assert(overlay2.output != "")
