==================================
Examples
==================================

This page provides a collection of example scripts:

   * :ref:`example_compute_voronoi_indices`
   * :ref:`example_compute_cna_bond_indices`
   * :ref:`example_creating_particles_programmatically`
   
   

.. _example_compute_voronoi_indices:

----------------------------------
Computing Voronoi indices
----------------------------------

This script demonstrates the use of the Voronoi analysis modifier.
The script calculates the distribution of Voronoi coordination polyhedra in an amorphous structure.

A Voronoi polyhedron is expressed in terms of the Schlaefli notation,
which is a vector of indices (n\ :sub:`1`, n\ :sub:`2`, n\ :sub:`3`, n\ :sub:`4`, n\ :sub:`5`, n\ :sub:`6`, ...),
where n\ :sub:`i` is the number of polyhedron faces with *i* edges/vertices.

The script computes the distribution of these Voronoi index vectors
and lists the 10 most frequent polyhedron types in the dataset. In the case
of a Cu\ :sub:`64%`-Zr\ :sub:`36%` bulk metallic glass, the most frequent polyhedron type is the icosahedron.
It has 12 faces with five edges each. Thus, the corresponding Voronoi index 
vector is:

   (0, 0, 0, 0, 12, 0, ...)
   
Python script:
   
.. literalinclude:: ../../../examples/scripts/voronoi_analysis.py
   :lines: 19-

Program output:

.. code-block:: none

  (0, 0, 0, 0, 12, 0)     12274   (11.4 %)
  (0, 0, 0, 2, 8, 2)      7485    (6.9 %)
  (0, 0, 0, 3, 6, 4)      5637    (5.2 %)
  (0, 0, 0, 1, 10, 2)     4857    (4.5 %)
  (0, 0, 0, 3, 6, 3)      3415    (3.2 %)
  (0, 0, 0, 2, 8, 1)      2927    (2.7 %)
  (0, 0, 0, 1, 10, 5)     2900    (2.7 %)
  (0, 0, 0, 1, 10, 4)     2068    (1.9 %)
  (0, 0, 0, 2, 8, 6)      2063    (1.9 %)
  (0, 0, 0, 2, 8, 5)      1662    (1.5 %)
  

.. _example_compute_cna_bond_indices:

------------------------------------
Computing CNA bond indices
------------------------------------

The following script demonstrates how to use the :py:class:`~ovito.modifiers.CreateBondsModifier`
to create bonds between particles. The structural environment of each created bond
is then characterized with the help of the :py:class:`~ovito.modifiers.CommonNeighborAnalysisModifier`,
which computes a triplet of indices for each bond from the topology of the surrounding bond network. 
The script accesses the computed CNA bond indices in the output :py:class:`~ovito.data.DataCollection` of the 
modification pipeline and exports them to a text file. The script enumerates the bonds of each particle 
using the :py:class:`Bonds.Enumerator <ovito.data.Bonds.Enumerator>` helper class.

The generated text file has the following format::

   Atom    CNA_pair_type:Number_of_such_pairs ...
   
   1       [4 2 1]:2  [4 2 2]:1 [5 4 3]:1 
   2       ...    
   ...

Python script:

.. literalinclude:: ../../../examples/scripts/cna_bond_analysis.py

 
  
.. _example_creating_particles_programmatically:

------------------------------------
Creating particles programmatically
------------------------------------

The following script demonstrates how to create particles, a simulation cell, and bonds on the fly
without loading an external simulation file. This approach can be used to implement custom data importers
or dynamically generate atomic structures, which can then be further processed with OVITO or exported to a file.

The script creates different data objects and adds them to a new :py:class:`~ovito.data.DataCollection` instance.
Finally, an :py:class:`~ovito.ObjectNode` is created and the :py:class:`~ovito.data.DataCollection` is set as
its data source.

.. literalinclude:: ../../../tests/scripts/test_suite/create_new_particle_property.py
