.. _writing_custom_modifiers:

===================================
Writing new modifiers
===================================

OVITO provides a collection of built-in data manipulation and analysis modifiers, which can be found in the :py:mod:`ovito.modifiers` module.
These modifier types are all implemented in C++, and the Python interface allows you to instantiate them, 
insert them into the modification pipeline of an :py:class:`~ovito.ObjectNode`, and configure their parameters.
However, sometimes the capabilities provided by these built-in modifiers are not sufficent and you may want to
write your own, completely new type of modifier that can participate in the data pipeline system of OVITO.
The following sections describe how this is done.

----------------------------------------------
Inserting a custom modifier into the pipeline
----------------------------------------------

Creating a user-defined modifier essentially requires writing a Python function with the following signature, 
which is responsible for computing the effect of the custom modifier::

  def modify(frame, input, output):
      ...

The meaning of the parameters and the implementation of this function will be described 
in later sections. You can insert the custom modifier into the modification pipeline either by using 
OVITO's graphical user interface or programmatically from Python:

  1. Within the graphical user interface, select *Python script* from the modifier drop-down list to insert
     a Python script modifier into the modification pipeline. OVITO provides a text input field
     which allows you to enter the code for the ``modify()`` function. The 
     `corresponding page <../../particles.modifiers.python_script.html>`_ in the OVITO
     user manual provides more information on this procedure and on how you can save a custom script modifier 
     for future use within the graphical program.
     
  2. Via the scripting interface, a custom modifier is inserted into the data pipeline
     of an :py:class:`~ovito.ObjectNode` by creating an instance of the :py:class:`~ovito.modifiers.PythonScriptModifier`
     class as follows::
     
        from ovito.modifiers import PythonScriptModifier
     
        # Our custom modifier function:
        def my_modifier(frame, input, output):
            ...
            
        # Inserting it into the modification pipeline of the node:
        node.modifiers.append(PythonScriptModifier(function = my_modifier))

     Note how our custom modifier function, which can have any name in this case, 
     is assigned to the :py:attr:`~ovito.modifiers.PythonScriptModifier.function` attribute of the
     :py:class:`~ovito.modifiers.PythonScriptModifier` instance, which in turn is inserted into the modification pipeline.
     
-----------------------------------
The modifier function
-----------------------------------

The custom modifier function defined above is called by OVITO every time the modification pipeline
is evaluated. It receives the data produced by the upstream part of the pipeline (e.g. the particles
loaded by a :py:class:`~ovito.io.FileSource` and further processed by other modifiers that 
precede the custom modifier in the pipeline). Our Python modifier function then has the possibility to modify or extend
the data as needed. After the user-defined Python function returns, the output flows further down the pipeline, and, eventually, 
the final results are stored in the :py:attr:`~ovito.ObjectNode.output` cache of the :py:class:`~ovito.ObjectNode` and rendered in the viewports.

Our custom modifier function is invoked by the system with three arguments:

  * **frame** (*int*) -- The animation frame number at which the pipeline is evaluated. 
  * **input** (:py:class:`~ovito.data.DataCollection`) -- Contains the input data objects that the modifier receives from upstream.
  * **output** (:py:class:`~ovito.data.DataCollection`) -- This is where the modifier function should put its output data objects. 
  
The *input* :py:class:`~ovito.data.DataCollection`, and in particular the data objects stored in it, should not be modified by the modifier function.
They are owned by the upstream part of the modification pipeline and must be accessed in a read-only fashion (e.g. by using the :py:attr:`~ovito.data.ParticleProperty.array`
attribute instead of :py:attr:`~ovito.data.ParticleProperty.marray` to access per-particle values of a :py:class:`~ovito.data.ParticleProperty`).

When the user-defined modifier function is invoked by the system, the *output* data collection already contains
all data objects from the *input* collection. Thus, the default behavior is that all objects (e.g. particle properties, simulation cell, etc.) are passed
through unmodified. 

Modifying existing data objects
-----------------------------------

For performance reasons no copies are made by default, and the *output* collection references the same data objects as the *input* collection.
This means, before it is safe to modify a data object in the *output* data collection, you have to make a copy first. Otherwise you risk 
modifying data that is owned by the upstream part of the modification pipeline (e.g. the :py:class:`~ovito.io.FileSource`). An in-place copy of a data object
is made using the :py:meth:`DataCollection.copy_if_needed() <ovito.data.DataCollection.copy_if_needed>` method. The following example demonstrates the 
principle:: 

   def modify(frame, input, output):
   
       # Simulation cell is passed through by default:
       assert(output.cell is input.cell)
       
       # Make a copy of the simulation cell:
       cell = output.copy_if_needed(output.cell)
       
       # Now we are allowed to modify the copy:
       cell.pbc = (False, False, False)
       
       # The object was copied in place in the output data collection:
       assert(cell is output.cell)
       assert(cell is not input.cell)
       

Adding new data objects
-----------------------------------

The custom modifier function can inject new data objects into the modification pipeline simply by adding
them to the *output* data collection::

   def modify(frame, input, output):
   
       # Create a new bonds data object and a bond between atoms 0 and 1.
       bonds = ovito.data.Bonds()
       bonds.add_full(0, 1)
       
       # Insert into output collection:
       output.add(bonds)
       
For adding new particle properties (or overwriting existing properties), 
a special method :py:meth:`~ovito.data.DataCollection.create_particle_property` is provided 
by the :py:class:`~ovito.data.DataCollection` class::

   def modify(frame, input, output):   
       # Create the 'Color' particle property and set the color of all particles to green:
       color_property = output.create_particle_property(ParticleProperty.Type.Color)
       color_property.marray[:] = (1.0, 0.0, 0.0)

