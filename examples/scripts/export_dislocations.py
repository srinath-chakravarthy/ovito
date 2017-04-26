""" 
    This OVITO script exports the dislocation lines extracted by the
    DXA analysis modifier to a file.
    
    You should run this script from within the graphical user interface
    using the Scripting->Run Script File menu option.
"""
import ovito
from ovito.data import *

def export_dislocations(disloc_network):
    print("%i dislocation segments" % len(network.segments))
    for segment in network.segments:
        print("Segment %i: length=%f, Burgers vector=%s" % (segment.id, segment.length, segment.true_burgers_vector))
        print(segment.points)
        

# Loop over all objects in the scene and
# find one that contains dislocation data.
# Then call export_dislocations().
for i in range(len(ovito.dataset.scene_nodes)):
    node = ovito.dataset.scene_nodes[i]
    data = node.compute()    
    if hasattr(data, 'dislocations'): 
        export_dislocations(data.dislocations)
        break
