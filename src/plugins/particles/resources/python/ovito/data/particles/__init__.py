import re
import numpy
import math
try:
    # Python 3.x
    import collections.abc as collections
except ImportError:
    # Python 2.x
    import collections
import PyQt5.QtCore

# Load dependencies
import ovito
import ovito.data

# Load the native code module
import ovito.plugins.Particles

# Inject selected classes into parent module.
ovito.data.SimulationCell = ovito.plugins.Particles.SimulationCell
ovito.data.ParticleProperty = ovito.plugins.Particles.ParticleProperty
ovito.data.Bonds = ovito.plugins.Particles.Bonds
ovito.data.SurfaceMesh = ovito.plugins.Particles.SurfaceMesh
ovito.data.ParticleTypeProperty = ovito.plugins.Particles.ParticleTypeProperty
ovito.data.ParticleType = ovito.plugins.Particles.ParticleType
ovito.data.BondProperty = ovito.plugins.Particles.BondProperty
ovito.data.BondTypeProperty = ovito.plugins.Particles.BondTypeProperty
ovito.data.BondType = ovito.plugins.Particles.BondType
ovito.data.TrajectoryLineGenerator = ovito.plugins.Particles.TrajectoryLineGenerator
ovito.data.__all__ += ['SimulationCell', 'ParticleProperty', 'Bonds', 'SurfaceMesh', 'ParticleTypeProperty', 'ParticleType',
            'BondProperty', 'BondTypeProperty', 'BondType', 'TrajectoryLineGenerator', 'CutoffNeighborFinder',
            'NearestNeighborFinder']

# For backward-compatibility with OVITO 2.5.1:
def _ParticleProperty_data_attribute_name(self):
    if self.type != ovito.data.ParticleProperty.Type.User:
        return re.sub('\W|^(?=\d)','_', self.name).lower()
    else:
        return None
ovito.data.ParticleProperty._data_attribute_name = property(_ParticleProperty_data_attribute_name)

# Access particle and bond properties by their name (not display title).
def _ParticleProperty_data_key(self):
    return self.name
ovito.data.ParticleProperty._data_key = property(_ParticleProperty_data_key)
ovito.data.BondProperty._data_key = property(_ParticleProperty_data_key)

# Implement the 'particle_properties' attribute of the DataCollection class.
def _DataCollection_particle_properties(self):
    """
    Returns a read-only dictionary view that provides access to the :py:class:`ParticleProperty` 
    objects stored in this :py:class:`!DataCollection`.
    """
    
    # Helper class used to implement the 'particle_properties' property of the DataCollection class.
    class _ParticlePropertyView(collections.Mapping):
        
        def __init__(self, objects):
            self._objects = objects
            
        def __len__(self):
            # Count the number of ParticleProperty objects in the collection.
            return sum(isinstance(obj, ovito.data.ParticleProperty) for obj in self._objects)

        def __getitem__(self, key):
            for obj in self._objects:
                if isinstance(obj, ovito.data.ParticleProperty): 
                    if obj.name == key:
                        return obj
            raise KeyError("The DataCollection does not contain a particle property with the name '%s'." % key)
        
        def __iter__(self):
            for obj in self._objects:
                if isinstance(obj, ovito.data.ParticleProperty):
                    yield obj.name
               
        def __getattr__(self, name):
            for obj in self._objects:
                if isinstance(obj, ovito.data.ParticleProperty): 
                    if obj.type != ovito.data.ParticleProperty.Type.User and re.sub('\W|^(?=\d)','_', obj.name).lower() == name:
                        return obj
            raise AttributeError("DataCollection does not contain the particle property '%s'." % name)
     
        def __repr__(self):
            return repr(dict(self))
     
    return _ParticlePropertyView(self.objects)
ovito.data.DataCollection.particle_properties = property(_DataCollection_particle_properties)

# Implement the 'bond_properties' attribute of the DataCollection class.
def _DataCollection_bond_properties(self):
    """
    Returns a dictionary view that provides access to the :py:class:`BondProperty` 
    instances stored in this :py:class:`!DataCollection`.
    """
    
    # Helper class used to implement the 'bond_properties' property of the DataCollection class.
    class _BondPropertyView(collections.Mapping):
        
        def __init__(self, objects):
            self._objects = objects
            
        def __len__(self):
            # Count the number of BondProperty objects in the collection.
            return sum(isinstance(obj, ovito.data.BondProperty) for obj in self._objects)
        
        def __getitem__(self, key):
            for obj in self._objects:
                if isinstance(obj, ovito.data.BondProperty): 
                    if obj.name == key:
                        return obj
            raise KeyError("The DataCollection contains no bond property with the name '%s'." % key)
        
        def __iter__(self):
            for obj in self._objects:
                if isinstance(obj, ovito.data.BondProperty):
                    yield obj.name
               
        def __getattr__(self, name):
            for obj in self._objects:
                if isinstance(obj, ovito.data.BondProperty): 
                    if obj.type != ovito.data.BondProperty.Type.User and re.sub('\W|^(?=\d)','_', obj.name).lower() == name:
                        return obj
            raise AttributeError("DataCollection does not contain the bond property '%s'." % name)
     
    return _BondPropertyView(self.objects)
ovito.data.DataCollection.bond_properties = property(_DataCollection_bond_properties)

