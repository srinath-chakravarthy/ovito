==================================
Overview
==================================

OVITO scripting interface provides full access to most of OVITO's program features. Using Python scripts, you can
do many things that are already familiar from the graphical user interface (and even a few more):

  * Import data from simulation files
  * Apply modifiers to the data
  * Let OVITO compute the results of the modifier pipeline
  * Set up a camera and render pictures or movies of the scene
  * Control the visual appearance of particles and other objects
  * Access per-particle properties and analysis results computed by OVITO
  * Implement new types of modifiers
  * Export data to a file

The following sections will introduce the essential concepts and walk you through different parts of OVITO's 
scripting interface.

------------------------------------
OVITO's data flow architecture
------------------------------------

If you have worked with OVITO's graphical user interface before, you should already be familiar with 
its key workflow concept: After loading a simulation file into OVITO and typically set up a sequence of modifiers 
that act on that input data. The results of this *modification pipeline* are computed by OVITO 
and displayed in the interactive viewports.

To make use of this capability from a script, we first need to understand the basics of OVITO's underlying 
data model. In general, there are two different groups of objects that participate in the described system: 
Objects that constitute the modification pipeline (e.g. modifiers and a data source) and *data objects*, which 
enter the modification pipeline, get modified by it, or are newly produced by modifiers (e.g. particle properties). 
We start by discussing the first group in the next section.

------------------------------------
Data sources, modifiers, and more
------------------------------------

It all starts with a *data source*, which is an object
that provides the input data entering a modification pipeline. OVITO currently knows two types of 
data sources: The :py:class:`~ovito.io.FileSource` and the :py:class:`~ovito.data.DataCollection`.
In the following we will discuss the :py:class:`~ovito.io.FileSource` class, because that is the type of data source
typically used. It is responsible for loading data from an external file and passing it on to the modification pipeline.

The data source and the modification pipeline together form an :py:class:`~ovito.ObjectNode`. This class
orchestrates the data flow from the source into the modification pipeline and stores the pipeline's output in an internal 
data cache. As we will see later, the :py:class:`~ovito.ObjectNode` is also responsible for displaying the output
data in the three-dimensional scene. The data source is stored in the node's :py:attr:`~ovito.ObjectNode.source`
property, while the modification pipeline is accessible through the node's :py:attr:`~ovito.ObjectNode.modifiers`
property. The pipeline is exposed as a Python list that can be populated with modifiers.

The :py:class:`~ovito.ObjectNode` is usually placed in the *scene*, i.e. the three-dimensional world that is visible
through OVITO's viewports. All objects in the scene, and all other information that would get saved along in 
a :file:`.ovito` file (e.g. current render settings, viewport cameras, etc.), comprise the so-called :py:class:`~ovito.DataSet`. 
A Python script always runs in the context of exactly one global :py:class:`~ovito.DataSet` instance. This 
instance can be accessed through the :py:data:`ovito.dataset` module-level attribute. It provides access to the
list of object nodes in the scene (:py:attr:`dataset.scene_nodes <ovito.DataSet.scene_nodes>`), 
the current animation settings (:py:attr:`dataset.anim <ovito.DataSet.anim>`), the four 
viewports in OVITO's main window (:py:attr:`dataset.viewports <ovito.DataSet.viewports>`), and more.

.. image:: graphics/ObjectNode.*
   :width: 86 %
   :align: center

------------------------------------
Loading data and applying modifiers
------------------------------------

After the general object model has been described above, it is now time to give some code examples and demonstrate how
we deal with these things in a script. Typically, we first would like to load a simulation file. This is done
using the :py:func:`ovito.io.import_file` function::

   >>> from ovito.io import *
   >>> node = import_file("simulation.dump")
   
This high-level function does several things: It creates a :py:class:`~ovito.io.FileSource` (which will subsequently load the data 
from the given file), it creates an :py:class:`~ovito.ObjectNode` instance with an empty modification pipeline, and assigns the 
:py:class:`~ovito.io.FileSource` to the :py:attr:`~ovito.ObjectNode.source` property of the node. The function finally returns the 
newly created node to the caller after inserting it into the scene.

