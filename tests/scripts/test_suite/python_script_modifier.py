from ovito.io import *
from ovito.modifiers import *
from ovito.data import *
import numpy

node = import_file("../../files/CFG/shear.void.120.cfg")
modifier = PythonScriptModifier()
node.modifiers.append(modifier)

def compute_coordination(pindex, finder):
    return sum(1 for _ in finder.find(pindex))

def modify(frame, input, output):
    yield "Hello world"
    color_property = output.create_particle_property(ParticleProperty.Type.Color)
    color_property.marray[:] = (0,0.5,0)
    my_property = output.create_user_particle_property("MyCoordination", "int")
    finder = CutoffNeighborFinder(3.5, input)
    for index in range(input.number_of_particles):
        if index % 100 == 0: yield index/input.number_of_particles
        my_property.marray[index] = compute_coordination(index, finder)
    
modifier.function = modify
node.compute()

assert((node.output.color.array[0] == numpy.array([0,0.5,0])).all())
assert(node.output["MyCoordination"].array[0] > 0)