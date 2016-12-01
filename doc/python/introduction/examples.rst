==================================
Examples
==================================

This page provides a collection of example scripts:

   * :ref:`example_compute_voronoi_indices`
   * :ref:`example_compute_cna_bond_indices`
   * :ref:`example_msd_calculation`
   * :ref:`example_order_parameter_calculation`
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


.. _example_msd_calculation:

--------------------------------------------------------------------------
Writing a custom modifier for calculating the mean square displacement
--------------------------------------------------------------------------

OVITO allows you to implement your :ref:`own type of analysis modifier <writing_custom_modifiers>` by writing a Python function that gets called every time
the data pipeline is evaluated. This user-defined function has access to the positions and other properties of particles 
and can output information and results as new properties or global attributes.

As a first simple example, we look at the calculation of the mean square displacement (MSD) in a system of moving particles.
OVITO already provides the built-in `Displacement Vectors <../../particles.modifiers.displacement_vectors.html>`_ modifier, which 
calculates the displacement of every particle. It stores its results in the ``"Displacement Magnitude"``
particle property. So all our custom analysis modifier needs to do is to sum up the squared displacement magnitudes and divide by the number of particles:

.. literalinclude:: ../example_snippets/msd_calculation.py
  :lines: 14-25

When used within the graphical program, the MSD value computed by this custom modifier may be exported to a text file as a function of simulation time using
OVITO's standard file export feature (Select ``Calculation Results Text File`` as output format).

Alternatively, we can make use of the custom modifier from within a non-interactive batch script, which is executed
by the ``ovitos`` interpreter. Then we have to insert the :py:class:`~ovito.modifiers.CalculateDisplacementsModifier` programmatically:

.. literalinclude:: ../example_snippets/msd_calculation.py

.. _example_order_parameter_calculation:

--------------------------------------------------
Implementing an advanced analysis modifier
--------------------------------------------------

In the paper `[Phys. Rev. Lett. 86, 5530] <https://doi.org/10.1103/PhysRevLett.86.5530>`_ an order parameter has been specified as a means
of labeling an atom in the simulation as belonging to either the liquid or solid fcc crystal phase. In the following we will 
develop a custom analysis modifier for OVITO, which calculates this per-atom order parameter.

The order parameter is defined as follows (see the paper for details): For any of the 12 nearest neighbors of a given atom one can compute the distance the neighbor 
makes from the ideal fcc positions of the crystal in the given orientation (denoted by vector **r**\ :sub:`fcc`). The sum of the distances over the 12 neighbors, 
phi = 1/12*sum(| **r**\ :sub:`i` - **r**\ :sub:`fcc` |), acts as an "order parameter" for the central atom.

Calculating this parameter involves finding the 12 nearest neighbors of each atom and, for each of these neighbors, determining the
closest ideal lattice vector. To find the neighbors, OVITO provides the :py:class:`~ovito.data.NearestNeighborFinder` utility class.
It directly provides the vectors from the central atom to its nearest neighbors.

Let us start by defining some inputs for the order parameter calculation at the global scope:

.. literalinclude:: ../example_snippets/order_parameter_calculation.py
  :lines: 7-32

The actual modifier function needs to create an output particle property, which will store the calculated
order parameter of each atom. Two nested loops run over all input atoms and their 12 nearest neighbors respectively.

.. literalinclude:: ../example_snippets/order_parameter_calculation.py
  :lines: 34-67

Note that the ``yield`` statements in the modifier function above are only needed to support progress feedback in the
graphical version of OVITO and to give the pipeline system the possibility to interrupt the long-running calculation when needed. 

.. _example_creating_particles_programmatically:

--------------------------------------------------
Creating particles and bonds programmatically
--------------------------------------------------

The following script demonstrates how to create particles, a simulation cell, and bonds on the fly
without loading an external simulation file. This approach can be used to implement custom data importers
or dynamically generate atomic structures, which can then be further processed with OVITO or exported to a file.

The script creates different data objects and adds them to a new :py:class:`~ovito.data.DataCollection` instance.
Finally, an :py:class:`~ovito.ObjectNode` is created and the :py:class:`~ovito.data.DataCollection` is set as
its data source.

.. literalinclude:: ../../../tests/scripts/test_suite/create_new_particle_property.py