We can now start to populate the node's modification pipeline with some modifiers::

   >>> from ovito.modifiers import *
   >>> node.modifiers.append(SelectExpressionModifier(expression="PotentialEnergy < -3.9"))
   >>> node.modifiers.append(DeleteSelectedParticlesModifier())

Here we have created two modifiers and appended them to the modification pipeline. Note how a modifier's parameters 
can be initialized in two ways:

.. note::

   When constructing new objects such as modifiers it is possible to initialize object
   parameters using an arbitrary number of keyword arguments at construction time. Thus ::
   
       node.modifiers.append(CommonNeighborAnalysisModifier(cutoff = 3.2, mode = CommonNeighborAnalysisModifier.Mode.FixedCutoff))
       
   is equivalent to::

       modifier = CommonNeighborAnalysisModifier()
       modifier.cutoff = 3.2
       modifier.mode = CommonNeighborAnalysisModifier.Mode.FixedCutoff
       node.modifiers.append(modifier)
       
After the modification pipeline has been populated with the desired modifiers, we can basically do three things:
(i) write the results to a file, (ii) render an image of the data, (iii) or directly work with the pipeline 
data and read out particle properties, for instance.

------------------------------------
Exporting data to a file
------------------------------------

Exporting the processed data to a file is simple; we use the :py:func:`ovito.io.export_file` function
for this::

    >>> export_file(node, "outputdata.dump", "lammps_dump",
    ...    columns = ["Position.X", "Position.Y", "Position.Z", "Structure Type"])
    
The first argument passed to this high-level function is the node whose pipeline results should be exported.
Furthermore, the name of the output file and the format are specified by the second and third parameter. 
Depending on the selected file format, additional keyword parameters such as the list of particle properties to 
be exported must be provided.

------------------------------------
Rendering images
------------------------------------

To render an image, we first need a viewport that defines the view on the three-dimensional scene.
We can either use one of the four predefined viewports of OVITO for this, or simply create an *ad hoc* 
:py:class:`~ovito.vis.Viewport` instance in Python::

    >>> from ovito.vis import *
    >>> vp = Viewport()
    >>> vp.type = Viewport.Type.PERSPECTIVE
    >>> vp.camera_pos = (-100, -150, 150)
    >>> vp.camera_dir = (2, 3, -3)
    >>> vp.fov = math.radians(60.0)
    
As you can see, the :py:class:`~ovito.vis.Viewport` class has several parameters that control the 
position and orientation of the camera, the projection type, and the field of view (FOV) angle. Note that this
viewport will not be visible in OVITO's main window, because it is not part of the current :py:class:`~ovito.DataSet`; 
it is only a temporary object used within the script.

