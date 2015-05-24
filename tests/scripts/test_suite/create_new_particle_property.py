from ovito import *
from ovito.data import *

# The number of particles we are going to create.
num_particles = 3

# Create the particles position property.
pos_prop = ParticleProperty.create(ParticleProperty.Type.Position, num_particles)
pos_prop.mutable_array[0] = (1.0, 1.5, 0.3)
pos_prop.mutable_array[1] = (7.0, 4.2, 6.0)
pos_prop.mutable_array[2] = (5.0, 9.2, 8.0)

# Create the particle type property and insert two atom types.
type_prop = ParticleProperty.create(ParticleProperty.Type.ParticleType, num_particles)
type_prop.type_list.append(ParticleType(id = 1, name = 'Cu', color = (0.0,1.0,0.0)))
type_prop.type_list.append(ParticleType(id = 2, name = 'Ni', color = (0.0,0.5,1.0)))
type_prop.mutable_array[0] = 1  # First atom is Cu
type_prop.mutable_array[1] = 2  # Second atom is Ni
type_prop.mutable_array[2] = 2  # Third atom is Ni

# Create a user-defined property.
my_prop = ParticleProperty.create_user('My property', 'float', num_particles)
my_prop.mutable_array[0] = 3.141
my_prop.mutable_array[1] = 0.0
my_prop.mutable_array[2] = 0.0

# Create a simulation box.
cell = SimulationCell()
cell.matrix = [[10,0,0,0],
               [0,10,0,0],
               [0,0,10,0]]
cell.pbc = (True, True, True)
cell.display.line_width = 0.1

# Create bonds between particles.
bonds = Bonds()
bonds.add_full(0, 1)    # Creates two half bonds 0->1 and 1->0. 
bonds.add_full(1, 2)    # Creates two half bonds 1->2 and 2->1.
bonds.add_full(2, 0)    # Creates two half bonds 2->0 and 0->2.

# Create a data collection to hold the particle properties, the bonds, and the simulation cell.
data = DataCollection()
data.add(pos_prop)
data.add(type_prop)
data.add(my_prop)
data.add(cell)
data.add(bonds)

# Create a node and insert it into the scene.
node = ObjectNode()
node.source = data
dataset.scene_nodes.append(node)

# Select the new node and adjust viewport cameras to show everything.
dataset.selected_node = node
for vp in dataset.viewports:
    vp.zoom_all()