# Implement 'cell' attribute of DataCollection class.
def _DataCollection_cell(self):
    """
    Returns the :py:class:`SimulationCell` stored in this :py:class:`!DataCollection`.
    
    Accessing this property raises an ``AttributeError`` if the data collection
    contains no simulation cell information.
    """
    for obj in self.objects:
        if isinstance(obj, ovito.data.SimulationCell):
            return obj
    raise AttributeError("This DataCollection contains no simulation cell.")
ovito.data.DataCollection.cell = property(_DataCollection_cell)

# Implement 'bonds' attribute of DataCollection class.
def _DataCollection_bonds(self):
    """
    Returns the :py:class:`Bonds` object stored in this :py:class:`!DataCollection`.
    
    Accessing this property raises an ``AttributeError`` if the data collection
    contains no bonds.
    """
    for obj in self.objects:
        if isinstance(obj, ovito.data.Bonds):
            return obj
    raise AttributeError("This DataCollection contains no bonds data object.")
ovito.data.DataCollection.bonds = property(_DataCollection_bonds)

# Implement 'surface' attribute of DataCollection class.
def _DataCollection_surface(self):
    """
    Returns the :py:class:`SurfaceMesh` in this :py:class:`!DataCollection`.
    
    Accessing this property raises an ``AttributeError`` if the data collection
    contains no surface mesh instance.
    """
    for obj in self.objects:
        if isinstance(obj, ovito.data.SurfaceMesh):
            return obj
    raise AttributeError("This DataCollection contains no surface mesh.")
ovito.data.DataCollection.surface = property(_DataCollection_surface)

# Returns a NumPy array wrapper for a particle property.
def _ParticleProperty_array(self):
    """ 
    This attribute returns a NumPy array, which provides read access to the per-particle data stored in this particle property object.
        
    The returned array is one-dimensional for scalar particle properties (:py:attr:`.components` == 1),
    or two-dimensional for vector properties (:py:attr:`.components` > 1). The outer length of the array is 
    equal to the number of particles in both cases.
        
    Note that the returned NumPy array is read-only and provides a view of the internal data. 
    No copy of the data, which may be shared by multiple objects, is made. If you want to modify the 
    data stored in this particle property, use :py:attr:`.marray` instead.
    """
    return numpy.asarray(self)
ovito.data.ParticleProperty.array = property(_ParticleProperty_array)

# Returns a NumPy array wrapper for a particle property with write access.
def _ParticleProperty_marray(self):
    """ 
    This attribute returns a *mutable* NumPy array providing read/write access to the internal per-particle data.
        
    The returned array is one-dimensional for scalar particle properties (:py:attr:`.components` == 1),
    or two-dimensional for vector properties (:py:attr:`.components` > 1). The outer length of the array is 
    equal to the number of particles in both cases.
        
    .. note::
           
       After you are done modifying the data in the returned NumPy array, you must call
       :py:meth:`.changed`! Calling this method is necessary to inform the data pipeline system
       that the input particle data has changed and the modification pipeline needs to be re-evaluated.
       The reason is that OVITO cannot automatically detect modifications made by the script to 
       the returned NumPy array. Therefore, an explicit call to :py:meth:`.changed` is necessary. 
       
    **Example**
    
    .. literalinclude:: ../example_snippets/mutable_array.py
    
    """
    class DummyClass:
        pass
    o = DummyClass()
    o.__array_interface__ = self.__mutable_array_interface__
    # Create reference to particle property object to keep it alive.
    o.__base_property = self        
    return numpy.asarray(o)

# This is needed to enable the augmented assignment operators (+=, -=, etc.) for the 'marray' property.
def _ParticleProperty_marray_assign(self, other):
    if not hasattr(other, "__array_interface__"):
        raise ValueError("Only objects supporting the array interface can be assigned to the 'marray' property.")
    o = other.__array_interface__
    s = self.__mutable_array_interface__
    if o["shape"] != s["shape"] or o["typestr"] != s["typestr"] or o["data"] != s["data"]:
        raise ValueError("Assignment to the 'marray' property is restricted. Left and right-hand side must be identical.")
    # Assume that the data has been changed in the meantime.
    self.changed()
        
ovito.data.ParticleProperty.marray = property(_ParticleProperty_marray, _ParticleProperty_marray_assign)
# For backward compatibility with OVITO 2.5.1:
ovito.data.ParticleProperty.mutable_array = property(lambda self: self.marray)

# Returns a NumPy array wrapper for a bond property.
def _BondProperty_array(self):
    """ 
    This attribute returns a NumPy array, which provides read access to the per-bond data stored in this bond property object.
        
    The returned array is one-dimensional for scalar bond properties (:py:attr:`.components` == 1),
    or two-dimensional for vector properties (:py:attr:`.components` > 1). The outer length of the array is 
    equal to the number of half-bonds in both cases.
        
    Note that the returned NumPy array is read-only and provides a view of the internal data. 
    No copy of the data, which may be shared by multiple objects, is made. If you want to modify the 
    data stored in this bond property, use :py:attr:`.marray` instead.
    """
    return numpy.asarray(self)
ovito.data.BondProperty.array = property(_BondProperty_array)