In addition we need to create a :py:class:`~ovito.vis.RenderSettings` object, which controls the rendering
process (These are the parameters you normally set on the :guilabel:`Render` tab in OVITO's main window)::

    >>> settings = RenderSettings()
    >>> settings.filename = "myimage.png"
    >>> settings.size = (800, 600)
    
Here we have specified the output filename and the size of the image in pixels. Finally, we can let OVITO render 
the image::

    >>> vp.render(settings)
    
Note again how we can instead use the more compact notation to initialize the :py:class:`~ovito.vis.Viewport`
and the :py:class:`~ovito.vis.RenderSettings` by passing the parameter values to the class constructors:: 

    vp = Viewport(
        type = Viewport.Type.PERSPECTIVE,
        camera_pos = (-100, -150, 150),
        camera_dir = (2, 3, -3),
        fov = math.radians(60.0)
    )
    vp.render(RenderSettings(filename = "myimage.png", size = (800, 600)))

------------------------------------
Accessing computation results
------------------------------------

OVITO's scripting interface allows us to directly access the output data leaving the
modification pipeline. But first we have to ask OVITO to compute the results of the modification pipeline::

    >>> node.compute()
    
The node's :py:meth:`~ovito.ObjectNode.compute` method ensures that all modifiers in the pipeline
have been successfully evaluated. Note that the :py:meth:`~ovito.vis.Viewport.render` and 
:py:func:`~ovito.io.export_file` functions introduced above implicitly call :py:meth:`~ovito.ObjectNode.compute`
for us. But now, to gain direct access to the results, we have to explicitly request 
an evaluation of the modification pipeline.

The node caches the results of the last pipeline evaluation in its :py:attr:`~ovito.ObjectNode.output` field::

    >>> node.output
    DataCollection(['Simulation cell', 'Particle Identifier', 'Position', 
                    'Potential Energy', 'Color', 'Structure Type'])
    
The :py:class:`~ovito.data.DataCollection` contains the *data objects* that were produced
by the modification pipeline. For example, to access the :py:class:`simulation cell <ovito.data.SimulationCell>` we would write::

    >>> node.output.cell.matrix
    [[ 148.147995      0.            0.          -74.0739975 ]
     [   0.          148.07200623    0.          -74.03600311]
     [   0.            0.          148.0756073   -74.03780365]]
     
    >>> node.output.cell.pbc
    (True, True, True)

Similarly, the data of individual :py:class:`particle properties <ovito.data.ParticleProperty>` may be accessed as NumPy arrays:

    >>> import numpy
    >>> node.output.particle_properties.position.array
    [[ 73.24230194  -5.77583981  -0.87618297]
     [-49.00170135 -35.47610092 -27.92519951]
     [-50.36349869 -39.02569962 -25.61310005]
     ..., 
     [ 42.71210098  59.44919968  38.6432991 ]
     [ 42.9917984   63.53770065  36.33330154]
     [ 44.17670059  61.49860001  37.5401001 ]]

Sometimes we might be more interested in the data that *enters* the modification pipeline. 
The input data, which was read from the external file, is cached by the :py:class:`~ovito.io.FileSource`,
which is a :py:class:`~ovito.data.DataCollection` itself::

    >>> node.source
    DataCollection(['Simulation cell', 'Particle Identifier', 'Position'])

-------------------------------------------------
Controlling the visual appearance of objects
-------------------------------------------------

So far we have only looked at objects that represent data, e.g. particle properties or the simulation cell. 
How is this data displayed, and how can we control the visual appearance?

Every data object that can be visualized in OVITO is associated with a matching :py:class:`~ovito.vis.Display`
object. The display object is stored in the data object's :py:attr:`~.ovito.data.DataObject.display` property. For example::

    >>> cell = node.source.cell
    >>> cell                               # This is the data object
    <SimulationCell at 0x7f9a414c8060>
    
    >>> cell.display                       # This is its attached display object
    <SimulationCellDisplay at 0x7fc3650a1c20>

The py:class:`~ovito.vis.SimulationCellDisplay` instance is responsible for rendering the simulation
cell and provides parameters that allow us to influence the visual appearance. For example, to change the
display color of the simulation box::

    >>> cell.display.rendering_color = (1.0, 0.0, 1.0)

We can also turn off the display of any object completely by setting the :py:attr:`~ovito.vis.Display.enabled`
attribute of the display to ``False``::

    >>> cell.display.enabled = False 

The particles are rendered by a corresponding :py:class:`~ovito.vis.ParticleDisplay` object. It is always attached to the 
:py:class:`~ovito.data.ParticleProperty` data object storing the particle positions (the only mandatory particle
property that is always defined). Thus, to change the visual appearance of particles, 
we have to access the particle positions property in the :py:class:`~ovito.data.DataCollection`::

    >>> pos_prop = node.source.particle_properties.position
    >>> pos_prop
    <ParticleProperty at 0x7ff5fc868b30>
      
    >>> pos_prop.display
    <ParticleDisplay at 0x7ff5fc868c40>
       
    >>> pos_prop.display.shading = ParticleDisplay.Shading.Flat
    >>> pos_prop.display.radius = 1.4

