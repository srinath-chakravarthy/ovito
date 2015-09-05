===================================
Particle properties
===================================

OVITO stores particle properties such as the position, mass, color, etc. in separate data arrays. 
A particle system is therefore nothing else than as a loose collection of :py:class:`~ovito.data.ParticleProperty` instances, 
and the number of particles is implicitly defined by the length of these data arrays (which is the same
for all properties). All defined particle properties are stored in a :py:class:`~ovito.data.DataCollection` instance,
which is a generic container for data objects (:py:class:`~ovito.data.ParticleProperty` is a subclass of
:py:class:`~ovito.data.DataObject`).

A :py:class:`~ovito.data.DataCollection` can hold an arbitrary number of particle properties and other data objects.
At the very least you will find one :py:class:`~ovito.data.ParticleProperty` instance in a data collection, 
namely the ``Position`` property, which is essential to constitute a particle system. 
Furthermore, the number of particles is returned by the :py:attr:`DataCollection.number_of_particles <ovito.data.DataCollection.number_of_particles>` attribute,
which is a shortcut to querying the length of the data :py:attr:`~ovito.data.ParticleProperty.array` of the ``Position`` particle property.

To find out which particle properties are defined, you can query the 
:py:attr:`DataCollection.particle_properties <ovito.data.DataCollection.particle_properties>` dictionary view
for its keys::

    >>> data_collection = node.output
    >>> list(data_collection.particle_properties.keys())
    ['Particle Identifier', 'Particle Type', 'Position', 'Color']

Accordingly, individual particle properties can be accessed through these dictionary keys::

    >>> data_collection.particle_properties['Particle Identifier']
    <ParticleProperty at 0x7fe16c8bc7b0>

In addition to particle properties, a data collection can contain other data objects 
such as a :py:class:`~ovito.data.SimulationCell` or a :py:class:`~ovito.data.Bonds` object.
These are accessible through the dictionary interface of the :py:class:`~ovito.data.DataCollection` itself,
which lists all stored data objects (including the particle properties)::

    >>> list(data_collection.keys())
    ['Simulation cell', 'Bonds', 'Particle Identifier', 'Particle Type', 'Position', 'Color']

    >>> data_collection['Simulation cell']
    <SimulationCell at 0x7fd54ba34c40>

A :py:class:`~ovito.ObjectNode` has two :py:class:`DataCollections <ovito.data.DataCollection>`: one caching
the original input data of the modification pipeline, which was read from the external file, and another one caching 
the output of the pipeline after the modifiers have been applied. For example::

    >>> node.source
    DataCollection(['Simulation cell', 'Position'])
    
    >>> node.compute()
    >>> node.output
    DataCollection(['Simulation cell', 'Position', 'Color', 'Structure Type', 'Bonds'])

Here, some modifiers in the pipeline have added two additional particle properties and created a set of bonds,
which are stored in a :py:class:`~ovito.data.Bonds` data object in the output data collection.

The dictionary interface of the :py:class:`~ovito.data.DataCollection` class allows to access data objects via their
name keys. As a simplification, it is also possible to access standard particle properties, the simulation cell, and bonds,
as object attributes, e.g.::

    >>> node.output.particle_properties.position
    <ParticleProperty at 0x7fe16c8bc7b0>
    
    >>> node.output.particle_properties.structure_type
    <ParticleProperty at 0x7ff46263cff0>
    
    >>> node.output.cell
    <SimulationCell at 0x7fd54ba34c40>

    >>> node.output.bonds
    <Bonds at 0x7ffe88613a60>
    
To access standard particle properties in this way, the Python attribute name can be derived from the
particle property name by replacing all letters with their lower-case variants and white-spaces with underscores (e.g. 
``particle_properties['Structure Type']`` becomes ``particle_properties.structure_type``). The names of all standard particle
properties are listed :ref:`here <standard-property-list>`.