# Returns a NumPy array wrapper for a bond property with write access.
def _BondProperty_marray(self):
    """ 
    This attribute returns a *mutable* NumPy array providing read/write access to the internal per-bond data.
        
    The returned array is one-dimensional for scalar bond properties (:py:attr:`.components` == 1),
    or two-dimensional for vector properties (:py:attr:`.components` > 1). The outer length of the array is 
    equal to the number of half-bonds in both cases.
        
    .. note::
           
       After you are done modifying the data in the returned NumPy array, you must call
       :py:meth:`.changed`! Calling this method is necessary to inform the data pipeline system
       that the input bond data has changed and the modification pipeline needs to be re-evaluated.
       The reason is that OVITO cannot automatically detect modifications made by the script to 
       the returned NumPy array. Therefore, an explicit call to :py:meth:`.changed` is necessary. 
           
    """
    class DummyClass:
        pass
    o = DummyClass()
    o.__array_interface__ = self.__mutable_array_interface__
    # Create reference to particle property object to keep it alive.
    o.__base_property = self
    return numpy.asarray(o)
        
ovito.data.BondProperty.marray = property(_BondProperty_marray, _ParticleProperty_marray_assign)

# Returns a NumPy array wrapper for bonds list.
def _Bonds_array(self):
    """ This attribute returns a NumPy array providing direct access to the bonds list.
        
        The returned array is two-dimensional and contains pairs of particle indices connected by a bond.
        The array's shape is *N x 2*, where *N* is the number of half-bonds. Each pair-wise bond occurs twice
        in the array, once for the connection A->B and second time for the connection B->A.
        Particle indices start at 0.
        
        Note that the returned NumPy array is read-only and provides a view of the internal data. 
    """
    return numpy.asarray(self)
ovito.data.Bonds.array = property(_Bonds_array)

# Returns a NumPy array wrapper for the bond PBC shift vectors.
def _Bonds_pbc_vectors(self):
    """ A NumPy array providing read access to the PBC shift vectors of bonds.
        
        The returned array's shape is *N x 3*, where *N* is the number of half-bonds. It contains the
        periodic shift vector for each half-bond.
        
        A PBC shift vector consists of three integers, which specify how many times (and in which direction)
        the corresonding half-bond crosses the periodic boundaries of the simulation cell. For example, a shift vector (0,-1,0)
        indicates that the half-bond crosses the periodic boundary in the negative Y direction 
        once. In other words, the particle where the half-bond originates from is located
        close to the lower edge of the simulation cell in the Y direction while the second particle is located 
        close to the opposite side of the box.
        
        The PBC shift vectors are important for visualizing the bonds between particles with wrapped coordinates, 
        which are located on opposite sides of a periodic cell. When the PBC shift vector of a bond is (0,0,0), OVITO assumes that 
        both particles connected by the bond are located in the same periodic image and the bond is rendered such that
        it directly connects the two particles without going through a cell boundary.
        
        Note that, if the PBC shift vector of a half-bond A->B is (n\ :sub:`x`, n\ :sub:`y`, n\ :sub:`z`), then
        the shift vector of the reverse half-bond B->A is always (-n\ :sub:`x`, -n\ :sub:`y`, -n\ :sub:`z`).
    """
    class DummyClass:
        pass
    o = DummyClass()
    o.__array_interface__ = self._pbc_vectors
    # Create reference to particle property object to keep it alive.
    o.__base_property = self
    return numpy.asarray(o)
ovito.data.Bonds.pbc_vectors = property(_Bonds_pbc_vectors)

def _Bonds_add(self, p1, p2, pbc_shift = (0,0,0)):
    """ Creates a new half-bond from particle *p1* to particle *p2*. 
    
        To also create a reverse half-bond from *p2* to *p1*, use :py:meth:`.add_full` instead.

        :param int p1: Zero-based index of the particle at which the bonds originates.
        :param int p2: Zero-based index of the particle the bonds leads to.
        :param pbc_shift: A tuple of three integers, which specifies how often each periodic 
                          boundary of the simulation cell is crossed when following the new bond from *p1* to *p2*. 
                          This information is needed by OVITO to correctly wrap bonds at periodic boundaries. 
                          See :py:attr:`.pbc_vectors` array.
    """
    self.addBond(p1, p2, pbc_shift)
ovito.data.Bonds.add = _Bonds_add

def _Bonds_add_full(self, p1, p2, pbc_shift = (0,0,0)):
    """ Creates two half-bonds between the particles *p1* and *p2*. This is equivalent to 
        two calls to :py:meth:`.add`.
    
        :param int p1: Zero-based index of the first particle.
        :param int p2: Zero-based index of the second particle.
        :param pbc_shift: A tuple of three integers, which specifies how often each periodic 
                          boundary of the simulation cell is crossed when following the new bond from *p1* to *p2*. 
                          This information is needed by OVITO to correctly wrap bonds at periodic boundaries. 
                          See :py:attr:`.pbc_vectors` array.
    """
    self.addBond(p1, p2, pbc_shift)
    self.addBond(p2, p1, (-pbc_shift[0], -pbc_shift[1], -pbc_shift[2]))
ovito.data.Bonds.add_full = _Bonds_add_full

# Implement 'pbc' property of SimulationCell class.
def _get_SimulationCell_pbc(self):
    """ A tuple with three boolean values, which specify periodic boundary flags of the simulation cell along each cell vector. """
    return (self.pbc_x, self.pbc_y, self.pbc_z)
def _set_SimulationCell_pbc(self, flags):
    assert(len(flags) == 3) # Expected tuple with three Boolean flags.
    self.pbc_x = flags[0]
    self.pbc_y = flags[1]
    self.pbc_z = flags[2]
