==================================
Overview
==================================

OVITO scripting interface provides full access to most of OVITO's program features. Using Python scripts, you can
do many things that are already familiar from the graphical user interface (and even a few more):

  * :ref:`Import data from external files <file_io_overview>`
  * :ref:`Apply modifiers to a dataset and configure them <modifiers_overview>`
  * Set up a camera and render pictures or movies of the scene
  * Control the visual appearance of particles and other objects
  * Access per-particle properties and other analysis results computed by OVITO
  * :ref:`Implement new types of modifiers <writing_custom_modifiers>`
  * :ref:`Export data to a file <file_output_overview>`

The following sections will introduce the essential concepts and walk you through different parts of OVITO's 
scripting framework.

------------------------------------
OVITO's data pipeline architecture
------------------------------------

If you have worked with OVITO's graphical user interface before, you should already be familiar with 
its key workflow concept: After loading a simulation file into OVITO you typically apply one or more modifiers 
that act on the input data. The result of this sequence of modifiers (*modification pipeline*) is computed by OVITO 
and displayed in the interactive viewports.

To access this capability from a script, we first need to understand the basics of OVITO's underlying 
data model. In general, there are two different groups of objects that participate in the described system: 
Objects that constitute the modification pipeline (the modifiers and the data source) and *data objects*, which 
carry the data that is being processed by the modifier. The data objects enter the modification pipeline, 
get modified by a modifier, or are newly produced (e.g. computed particle properties). 
We start by discussing the objects that consitute a modification pipeline.

------------------------------------
Data sources, modifiers, and more
------------------------------------

A modification pipeline is fed by a *data source*, which is an object
that provides or generates the input data entering a modification pipeline. OVITO currently knows two types of 
data sources: :py:class:`~ovito.io.FileSource` and :py:class:`~ovito.data.DataCollection`.
The :py:class:`~ovito.io.FileSource` class is the data source typically used. It is responsible for loading data 
from an external file and passing it on to the modification pipeline.

The data source and the modification pipeline together form an :py:class:`~ovito.ObjectNode`. This class
orchestrates the data flow from the source into the modification pipeline and caches the pipeline's output. 
As we will see later, the :py:class:`~ovito.ObjectNode` is also responsible for displaying the output
data in the three-dimensional scene. The data source is stored in the :py:attr:`ObjectNode.source <ovito.ObjectNode.source>`
property. The modification pipeline is simply a list of :py:class:`~ovito.modifiers.Modifier` objects and is 
is accessible through the :py:attr:`ObjectNode.modifiers <ovito.ObjectNode.modifiers>` property. 

The :py:class:`~ovito.ObjectNode` is usually placed in the *scene*, i.e. the three-dimensional world that is visible
through OVITO's viewports. All objects in the scene, and all other information that would get saved along in 
a :file:`.ovito` file (e.g. current render settings, viewport cameras, etc.), comprise the so-called :py:class:`~ovito.DataSet`. 
A Python script always runs in the context of one global :py:class:`~ovito.DataSet` instance. This 
instance can be accessed through the :py:data:`ovito.dataset` global variable. The :py:class:`~ovito.DataSet` provides access to the
list of object nodes in the scene (:py:attr:`dataset.scene_nodes <ovito.DataSet.scene_nodes>`), 
the current animation settings (:py:attr:`dataset.anim <ovito.DataSet.anim>`), the four 
viewports in OVITO's main window (:py:attr:`dataset.viewports <ovito.DataSet.viewports>`), and more.

.. image:: graphics/ObjectNode.*
   :width: 86 %
   :align: center

------------------------------------
Loading data and applying modifiers
------------------------------------

An :py:class:`~ovito.ObjectNode` is automatically created when you import a data file into OVITO 
using the :py:func:`ovito.io.import_file` function::

   >>> from ovito.io import *
   >>> node = import_file("simulation.dump")
   
This high-level function creates an :py:class:`~ovito.ObjectNode` with an empty modification pipeline
and sets up a :py:class:`~ovito.io.FileSource` (which will subsequently load the data 
from the given file) and assigns it to the :py:attr:`ObjectNode.source <ovito.ObjectNode.source>` property. 

We can now start populating the node's modification pipeline with some modifiers by appending them
to the :py:attr:`ObjectNode.modifiers <ovito.ObjectNode.modifiers>` list::

   >>> from ovito.modifiers import *
   >>> node.modifiers.append(SelectExpressionModifier(expression="PotentialEnergy < -3.9"))
   >>> node.modifiers.append(DeleteSelectedParticlesModifier())

Modifiers are constructed by calling the constructor of the corresponding classes, which are
all found in the :py:mod:`ovito.modifiers` module. Note how a modifier's parameters can be initialized in two ways:

.. note::

   When constructing a new object (such as a modifier, but also most other OVITO objects) it is possible to directly initialize its
   properties by passing keyword arguments to the constructor function. Thus ::
   
       node.modifiers.append(CommonNeighborAnalysisModifier(cutoff = 3.2, only_selected = True))
       
   is equivalent to setting the properties one by one after constructing the object::

       modifier = CommonNeighborAnalysisModifier()
       modifier.cutoff = 3.2
       modifier.only_selected = True
       node.modifiers.append(modifier)
       
