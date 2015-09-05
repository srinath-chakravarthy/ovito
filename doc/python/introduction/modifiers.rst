===================================
Modifiers
===================================

Modifiers are objects that can be inserted into a node's modification pipeline.
They modify, filter, or extend the data that flows down the pipeline from the 
:py:class:`~ovito.io.FileSource` to the node's output cache, which is an instance of the 
:py:class:`~ovito.data.DataCollection` class.

You insert a new modifier by creating a new instance of the corresponding modifier class
(See :py:mod:`ovito.modifiers` module for the list of available modifier classes) and 
adding it to the node's :py:attr:`~ovito.ObjectNode.modifiers`
list property::

   >>> from ovito.modifiers import *
   >>> mod = AssignColorModifier( color=(0.5, 1.0, 0.0) )
   >>> node.modifiers.append(mod)
   
Entries in the :py:attr:`ObjectNode.modifiers <ovito.ObjectNode.modifiers>` list are processed front to back, i.e.,
appending a modifier to the end of the list will position it at the end of the modification pipeline.
This corresponds to the bottom-up execution order known from OVITO's graphical user interface.

Note that inserting a new modifier into the modification pipeline does not directly trigger a
computation. The modifier will only be evaluated when the results of the pipeline need to be recomputed. 
Evaluation of the modification pipeline can either happen implicitly, e.g. 

  * when the interactive viewports in OVITO's main window are updated, 
  * when rendering an image,
  * when exporting data using :py:func:`ovito.io.export_file`,
  
or explicitly, when calling the :py:meth:`ObjectNode.compute() <ovito.ObjectNode.compute>` method.
This method explicitly updates the output cache holding the results of the node's modification pipeline.
The output of the modification pipeline is stored in a :py:class:`~ovito.data.DataCollection`
that can be accessed through the :py:attr:`output <ovito.ObjectNode.output>` 
attribute of the object node. The data collection holds all data objects that
have left modification pipeline the last time it was evaluated::

    >>> node.compute()
    >>> node.output
    DataCollection(['Simulation cell', 'Position', 'Color'])
    
    >>> for key in node.output:
    ...     print(node.output[key])
    <SimulationCell at 0x7fb6238f1b30>
    <ParticleProperty at 0x7fb623d0c760>
    <ParticleProperty at 0x7fb623d0c060>

In this example, the output data collection consists of a :py:class:`~ovito.data.SimulationCell`
object and two :py:class:`~ovito.data.ParticleProperty` objects, which store the particle positions and 
particle colors. We will learn more about the :py:class:`~ovito.data.DataCollection` class and
particle properties later.

---------------------------------
Analysis modifiers
---------------------------------

Analysis modifiers perform some computation based on the data they receive from the upstream part of the
modification pipeline (or the :py:class:`~ovito.io.FileSource`). Typically they produce additional 
output data (for example a new particle property), which is inserted back into the pipeline 
where it is accessible to the following modifiers (e.g. a :py:class:`~ovito.modifiers.ColorCodingModifier`).

Let us take the :py:class:`~ovito.modifiers.CommonNeighborAnalysisModifier` as an example for an analysis modifier. 
It takes the particle positions as input and classifies each particle as either FCC, HCP, BCC, or some other
structural type. This per-particle information computed by the modifier is inserted into the pipeline as a new 
:py:class:`~ovito.data.ParticleProperty` data object. Since it flows down the pipeline, this particle property
is accessible by subsequent modifiers and will eventually arrive in the node's output data collection
where we can access it from the Python script::

    >>> cna = CommonNeighborAnalysis()
    >>> node.modifiers.append(cna)
    >>> node.compute()
    >>> print(node.output.particle_properties.structure_type.array)
    [1 0 0 ..., 1 2 0]
    
Note that the :py:class:`~ovito.modifiers.CommonNeighborAnalysisModifier` encodes the computed
structural type of each particle as an integer number (0=OTHER, 1=FCC, ...). 

In addition to this per-particle data, some analysis modifiers generate global status information
as part of the computation. This information is not inserted into the data pipeline; instead it is 
cached by the modifier itself and can be directly accessed as an attribute of the modifier class. For instance, 
the :py:attr:`~ovito.modifiers.CommonNeighborAnalysisModifier.counts` attribute of the :py:class:`~ovito.modifiers.CommonNeighborAnalysisModifier` lists 
the number of particles found by the modifier for each structural type::

    >>> for c in enumerate(cna.counts):
	...     print("Structure type %i: %i particles" % c)
    Structure type 0: 117317 particles
    Structure type 1: 1262 particles
    Structure type 2: 339 particles
    Structure type 3: 306 particles
    Structure type 4: 0 particles
    Structure type 5: 0 particles
    
Note that the :py:class:`~ovito.modifiers.CommonNeighborAnalysisModifier` class defines a set of integer constants 
that make it easier to refer to individual structure types, e.g.::

    >>> print("Number of FCC atoms:", cna.counts[CommonNeighborAnalysisModifier.Type.FCC])
    Number of FCC atoms: 1262

.. important::

   The most important thing to remember here is that the modifier caches information from the 
   last pipeline evaluation. That means you have to call :py:meth:`ObjectNode.compute() <ovito.ObjectNode.compute>` first 
   before you access these output attributes of a modifier to make ensure that the values are up to date!
 