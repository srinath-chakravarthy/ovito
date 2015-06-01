"""
This module contains data container classes that are used by OVITO's modification pipeline system.

**Data collection:**

  * The :py:class:`DataCollection` class is a container for multiple data objects and holds the results of a modification pipeline.

**Data objects:**

  * :py:class:`DataObject` (base class of all other data classes)
  * :py:class:`Bonds`
  * :py:class:`ParticleProperty`
  * :py:class:`ParticleTypeProperty`
  * :py:class:`SimulationCell`
  * :py:class:`SurfaceMesh`

**Helper classes:**

  * :py:class:`ParticleType`
  * :py:class:`CutoffNeighborFinder`

"""

import numpy as np

try:
    # Python 3.x
    import collections.abc as collections
except ImportError:
    # Python 2.x
    import collections

# Load the native module.
from PyScriptScene import DataCollection
from PyScriptScene import DataObject

# Give the DataCollection class a dict-like interface.
DataCollection.__len__ = lambda self: len(self.objects)
def _DataCollection__iter__(self):
    for o in self.objects:
        if hasattr(o, "_data_key"):
            yield o._data_key
        else:
            yield o.objectTitle
DataCollection.__iter__ = _DataCollection__iter__
def _DataCollection__getitem__(self, key):
    for o in self.objects:
        if hasattr(o, "_data_key"):
            if o._data_key == key:
                return o
        else:
            if o.objectTitle == key:
                return o
    raise KeyError("DataCollection does not contain object key '%s'." % key)
DataCollection.__getitem__ = _DataCollection__getitem__
def _DataCollection__getattr__(self, name):
    for o in self.objects:
        if hasattr(o, "_data_attribute_name"):
            if o._data_attribute_name == name:
                return o
    raise AttributeError("DataCollection does not have an attribute named '%s'." % name)
DataCollection.__getattr__ = _DataCollection__getattr__
def _DataCollection__str__(self):
    return "DataCollection(" + str(list(self.keys())) + ")"
DataCollection.__str__ = _DataCollection__str__
# Mix in base class collections.Mapping:
DataCollection.__bases__ = DataCollection.__bases__ + (collections.Mapping, )

# Implement 'display' attribute of DataObject class.
def _DataObject_display(self):
    """ The :py:class:`~ovito.vis.Display` object associated with this data object, which is responsible for
        displaying the data. If this field is ``None``, the data is of a non-visual type.
    """ 
    if not self.displayObjects:
        return None # This data object doesn't have a display object.
    return self.displayObjects[0]
DataObject.display = property(_DataObject_display)


def _DataCollection_to_ase_atoms(self):
    """
    Convert from ovito.data.DataCollection to ase.atoms.Atoms

    Raises an ImportError if ASE is not available
    """

    from ase.atoms import Atoms

    # Extract basic dat: pbc, cell, positions, particle types
    pbc = self.cell.pbc
    cell_matrix = np.array(self.cell.matrix)    
    cell, origin = cell_matrix[:, :3], cell_matrix[:, 3]
    info = {'cell_origin': origin }
    positions = np.array(self.position)
    type_names = dict([(t.id, t.name) for t in
                       self.particle_type.type_list])
    symbols = [type_names[id] for id in np.array(self.particle_type)]

    # construct ase.Atoms object
    atoms = Atoms(symbols,
                  positions,
                  cell=cell,
                  pbc=pbc,
                  info=info)

    # Convert any other particle properties to additional arrays
    for name, prop in self.iteritems():
        if name in ['Simulation cell',
                    'Position',
                    'Particle Type']:
            continue
        if not isinstance(prop, ParticleProperty):
            continue
        atoms.new_array(prop.name, prop.array)
    
    return atoms

DataCollection.to_ase_atoms = _DataCollection_to_ase_atoms


def _DataCollection_create_from_ase_atoms(cls, atoms):
    """
    Convert from ase.atoms.Atoms to ovito.data.DataCollection
    """
    data = cls()

    # Set the unit cell and origin (if specified in atoms.info)
    cell = SimulationCell()
    matrix = np.zeros((3,4))
    matrix[:, :3] = atoms.get_cell()
    matrix[:, 3]  = atoms.info.get('cell_origin',
                                   [0., 0., 0.])
    cell.matrix = matrix
    cell.pbc = [bool(p) for p in atoms.get_pbc()]
    data.add(cell)

    # Add ParticleProperty from atomic positions
    num_particles = len(atoms)
    position = ParticleProperty.create(ParticleProperty.Type.Position,
                                       num_particles)
    position.mutable_array[...] = atoms.get_positions()
    data.add(position)

    # Set particle types from chemical symbols
    types = ParticleProperty.create(ParticleProperty.Type.ParticleType,
                                    num_particles)
    symbols = atoms.get_chemical_symbols()
    type_list = list(set(symbols))
    for i, sym in enumerate(type_list):
        types.type_list.append(ParticleType(id=i+1, name=sym))
    types.mutable_array[:] = [ type_list.index(sym)+1 for sym in symbols ]
    data.add(types)

    # Add other properties from atoms.arrays
    for name, array in atoms.arrays.iteritems():
        if name in ['positions', 'numbers']:
            continue
        if array.dtype.kind == 'i':
            typ = 'int'
        elif array.dtype.kind == 'f':
            typ = 'float'
        else:
            continue
        num_particles = array.shape[0]
        num_components = 1
        if len(array.shape) == 2:
            num_components = array.shape[1]
        prop = ParticleProperty.create_user(name,
                                            typ,
                                            num_particles,
                                            num_components)
        prop.mutable_array[...] = array
        data.add(prop)
    
    return data

DataCollection.create_from_ase_atoms = classmethod(_DataCollection_create_from_ase_atoms)