ovito.data.SimulationCell.pbc = property(_get_SimulationCell_pbc, _set_SimulationCell_pbc)

class CutoffNeighborFinder(ovito.plugins.Particles.CutoffNeighborFinder):
    """ 
    A utility class that computes particle neighbor lists.
    
    This class allows to iterate over the neighbors of each particle within a given cutoff distance.
    You can use it to build neighbors lists or perform other kinds of analyses that require neighbor information.
    
    The constructor takes a positive cutoff radius and a :py:class:`DataCollection <ovito.data.DataCollection>` 
    containing the input particle positions and the cell geometry (including periodic boundary flags).
    
    Once the :py:class:`!CutoffNeighborFinder` has been constructed, you can call its :py:meth:`.find` method to 
    iterate over the neighbors of a specific particle, for example:
    
    .. literalinclude:: ../example_snippets/cutoff_neighbor_finder.py
    
    If you want to determine the *N* nearest neighbors of a particle,
    use the :py:class:`NearestNeighborFinder` class instead.    
    """
        
    def __init__(self, cutoff, data_collection):
        """ This is the constructor. """
        super(self.__class__, self).__init__()
        if not hasattr(data_collection, 'position'):
            raise KeyError("DataCollection does not contain any ")
        if not hasattr(data_collection, 'cell'):
            raise KeyError("DataCollection does not contain simulation cell information.")
        self.particle_count = data_collection.number_of_particles
        if not self.prepare(cutoff, data_collection.position, data_collection.cell):
            raise RuntimeError("Operation has been canceled by the user.")
        
    def find(self, index):
        """ 
        Returns an iterator over all neighbors of the given particle.
         
        :param int index: The index of the central particle whose neighbors should be iterated. Particle indices start at 0.
        :returns: A Python iterator that visits all neighbors of the central particle within the cutoff distance. 
                  For each neighbor the iterator returns an object with the following attributes:
                  
                      * **index**: The index of the current neighbor particle (starting at 0).
                      * **distance**: The distance of the current neighbor from the central particle.
                      * **distance_squared**: The squared neighbor distance.
                      * **delta**: The three-dimensional vector connecting the central particle with the current neighbor (taking into account periodicity).
                      * **pbc_shift**: The periodic shift vector, which specifies how often each periodic boundary of the simulation cell is crossed when going from the central particle to the current neighbor.
        
        Note that all periodic images of particles within the cutoff radius are visited. Thus, the same particle index may appear multiple times in the neighbor
        list of a central particle. In fact, the central particle may be among its own neighbors in a sufficiently small periodic simulation cell.
        However, the computed vector (``delta``) and PBC shift (``pbc_shift``) taken together will be unique for each visited image of a neighboring particle.
        """
        if index < 0 or index >= self.particle_count:
            raise IndexError("Particle index is out of range.")
        # Construct the C++ neighbor query. 
        query = ovito.plugins.Particles.CutoffNeighborFinder.Query(self, int(index))
        # Iterate over neighbors.
        while not query.at_end:
            yield query
            query.next()
            
ovito.data.CutoffNeighborFinder = CutoffNeighborFinder

