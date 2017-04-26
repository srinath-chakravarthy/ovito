from ovito.io import *
from ovito.modifiers import *
from ovito.data import *
import numpy as np

node = import_file("../../files/CFG/shear.void.120.cfg")
modifier = CommonNeighborAnalysisModifier()
node.modifiers.append(modifier)
node.compute()

print("Number of FCC atoms: {}".format(node.output.attributes['CommonNeighborAnalysis.counts.FCC']))

print(node.source.position.array)
print(node.source.position.marray)
node.source.position.marray[0] = (0,0,0)
node.source.position.changed()
print(node.source.position.array)

node.source = DataCollection()