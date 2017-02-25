import sys

import numpy as np

import ovito
from ovito.data import *
from ovito.modifiers import *

from ase.lattice import bulk
from ase.io import read, write
from ase.calculators.singlepoint import SinglePointCalculator

atoms = bulk('Si', cubic=True)
atoms *= (5, 5, 5)

# add some comuted properties
atoms.new_array('real', (atoms.positions**2).sum(axis=1))
atoms.new_array('int', np.array([-1]*len(atoms)))

# calculate energy and forces with dummy calculator
forces = -1.0*atoms.positions
spc = SinglePointCalculator(atoms,
							forces=forces)
atoms.set_calculator(spc)

# convert from Atoms to DataCollection
data = DataCollection.create_from_ase_atoms(atoms)

# Create a node and insert it into the scene
node = ObjectNode()
node.source = data
ovito.dataset.scene_nodes.append(node)

new_data = node.compute()

# Select the new node and adjust viewport cameras to show everything.
ovito.dataset.selected_node = node
for vp in ovito.dataset.viewports:
    vp.zoom_all()

# Do the reverse conversion, after pipeline has been applied
atoms = new_data.to_ase_atoms()

# Dump results to disk
atoms.write('dump.extxyz')