class NearestNeighborFinder(ovito.plugins.Particles.NearestNeighborFinder):
    """ 
    A utility class that finds the *N* nearest neighbors of a particle or a spatial location.
    
    The constructor takes the (maximum) number of requested nearest neighbors, *N*, and a :py:class:`DataCollection <ovito.data.DataCollection>` 
    containing the input particle positions and the cell geometry (including periodic boundary flags).
    *N* must be a positive integer not greater than 30 (which is the built-in maximum supported by this class).
    
    Once the :py:class:`!NearestNeighborFinder` has been constructed, you can call its :py:meth:`.find` method to 
    iterate over the sorted list of nearest neighbors of a specific particle, for example:
    
    .. literalinclude:: ../example_snippets/nearest_neighbor_finder.py
       :lines: 1-18
    
    Furthermore, the class provides the :py:meth:`find_at` method, which allows to query the nearest neighbors at an 
    arbitrary spatial position (doesn't have to be a physical particle):

    .. literalinclude:: ../example_snippets/nearest_neighbor_finder.py    
       :lines: 19-

    If you want to find all neighbor particles within a certain cutoff radius of a central particle,
    use the :py:class:`CutoffNeighborFinder` class instead.
    """
        
    def __init__(self, N, data_collection):
        """ This is the constructor. """
        super(self.__class__, self).__init__(N)
        if N<=0 or N>30:
            raise ValueError("The requested number of nearest neighbors is out of range.")
        if not hasattr(data_collection, 'position'):
            raise KeyError("DataCollection does not contain any particles.")
        if not hasattr(data_collection, 'cell'):
            raise KeyError("DataCollection does not contain simulation cell information.")
        self.particle_count = data_collection.number_of_particles
        if not self.prepare(data_collection.position, data_collection.cell):
            raise RuntimeError("Operation has been canceled by the user.")
        
    def find(self, index):
        """ 
        Returns an iterator that visits the *N* nearest neighbors of the given particle in order of ascending distance.
         
        :param int index: The index of the central particle whose neighbors should be iterated. Particle indices start at 0.
        :returns: A Python iterator that visits the *N* nearest neighbors of the central particle in order of ascending distance. 
                  For each visited neighbor the iterator returns an object with the following attributes:
                  
                      * **index**: The index of the current neighbor particle (starting at 0).
                      * **distance**: The distance of the current neighbor from the central particle.
                      * **distance_squared**: The squared neighbor distance.
                      * **delta**: The three-dimensional vector connecting the central particle with the current neighbor (taking into account periodicity).
        
        Note that several periodic images of the same particle may be visited. Thus, the same particle index may appear multiple times in the neighbor
        list of a central particle. In fact, the central particle may be among its own neighbors in a sufficiently small periodic simulation cell.
        However, the computed neighbor vector (``delta``) will be unique for each visited image of a neighboring particle.
        
        The number of neighbors actually visited may be smaller than the requested number, *N*, if the
        system contains too few particles and has no periodic boundary conditions.
        """
        if index < 0 or index >= self.particle_count:
            raise IndexError("Particle index is out of range.")
        # Construct the C++ neighbor query. 
        query = ovito.plugins.Particles.NearestNeighborFinder.Query(self)
        query.findNeighbors(int(index))
        # Iterate over neighbors.
        for i in range(query.count):
            yield query[i]

    def find_at(self, coords):
        """ 
        Returns an iterator that visits the *N* nearest particles around a spatial point given by *coords* in order of ascending distance.
        Unlike the :py:meth:`find` method, which queries the nearest neighbors of a physical particle, the :py:meth:`!find_at` method allows  
        searching for neareby particles at arbitrary locations in space. 
         
        :param coords: A (x,y,z) coordinate triple specifying the spatial location where the *N* nearest particles should be queried.
        :returns: A Python iterator that visits the *N* nearest neighbors in order of ascending distance. 
                  For each visited particle the iterator returns an object with the following attributes:
                  
                      * **index**: The index of the current particle (starting at 0).
                      * **distance**: The distance of the current neighbor from the query location.
                      * **distance_squared**: The squared distance to the query location.
                      * **delta**: The three-dimensional vector from the query point to the current particle (taking into account periodicity).

        If there exists a particle that is exactly located at the query position given by *coords*, then it will be returned by this function.
        This is in contrast to the :py:meth:`find` function, which does not visit the central particle itself.
        
        The number of neighbors actually visited may be smaller than the requested number, *N*, if the
        system contains too few particles and has no periodic boundary conditions.
        """
        # Construct the C++ neighbor query. 
        query = ovito.plugins.Particles.NearestNeighborFinder.Query(self)
        query.findNeighborsAtLocation(coords, True)
        # Iterate over neighbors.
        for i in range(query.count):
            yield query[i]
            
ovito.data.NearestNeighborFinder = NearestNeighborFinder

def _ParticleProperty_create(prop_type, num_particles):
    """
        Static factory function that creates a new :py:class:`!ParticleProperty` instance for a standard particle property.
        To create a new user-defined property, use :py:meth:`.create_user` instead. 
        
        Note that this factory function is a low-level method. If you want to add a new 
        particle property to an existing :py:class:`~ovito.data.DataCollection`, you can do so using the high-level method  
        :py:meth:`~ovito.data.DataCollection.create_particle_property` instead.
        
        :param ParticleProperty.Type prop_type: The standard particle property to create. See the :py:attr:`.type` attribute for a list of possible values.
        :param int num_particles: The number of particles. This determines the size of the allocated data array.
        :returns: A newly created instance of the :py:class:`!ParticleProperty` class or one of its sub-classes. 
        
    """
    assert(isinstance(prop_type, ovito.data.ParticleProperty.Type))
    assert(prop_type != ovito.data.ParticleProperty.Type.User)
    assert(num_particles >= 0)
    
    return ovito.data.ParticleProperty.createStandardProperty(ovito.dataset, num_particles, prop_type, 0, True)
ovito.data.ParticleProperty.create = staticmethod(_ParticleProperty_create)

def _ParticleProperty_create_user(name, data_type, num_particles, num_components = 1):
    """
        Static factory function that creates a new :py:class:`!ParticleProperty` instance for a user-defined particle property.
        To create one of the standard properties, use :py:meth:`.create` instead. 
        
        Note that this factory function is a low-level method. If you want to add a new user-defined 
        particle property to an existing :py:class:`~ovito.data.DataCollection`, you can do so using the high-level method 
        :py:meth:`~ovito.data.DataCollection.create_user_particle_property` instead.

        :param str name: The name of the user-defined particle property to create.
        :param str data_type: Must be either ``"int"`` or ``"float"``.                
        :param int num_particles: The number of particles. This determines the size of the allocated data array.
        :param int num_components: The number of components when creating a vector property.
        :returns: A newly created instance of the :py:class:`!ParticleProperty` class. 
        
    """
    assert(num_particles >= 0)
    assert(num_components >= 1)
    if data_type == "int":
        data_type = PyQt5.QtCore.QMetaType.type("int")
    elif data_type == "float":
        data_type = PyQt5.QtCore.QMetaType.type("FloatType")
    else:
        raise RuntimeError("Invalid data type. Only 'int' or 'float' are allowed.")
        
    return ovito.data.ParticleProperty.createUserProperty(ovito.dataset, num_particles, data_type, num_components, 0, name, True)
ovito.data.ParticleProperty.create_user = staticmethod(_ParticleProperty_create_user)

