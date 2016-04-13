===================================
File I/O
===================================

This section describes how to load simulation data from external files and how to export modified  
particle data again using a script.

------------------------------------
File input
------------------------------------

The primary way of loading an external data file is the :py:func:`~ovito.io.import_file` function::

   >>> from ovito.io import import_file
   >>> node = import_file("simulation.dump")

This high-level function works like the `Load File` menu function in OVITO's graphical user interface. 
It creates and returns an :py:class:`~ovito.ObjectNode`, whose :py:class:`~ovito.io.FileSource` is set up to point
to the specified file, and which is reponsible for loading the actual data from the file. 

In case you already have an existing object node, for example after a first call to :py:func:`~ovito.io.import_file`, 
you can subsequently load a different simulation file with the :py:meth:`~ovito.io.FileSource.load` method
of the :py:class:`~ovito.io.FileSource` owned by the node::

   >>> node.source.load("other_simulation.dump")

This method takes the same parameters as the :py:func:`~ovito.io.import_file` function, but it doesn't create a new
object node. Any existing modification pipeline of the object node is preserved.

Note that the :py:meth:`~ovito.io.FileSource.load` method is also used to
load reference configurations for modifiers that require reference particle coordinates, e.g.::

   >>> modifier = CalculateDisplacementsModifier()
   >>> modifier.reference.load("reference.dump")

Here the :py:attr:`~ovito.modifiers.CalculateDisplacementsModifier.reference` attribute refers 
to a second :py:class:`~ovito.io.FileSource`, which is owned by the :py:class:`~ovito.modifiers.CalculateDisplacementsModifier` and which is responsible
for loading the reference particle positions required by the modifier.

**Column mapping**

Both the global :py:func:`~ovito.io.import_file` function and the :py:meth:`FileSource.load() <ovito.io.FileSource.load>` method
accept format-specific keyword arguments in addition to the filename. For instance, when loading XYZ
files, the mapping of input file columns to OVITO's particle properties needs to be specified using the ``columns`` keyword::

   >>> node = import_file("simulation.xyz", columns = 
   ...           ["Particle Type", "Position.X", "Position.Y", "Position.Z", "My Property"])
   
The number of entries in the ``columns`` list must match the number of data columns in the input file. 
See the documentation of the :py:func:`~ovito.io.import_file` function for more information on this.

**Simulation sequences**

So far we have only considered loading a single simulation snapshot. As you know from the graphical program version, OVITO is also able to
load a sequence of simulation snapshots (a trajectory), which can be played back as an animation. 
There are two possible scenarios:

1. To load a file that stores multiple simulation frames, use the ``multiple_frames`` keyword::

    >>> node = import_file("sequence.dump", multiple_frames = True)

   OVITO will scan the entire file and discover all contained simulation frames. This works for LAMMPS dump files and XYZ files.

2. To load a series of simulation files from a directory, following a naming pattern like :file:`frame.0.dump`, :file:`frame.1000.dump`,
   :file:`frame.2000.dump`, etc., pass a wildcard pattern to the :py:func:`~ovito.io.import_file` function::

    >>> node = import_file("frame.*.dump")

   OVITO will automatically find all files in the directory belonging to the the simulation trajectory.

In both cases you can check how many animation frames were found by querying the :py:attr:`~ovito.io.FileSource.num_frames` property 
of the :py:class:`~ovito.io.FileSource`::

   >>> node.source.num_frames
   100

.. note::
   
   To save memory and time, OVITO never loads all frames from a trjectory at once. It only scans the directory (or the multi-frame file) 
   to discover all frames belonging to the sequence and adjusts the internal animation length to match the number of input frames found. 
   The actual simulation data will only be loaded by the :py:class:`~ovito.io.FileSource` on demand, e.g., when 
   jumping to a specific frame in the animation or when rendering a movie.
   
You can iterate over the frames of a loaded animation sequence in a script loop::

   # Load a sequence of simulation files 'frame.0.dump', 'frame.1000.dump', etc.
   node = import_file("simulation.*.dump")
   
   for frame in range(node.source.num_frames):
       # Load the input data and apply the modifiers to current frame:
       node.compute(frame)
       # ... access computation results for current animation frame.
       
------------------------------------
File output
------------------------------------

You can write particles to a file using the :py:func:`ovito.io.export_file` function::

    >>> export_file(node, "outputfile.dump", "lammps_dump",
    ...    columns = ["Position.X", "Position.Y", "Position.Z", "My Property"])

OVITO will automatically evaluate the node's modification pipeline and export the computed results to the file.
If the node's modification pipeline contains no modifiers, then the original, unmodified data
will be exported. 

The second function parameter specifies the output filename, and the third parameter selects the 
output format. For a list of supported file formats, see :py:func:`~ovito.io.export_file` documentation.
Depending on the selected output format, additional keyword arguments must be specified. For instance,
in the example above the ``columns`` parameter lists the particle properties to be exported.
