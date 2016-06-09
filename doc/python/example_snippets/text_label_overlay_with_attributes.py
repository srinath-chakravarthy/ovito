import ovito
from ovito.io import import_file
from ovito.vis import TextLabelOverlay
from ovito.modifiers import SelectExpressionModifier

# Import a simulation dataset and select some atoms based on their potential energy:
node = import_file("simulation.dump")
node.add_to_scene()
node.modifiers.append(SelectExpressionModifier(expression = 'peatom > -4.2'))

# Create the overlay. Note that the text string contains a reference
# to an output attribute of the SelectExpressionModifier.
overlay = TextLabelOverlay(text = 'Number selected atoms: [SelectExpression.num_selected]')
# Specify source of dynamically computed attributes.
overlay.source_node = node

# Attach overlay to the active viewport.
viewport = ovito.dataset.viewports.active_vp
viewport.overlays.append(overlay)