# Extend DataCollection class by adding the 'create_particle_property()' and 'create_user_particle_property()' methods.
def _DataCollection_create_particle_property(self, property_type, data = None):
    """ 
        Adds a standard particle property to this data collection.
        
        If the specified particle property already exists in this data collection, the existing property instance is returned.
        Otherwise the method creates a new property instance using :py:meth:`ParticleProperty.create` and adds it to this data collection.
        
        The optional parameter *data* allows to directly set or initialize the values of the particle property.
        
        :param ParticleProperty.Type property_type: The standard particle property to create. See the :py:attr:`ParticleProperty.type` attribute for a list of possible values.
        :param data: An optional data array (e.g. NumPy array), which contains the per-particle values used to initialize the particle property.
                     The size of the array must match the number of particles in this data collection (see :py:attr:`.number_of_particles` attribute).                      
        :returns: A newly created instance of the :py:class:`ovito.data.ParticleProperty` class or one of its sub-classes if the property did not exist yet in the data collection.
                  Otherwise, the existing particle property object is returned.
        
    """
    # Check if property already exists in the data collection.
    prop = None
    position_prop = None
    for obj in self.values():
        if isinstance(obj, ovito.data.ParticleProperty):
            if obj.type == property_type:
                prop = obj
            if obj.type == ovito.data.ParticleProperty.Type.Position:
                position_prop = obj

    # First we have to determine the number of particles. This requires the 'Position' particle property
    if position_prop is None:
        raise RuntimeError("Cannot add new particle property to data collection, because data collection contains no particles.")
    num_particles = position_prop.size
                
    if prop is None:
        # If property does not exists yet, create a new ParticleProperty instance.
        prop = ovito.data.ParticleProperty.create(property_type, num_particles)
        self.add(prop)
    else:
        # Otherwise, make sure the existing property is a fresh copy so we can safely modify it.
        prop = self.copy_if_needed(prop)
        
    # Initialize property with per-particle data if provided.
    if data is not None:
        prop.marray[:] = data
        prop.changed()
    
    return prop
ovito.data.DataCollection.create_particle_property = _DataCollection_create_particle_property

def _DataCollection_create_user_particle_property(self, name, data_type, num_components=1, data = None):
    """ 
        Adds a user-defined particle property to this data collection.
        
        If a particle property with the given name already exists in this data collection, the existing property instance is returned.
        Otherwise the method creates a new property instance using :py:meth:`ParticleProperty.create_user` and adds it to this data collection.
        
        The optional parameter *data* allows to directly set or initialize the values of the particle property.
        
        :param str name: The name of the user-defined particle property to create.
        :param str data_type: Must be either ``"int"`` or ``"float"``.                
        :param int num_components: The number of components when creating a vector property.
        :param data: An optional data array (e.g. NumPy array), which contains the per-particle values used to initialize the particle property.
                     The size of the array must match the number of particles in this data collection (see :py:attr:`.number_of_particles` attribute).                      
        :returns: A newly created instance of the :py:class:`~ovito.data.ParticleProperty` class or one of its sub-classes if the property did not exist yet in the data collection.
                  Otherwise, the existing particle property object is returned.
        
    """
    # Check if property already exists in the data collection.
    prop = None
    position_prop = None
    for obj in self.values():
        if isinstance(obj, ovito.data.ParticleProperty):
            if obj.name == name:
                prop = obj
            if obj.type == ovito.data.ParticleProperty.Type.Position:
                position_prop = obj

    # First we have to determine the number of particles. This requires the 'Position' particle property
    if position_prop is None:
        raise RuntimeError("Cannot add new particle property to data collection, because data collection contains no particles.")
    num_particles = position_prop.size
                
    if prop is None:
        # If property does not exists yet, create a new ParticleProperty instance.
        prop = ovito.data.ParticleProperty.create_user(name, data_type, num_particles, num_components)
        self.add(prop)
    else:
        # Otherwise, make sure the existing property is a fresh copy so we can safely modify it.
        prop = self.copy_if_needed(prop)
        
    # Initialize property with per-particle data if provided.
    if data is not None:
        prop.marray[:] = data
        prop.changed()
    
    return prop
ovito.data.DataCollection.create_user_particle_property = _DataCollection_create_user_particle_property

def _BondProperty_create(prop_type, num_bonds):
    """
        Static factory function that creates a new :py:class:`!BondProperty` instance for a standard bond property.
        To create a new user-defined property, use :py:meth:`.create_user` instead. 
        
        Note that this factory function is a low-level method. If you want to add a new 
        bond property to an existing :py:class:`~ovito.data.DataCollection`, you can do so using the high-level method  
        :py:meth:`~ovito.data.DataCollection.create_bond_property` instead.
        
        :param BondProperty.Type prop_type: The standard bond property to create. See the :py:attr:`.type` attribute for a list of possible values.
        :param int num_bonds: The number of half-bonds. This determines the size of the allocated data array.
        :returns: A newly created instance of the :py:class:`!BondProperty` class or one of its sub-classes. 
        
    """
    assert(isinstance(prop_type, ovito.data.BondProperty.Type))
    assert(prop_type != ovito.data.BondProperty.Type.User)
    assert(num_bonds >= 0)
    
    return ovito.data.BondProperty.createStandardProperty(ovito.dataset, num_bonds, prop_type, 0, True)
ovito.data.BondProperty.create = staticmethod(_BondProperty_create)

