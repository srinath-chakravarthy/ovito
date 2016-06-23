.. _modifiers_overview:

===================================
Modifiers
===================================

Modifiers are objects that make up a node's modification pipeline.
They modify, filter, or extend the data that flows down the pipeline from the 
:py:class:`~ovito.io.FileSource` to the node's output cache, which is an instance of the 
:py:class:`~ovito.data.DataCollection` class.

You insert a new modifier into a pipeline by first creating a new instance of the corresponding modifier class
(See :py:mod:`ovito.modifiers` module for the list of available modifier classes) and then 
adding it to the node's :py:attr:`~ovito.ObjectNode.modifiers` list::

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
modification pipeline (or the :py:class:`~ovito.io.FileSource`). Typically they produce new 
output data (for example an additional particle property), which is fed back into the pipeline 
where it will be accessible to the following modifiers (e.g. a :py:class:`~ovito.modifiers.ColorCodingModifier`).

Let us take the :py:class:`~ovito.modifiers.CommonNeighborAnalysisModifier` as an example for a typical analysis modifier. 
It takes the particle positions as input and classifies each particle as either FCC, HCP, BCC, or some other
structural type. This per-particle information computed by the modifier is inserted into the pipeline as a new 
:py:class:`~ovito.data.ParticleProperty` data object. Since it flows down the pipeline, this particle property
is accessible by subsequent modifiers and will eventually arrive in the node's output data collection
where we can access it from a Python script::

    >>> cna = CommonNeighborAnalysis()
    >>> node.modifiers.append(cna)
    >>> node.compute()
    >>> print(node.output.particle_properties.structure_type.array)
    [1 0 0 ..., 1 2 0]
    
Note that the :py:class:`~ovito.modifiers.CommonNeighborAnalysisModifier` encodes the computed
structural type of each particle as an integer number (0=OTHER, 1=FCC, ...). 

In addition to this kind of per-particle data, many analysis modifiers generate global information
as part of their computation. This information, which typically consists of scalar quantities, is inserted into the data 
pipeline as *attributes*. For instance, the  :py:class:`~ovito.modifiers.CommonNeighborAnalysisModifier` reports
the total number of particles that match the FCC structure type as an attribute named ``CommonNeighborAnalysis.counts.FCC``::

    >>> node.output.attributes['CommonNeighborAnalysis.counts.FCC']
    1262
    
Note how we could have obtained the same value by explicitly counting the number of particles of FCC type
ourselves::

    >>> structure_property = node.output.particle_properties.structure_type.array
    >>> numpy.count_nonzero(structure_property == CommonNeighborAnalysisModifier.Type.FCC)
    1262
    
Attributes are stored in the :py:attr:`~ovito.data.DataCollection.attributes` dictionary of the :py:class:`~ovito.data.DataCollection`.
The class documentation of each modifier lists the attributes that it generates.