The per-particle data stored in a :py:class:`~ovito.data.ParticleProperty` can be accessed through
its :py:attr:`~ovito.data.ParticleProperty.array` attribute, which returns a NumPy array::

    >>> coordinates = node.output.particle_properties.position.array
    >>> print(coordinates)
    [[ 73.24230194  -5.77583981  -0.87618297]
     [-49.00170135 -35.47610092 -27.92519951]
     [-50.36349869 -39.02569962 -25.61310005]
     ..., 
     [ 42.71210098  59.44919968  38.6432991 ]
     [ 42.9917984   63.53770065  36.33330154]
     [ 44.17670059  61.49860001  37.5401001 ]]
     
    >>> len(coordinates)      # This is equal to the number of particles
    112754
    
.. note::

   The :py:attr:`~ovito.data.ParticleProperty.array` attribute of a particle property allows
   you to directly access the per-particle data as a NumPy array. The array is one-dimensional
   for scalar particle properties and two-dimensional for vectorial properties.
   The data in the array is marked as read-only, because OVITO requires that the data does not change without 
   the program knowing it. If you want to alter the values of a particle property
   directly (e.g. because there is no modifier to achieve the same effect), then have a look
   at the :py:attr:`~ovito.data.ParticleProperty.marray` attribute of the :py:class:`~ovito.data.ParticleProperty` class,
   which provides write access to the internal data.

-----------------------------------
Particle type property
-----------------------------------

Most particle properties are instances of the :py:class:`~ovito.data.ParticleProperty` class. However,
there exist specializations. For instance, the :py:class:`~ovito.data.ParticleTypeProperty` class is a subclass
of :py:class:`~ovito.data.ParticleProperty` and supplements the per-particle type info with a list of 
defined particle types, each having a name, a display color, and a display radius::

    >>> node = import_file('example.poscar')
    
    >>> ptp = node.source.particle_properties.particle_type   # Access the 'Particle Type' property
    >>> ptp
    <ParticleTypeProperty at 0x7fe0a2c355d0>
    
    >>> ptp.array     # This contains the per-particle data, one integer per particle
    [1 1 2 ..., 1 2 1]
    
    >>> for ptype in ptp.type_list:
    ...     print(ptype.id, ptype.name, ptype.color)
    1 Cu (1.0 0.4 0.4)
    2 Zr (0.0 1.0 0.4)

The :py:attr:`~ovito.data.ParticleTypeProperty.type_list` attribute lists the defined
:py:class:`ParticleTypes <ovito.data.ParticleType>`. In the example above we were looping over this 
list to print the numeric ID, human-readable name, and color of each atom type.

-----------------------------------
Bonds and bond properties
-----------------------------------

Bonds are stored in a :py:class:`~ovito.data.Bonds` object, which is basically a data array containing
two integers per bond: The (zero-based) index of the particle the bond originates from and the index of the
particle it is pointing to. In fact, OVITO uses two half-bonds to represent every full bond between two particles; 
one half-bond from particle A to B, and an opposite half-bond
pointing from B to A. The :py:class:`~ovito.data.Bonds` class stores all half-bonds in a big list with arbitrary order, 
which can be accessed through the :py:attr:`~ovito.data.Bonds.array` attribute::

    >>> node.output.bonds.array
    [[   0    1]
     [   1    0]
     [   1    2]
     ..., 
     [2998 2997]
     [2998 2999]
     [2999 2998]]
 
In addition, bonds can have a number of properties, analogous to particle properties. Bond properties
are stored separately as instances of the :py:class:`~ovito.data.BondProperty` class, which can be
accessed via the :py:attr:`~ovito.data.DataCollection.bond_properties` dictionary view of the
:py:class:`~ovito.data.DataCollection`::

    >>> list(node.output.bond_properties.keys())
    ['Bond Type', 'Color']

    >>> btype_prop = node.output.bond_properties.bond_type
    >>> btype_prop
    <BondTypeProperty at 0x7fe16c8bc7b0>

The :py:class:`~ovito.data.BondTypeProperty` class is a specialization of the :py:class:`~ovito.data.BondProperty` 
base class.

The length of a :py:class:`~ovito.data.BondProperty` data array is always equal to the number of half-bonds::

    >>> len(node.output.bonds.array)
    6830
    >>> len(node.output.bond_properties.bond_type.array)
    6830
    >>> node.output.number_of_bonds
    6830
    