def _BondProperty_create_user(name, data_type, num_bonds, num_components = 1):
    """
        Static factory function that creates a new :py:class:`!BondProperty` instance for a user-defined bond property.
        To create one of the standard properties, use :py:meth:`.create` instead. 
        
        Note that this factory function is a low-level method. If you want to add a new user-defined 
        bond property to an existing :py:class:`~ovito.data.DataCollection`, you can do so using the high-level method 
        :py:meth:`~ovito.data.DataCollection.create_user_bond_property` instead.

        :param str name: The name of the user-defined bond property to create.
        :param str data_type: Must be either ``"int"`` or ``"float"``.                
        :param int num_bonds: The number of half-bonds. This determines the size of the allocated data array.
        :param int num_components: The number of components when creating a vector property.
        :returns: A newly created instance of the :py:class:`!BondProperty` class. 
        
    """
    assert(num_bonds >= 0)
    assert(num_components >= 1)
    if data_type == "int":
        data_type = PyQt5.QtCore.QMetaType.type("int")
    elif data_type == "float":
        data_type = PyQt5.QtCore.QMetaType.type("FloatType")
    else:
        raise RuntimeError("Invalid data type. Only 'int' or 'float' are allowed.")
        
    return ovito.data.BondProperty.createUserProperty(ovito.dataset, num_bonds, data_type, num_components, 0, name, True)
ovito.data.BondProperty.create_user = staticmethod(_BondProperty_create_user)

# Extend DataCollection class by adding the 'create_bond_property()' and 'create_user_bond_property()' methods.
def _DataCollection_create_bond_property(self, property_type, data = None):
    """ 
        Adds a standard bond property to this data collection.
        
        If the specified particle property already exists in this data collection, the existing property instance is returned.
        Otherwise the method creates a new property instance using :py:meth:`BondProperty.create` and adds it to this data collection.
        
        The optional parameter *data* allows to directly set or initialize the values of the bond property.
        
        :param BondProperty.Type property_type: The standard bond property to create. See the :py:attr:`BondProperty.type` attribute for a list of possible values.
        :param data: An optional data array (e.g. NumPy array), which contains the per-bond values used to initialize the property.
                     The size of the array must match the number of half-bonds in this data collection (see :py:attr:`.number_of_half_bonds` attribute).                      
        :returns: A newly created instance of the :py:class:`ovito.data.BondProperty` class or one of its sub-classes if the property did not exist yet in the data collection.
                  Otherwise, the existing bond property object is returned.
        
    """
    # Check if property already exists in the data collection.
    prop = None
    for obj in self.objects:
        if isinstance(obj, ovito.data.BondProperty):
            if obj.type == property_type:
                prop = obj

    bonds_obj = None
    for obj in self.objects:
        if isinstance(obj, ovito.data.Bonds):
            bonds_obj = obj
        
    # First we have to determine the number of bonds.
    if bonds_obj is None:
        raise RuntimeError("Cannot add new bond property to data collection, because data collection contains no bonds.")
    num_bonds = bonds_obj.size
                
    if prop is None:
        # If property does not exists yet, create a new BondProperty instance.
        prop = ovito.data.BondProperty.create(property_type, num_bonds)
        self.add(prop)
    else:
        # Otherwise, make sure the existing property is a fresh copy so we can safely modify it.
        prop = self.copy_if_needed(prop)
        
    # Initialize property with per-bond data if provided.
    if data is not None:
        prop.marray[:] = data
        prop.changed()
    
    return prop
ovito.data.DataCollection.create_bond_property = _DataCollection_create_bond_property

def _DataCollection_create_user_bond_property(self, name, data_type, num_components=1, data = None):
    """ 
        Adds a custom bond property to this data collection.
        
        If a bond property with the given name already exists in this data collection, the existing property instance is returned.
        Otherwise the method creates a new property instance using :py:meth:`BondProperty.create_user` and adds it to this data collection.
        
        The optional parameter *data* allows to directly set or initialize the values of the bond property.
        
        :param str name: The name of the user-defined bond property to create.
        :param str data_type: Must be either ``"int"`` or ``"float"``.                
        :param int num_components: The number of components when creating a vector property.
        :param data: An optional data array (e.g. NumPy array), which contains the per-bond values used to initialize the bond property.
                     The size of the array must match the number of half-bonds in this data collection (see :py:attr:`.number_of_half_bonds` attribute).                      
        :returns: A newly created instance of the :py:class:`~ovito.data.BondProperty` class or one of its sub-classes if the property did not exist yet in the data collection.
                  Otherwise, the existing bond property object is returned.
        
    """
    # Check if property already exists in the data collection.
    prop = None
    for obj in self.values():
        if isinstance(obj, ovito.data.BondProperty):
            if obj.name == name:
                prop = obj

    bonds_obj = None
    for obj in self.objects:
        if isinstance(obj, ovito.data.Bonds):
            bonds_obj = obj
        
    # First we have to determine the number of bonds.
    if bonds_obj is None:
        raise RuntimeError("Cannot add new bond property to data collection, because data collection contains no bonds.")
    num_bonds = bonds_obj.size
                
    if prop is None:
        # If property does not exists yet, create a new BondProperty instance.
        prop = ovito.data.BondProperty.create_user(name, data_type, num_bonds, num_components)
        self.add(prop)
    else:
        # Otherwise, make sure the existing property is a fresh copy so we can safely modify it.
        prop = self.copy_if_needed(prop)
        
    # Initialize property with per-particle data if provided.
    if data is not None:
        prop.marray[:] = data
        prop.changed()
    
    return prop