Note that :py:meth:`~ovito.data.DataCollection.create_particle_property` checks if the particle property already exists.
If yes, it automatically copies it in place so you can overwrite its content. Otherwise a fresh :py:class:`~ovito.data.ParticleProperty` instance
is created and added to the output data collection. That means :py:meth:`~ovito.data.DataCollection.create_particle_property`
can be used in both scenarios: to modify an existing particle property or to output a new property. 

Furthermore, there exists a second method, :py:meth:`~ovito.data.DataCollection.create_user_particle_property`,
which is used to create custom particle properties (in contrast to 
:py:attr:`standard properties <ovito.data.ParticleProperty.type>` like color, radius, etc.).

Initialization phase
-----------------------------------

Initialization of parameters and other inputs needed by our custom modifier function should be done outside the function.
For example, our modifier may require reference coordinates of particles, which need to be loaded from an external file. 
One example is the *Displacement vectors* modifier of OVITO, which asks the user to load a reference configuration file with the
coordinates that should be subtracted from the current particle coordinates. A corresponding implementation of this modifier in Python 
would look as follows::

    from ovito.data import ParticleProperty
    from ovito.io import FileSource
    
    reference = FileSource(adjust_animation_interval = False)
    reference.load("simulation.0.dump")
    
    def modify(frame, input, output):
        prop = output.create_particle_property(ParticleProperty.Type.Displacement)
        
        prop.marray[:] = (    input.particle_properties.position.array -
                          reference.particle_properties.position.array)
		
The script above creates a :py:class:`~ovito.io.FileSource` to load the reference particle positions from an external
simulation file. Setting :py:attr:`~ovito.io.FileSource.adjust_animation_interval` to false is required to
prevent OVITO from automatically changing the animation length.

Within the ``modify()`` function we can access the particle coordinates stored in the file source, which is a subclass
of :py:class:`~ovito.data.DataCollection`.

Asynchronous modifiers and progress reporting
-----------------------------------------------

Due to technical limitations the custom modifier function is always executed in the main thread of the application. 
This is in contrast to the built-in asynchronous modifiers of OVITO, which are implemented in C++. 
They are executed in a background thread to not block the graphical user interface during long-running operations.

That means, if our Python modifier function takes a long time to compute before returning control to OVITO, no input events 
can be processed by the application and the user interface will freeze. To avoid this, you can make your modifier function asynchronous using 
the ``yield`` Python statement (see the `Python docs <https://docs.python.org/3/reference/expressions.html#yieldexpr>`_ for more information). 
Calling ``yield`` within the modifier function temporarily yields control to the
main program, giving it the chance to process waiting user input events or repaint the viewports::

   def modify(frame, input, output):   
       for i in range(input.number_of_particles):
           # Perform a small computation step
           ...
           # Temporarily yield control to the system
           yield
           
In general, ``yield`` should be called periodically and as frequently as possible, for example after processing one particle from the input as 
in the code above. 

The ``yield`` keyword also gives the user (and the system) the possibility to cancel the execution of the custom
modifier function. When the evaluation of the modification pipeline is interrupted by the system, the ``yield`` statement does not return 
and the Python function execution is discontinued.

Finally, the ``yield`` mechanism gives the custom modifier function the possibility to report its progress back to the system.
The progress must be reported as a fraction in the range 0.0 to 1.0 using the ``yield`` statement. For example::

   def modify(frame, input, output):
       total_count = input.number_of_particles   
       for i in range(0, total_count):
           ...
           yield (i/total_count)

The current progress value will be displayed in the status bar by OVITO.
Moreover, a string describing the current status can be yielded, which will also be displayed in the status bar::

   def modify(frame, input, output):
       yield "Performing an expensive analysis..."
       ...

Setting display parameters
-----------------------------------

Many data objects such as the :py:class:`~ovito.data.Bonds` or :py:class:`~ovito.data.SimulationCell` object are associated with
a corresponding :py:class:`~ovito.vis.Display` object, which is responsible for rendering (visualizing) the data in the viewports.
The necessary :py:class:`~ovito.vis.Display` object is created automatically when the data object is created and is attached to it by OVITO. 
It can be accessed through the :py:attr:`~ovito.data.DataObject.display` attribute of the :py:class:`~ovito.data.DataObject` base class. 

If the script modifier function injects a new data objects into the pipeline, it can configure the parameters of the attached display object.
In the following example, the parameters of the :py:class:`~ovito.vis.BondsDisplay` are being initialized::

   def modify(frame, input, output):
   
       # Create a new bonds data object.
       bonds = ovito.data.Bonds()
       output.add(bonds)
       ...
       
       # Configure visual appearance of bonds.
       bonds.display.color = (1.0, 1.0, 1.0)
       bonds.display.use_particle_colors = False
       bonds.display.width = 0.4
       
However, every time our modifier function is executed, it will create a new :py:class:`~ovito.data.Bonds` object together with a 
new :py:class:`~ovito.vis.BondsDisplay` instance. If the modifier is used in an interactive OVITO session, this will lead to unexpected behavior 
when the user tries to change the display settings.
All parameter changes made by the user will get lost as soon as the modification pipeline is re-evaluated. To mitigate the problem, it is a good idea to 
create the :py:class:`~ovito.vis.BondsDisplay` just once outside the modifier function and then attach it to the :py:class:`~ovito.data.Bonds`
object created by the modifier function::

   bonds_display = BondsDisplay(color=(1,0,0), use_particle_colors=False, width=0.4)
   
   def modify(frame, input, output):   
       bonds = ovito.data.Bonds(display = bonds_display)
       output.add(bonds)

       