After the modification pipeline has been populated with modifiers, we can do three different things:
(i) write the results to a file, (ii) render an image of the data, (iii) or directly work with the pipeline 
data and read out particle properties and other results. Keep reading.

------------------------------------
Exporting data to a file
------------------------------------

Exporting the data that has left the modification pipeline to a file is simple; 
we use the :py:func:`ovito.io.export_file` function for this::

    >>> export_file(node, "outputdata.dump", "lammps_dump",
    ...    columns = ["Position.X", "Position.Y", "Position.Z", "Structure Type"])
    
The first argument of this high-level function is the node whose pipeline results should be exported.
It is followed by the name of the output file and the desired output format. 
Depending on the selected file format, additional keyword arguments such as the list of particle properties to 
be exported must be provided. See the documentation of the :py:func:`~ovito.io.export_file` function for more information. 

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
   
Now we have specified the output filename and the size of the image in pixels.
We should not forget to also add the :py:class:`~ovito.ObjectNode` to the *scene* by calling::

    >>> node.add_to_scene()

Because only object nodes that are part of the scene are visible in the viewports and in rendered images.
Finally, we can let OVITO render an image of the viewport::

    >>> vp.render(settings)
    
As a final remark, note how we could have used the more compact notation for object initialization introduced above.
We can configure the newly created :py:class:`~ovito.vis.Viewport` and :py:class:`~ovito.vis.RenderSettings` by passing the parameter values directly to the class constructors:: 

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

OVITO's scripting interface allows you to directly access the output data leaving the
modification pipeline. But before doing so, we first have to ask OVITO to compute the results of the modification pipeline::

    >>> node.compute()
    
The :py:meth:`~ovito.ObjectNode.compute` method ensures that all modifiers in the pipeline of the node
have been successfully evaluated. Note that the :py:meth:`~ovito.vis.Viewport.render` and 
:py:func:`~ovito.io.export_file` functions implicitly call :py:meth:`~ovito.ObjectNode.compute`
for us. But now, since we want to directly access the pipeline results, we have to explicitly request 
an evaluation of the modification pipeline.

The node caches the results of the last pipeline evaluation in the :py:attr:`ObjectNode.output <ovito.ObjectNode.output>` field
in the form of a :py:class:`~ovito.data.DataCollection`::

    >>> node.output
    DataCollection(['Simulation cell', 'Particle Identifier', 'Position', 
                    'Potential Energy', 'Color', 'Structure Type'])
    
It contains all the *data objects* that were processed or produced  
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

See the :py:mod:`ovito.data` module for a list of data object types that may occur in a :py:class:`~ovito.data.DataCollection`.

Sometimes we might also be interested in the data that *enters* the modification pipeline.
The input data, which was read from the external file, is cached by the :py:class:`~ovito.io.FileSource`,
which is itself a :py:class:`~ovito.data.DataCollection`::

    >>> node.source
    DataCollection(['Simulation cell', 'Particle Identifier', 'Position'])

-------------------------------------------------
Controlling the visual appearance of objects
-------------------------------------------------

So far we have only looked at objects that represent data, e.g. particle properties or the simulation cell. 
Let's see how this data is displayed and how we can control its visual appearance.

Every data object with a visual representation in OVITO is associated with a matching :py:class:`~ovito.vis.Display`
object. The display object is stored in the data object's :py:attr:`~.ovito.data.DataObject.display` property. For example::

    >>> cell = node.source.cell
    >>> cell                               # This is the SimulationCell data object
    <SimulationCell at 0x7f9a414c8060>
    
    >>> cell.display                       # This is its attached display object
    <SimulationCellDisplay at 0x7fc3650a1c20>

The :py:class:`~ovito.vis.SimulationCellDisplay` is responsible for rendering the simulation
cell in the viewports and provides parameters that allow us to configure the visual appearance. For example, to change the
display color of the simulation box::

    >>> cell.display.rendering_color = (1.0, 0.0, 1.0)

We can also turn off the display of any object entirely by setting the :py:attr:`~ovito.vis.Display.enabled`
attribute of the display to ``False``::

    >>> cell.display.enabled = False 

Particles are rendered by a :py:class:`~ovito.vis.ParticleDisplay` object. It is always attached to the 
:py:class:`~ovito.data.ParticleProperty` object storing the particle positions (which is the only mandatory particle
property that is always defined). Thus, to change the visual appearance of particles, 
we have to access the ``Positions`` particle property in the :py:class:`~ovito.data.DataCollection`::

    >>> pos_prop = node.source.particle_properties.position
    >>> pos_prop
    <ParticleProperty at 0x7ff5fc868b30>
      
    >>> pos_prop.display
    <ParticleDisplay at 0x7ff5fc868c40>
       
    >>> pos_prop.display.shading = ParticleDisplay.Shading.Flat
    >>> pos_prop.display.radius = 1.4