ovito.data.DataCollection.create_user_bond_property = _DataCollection_create_user_bond_property

# Extend the DataCollection class with a 'number_of_particles' property.
def _get_DataCollection_number_of_particles(self):
    """ The number of particles stored in the data collection. """
    # The number of particles is determined by the size of the 'Position' particle property.
    for obj in self.objects:
        if isinstance(obj, ovito.data.ParticleProperty) and obj.type == ovito.data.ParticleProperty.Type.Position:
            return obj.size
    return 0
ovito.data.DataCollection.number_of_particles = property(_get_DataCollection_number_of_particles)

# Extend the DataCollection class with a 'number_of_half_bonds' property.
def _get_DataCollection_number_of_half_bonds(self):
    """ 
        The number of half-bonds stored in the data collection. 
    
        Note that OVITO internally represents each full bond as two half-bonds, A->B and B->A.
        See also :py:attr:`.number_of_full_bonds`.
    """
    # The number of bonds is determined by the size of the BondsObject.
    try:
        return self.bonds.size
    except:
        return 0
ovito.data.DataCollection.number_of_half_bonds = property(_get_DataCollection_number_of_half_bonds)

# Extend the DataCollection class with a 'number_of_bonds' property.
def _get_DataCollection_number_of_bonds(self):
    return self.number_of_half_bonds
ovito.data.DataCollection.number_of_bonds = property(_get_DataCollection_number_of_bonds)

# Extend the DataCollection class with a 'number_of_full_bonds' property.
def _get_DataCollection_number_of_full_bonds(self):
    """ 
        The number of (full) bonds stored in the data collection.
    
        Note that OVITO internally represents each full bond as two half-bonds, A->B and B->A.
        See also :py:attr:`.number_of_half_bonds`.
    """
    # The number of bonds is determined by the size of the BondsObject.
    try:
        return self.bonds.size//2
    except:
        return 0
ovito.data.DataCollection.number_of_full_bonds = property(_get_DataCollection_number_of_full_bonds)

def _ParticleTypeProperty_get_type_by_id(self, id):
    """
    Returns the :py:class:`ParticleType` with the given numeric ID from the :py:attr:`.type_list`. 
    Raises a ``KeyError`` if the ID does not exist.
    """
    t = self._get_type_by_id(int(id))
    if t is None:
        raise KeyError("Particle type with ID %i is not defined." % id)
    return t
ovito.data.ParticleTypeProperty.get_type_by_id = _ParticleTypeProperty_get_type_by_id

def _ParticleTypeProperty_get_type_by_name(self, name):
    """
    Returns the :py:class:`ParticleType` with the given name from the :py:attr:`.type_list`.
    If multiple types exists with the same name, the first type is returned. 
    Raises a ``KeyError`` if there is no type with such a name.
    """
    t = self._get_type_by_name(name)
    if t is None:
        raise KeyError("Particle type with name '%s' is not defined." % name)
    return t
ovito.data.ParticleTypeProperty.get_type_by_name = _ParticleTypeProperty_get_type_by_name

def _BondTypeProperty_get_type_by_id(self, id):
    """
    Returns the :py:class:`BondType` with the given numeric ID from the :py:attr:`.type_list`. 
    Raises a ``KeyError`` if the ID does not exist.
    """
    t = self._get_type_by_id(int(id))
    if t is None:
        raise KeyError("Bond type with ID %i is not defined." % id)
    return t
ovito.data.BondTypeProperty.get_type_by_id = _BondTypeProperty_get_type_by_id

def _BondTypeProperty_get_type_by_name(self, name):
    """
    Returns the :py:class:`BondType` with the given name from the :py:attr:`.type_list`.
    If multiple types exists with the same name, the first type is returned. 
    Raises a ``KeyError`` if there is no type with such a name.
    """
    t = self._get_type_by_name(name)
    if t is None:
        raise KeyError("Bond type with name '%s' is not defined." % name)
    return t
ovito.data.BondTypeProperty.get_type_by_name = _BondTypeProperty_get_type_by_name

class Enumerator(ovito.plugins.Particles.Bonds.ParticleBondMap):
    """
    Utility class that allows efficiently iterating over the bonds of a particle.

    The class constructor takes the :py:class:`Bonds` object containing all the bonds 
    in the system, for which it will first build a lookup table.
    After the :py:class:`!Enumerator` has been constructed, the half-bonds of a
    particular atom can be visited using the :py:meth:`.bonds_of_particle` method.

    Warning: Do not modify the underlying :py:class:`Bonds` object while using the :py:class:`!Enumerator`.
    Adding or deleting bonds would render the internal lookup table of the :py:class:`!Enumerator`
    invalid.
    
    **Usage example**
    
    .. literalinclude:: ../example_snippets/bonds_enumerator.py
    """ 
                   
    def __init__(self, bonds):
        """ This is the constructor. """
        super(self.__class__, self).__init__(bonds)        
    
    def bonds_of_particle(self, particle_index):
        """
        Returns an iterator that yields the indices of the half-bonds connected to the given particle.
        The numbers returned by the iterator can be used to index the :py:attr:`Bonds.array`.
        """
        eol = self.endOfListValue
        currentBondIndex = self.firstBondOfParticle(particle_index)
        while currentBondIndex != eol:
            yield currentBondIndex
            currentBondIndex = self.nextBondOfParticle(currentBondIndex)
ovito.data.Bonds.Enumerator = Enumerator
del Enumerator
