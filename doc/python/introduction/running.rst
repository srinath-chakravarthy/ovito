==================================
Running scripts
==================================

OVITO's scripting interface serves two main purposes: It allows you to automate tasks (i.e. programmatically execute program functions) and to extend the 
program (e.g. adding new analysis functions). How you write a script and how you execute it depends on the intended purpose.
The following list gives an overview of the various ways scripts are being used within OVITO:

 1. **Programmatically execute program functions**: Scripts can invoke program actions like the human user does using the graphical interface.
    This requires writing a Python script file containing the commands using an external text editor. The script file is executed by choosing
    *Run Script File* from the *Scripting* menu of OVITO. The script can automatically add modifiers to the current dataset and configure them, for example. 
    Or it may access the results of the current modification pipeline and write the data to an output file in a custom format.
 
 2. **Batch-processing**: Batch-processing scripts also contain instructions to invoke program functions. However, they are meant to be run from the command line
    without any user interaction. A batch-processing script is therefore responsible for performing all necessary actions: loading the simulation data first, 
    optionally processing it using OVITO's modifier framework,
    and finally exporting or rendering the results. Batch scripts are run using the :program:`ovitos` script interpreter, which will be introduced
    in the next section. This allows you to leverage OVITO's file I/O and data analysis functions in a fully automated manner, for example to process
    a large number of simulation files on a remote computing cluster and perform complex analyses.
 
 3. **User-defined data modifiers**: OVITO's scripting framework also gives you the possibility to develop new types of modifiers, which can manipulate 
    or analyze simulation data in ways that are not covered by any of the built-in standard modifiers provided by the program. So-called *Python script modifiers* 
    (see :ref:`writing_custom_modifiers` section) participate in the data pipeline system of OVITO and behave like the built-in modifiers. A *Python script modifier* essentially consists
    of a single Python script function named ``modify()``, which you define. It is executed automatically by the system whenever the data pipeline is evaluated.
    This is in contrast to the command scripts described above, which are executed only once and explicitly by the user. 
    
 4. **User-defined viewport overlays**: A `Python script overlay <../../viewport_overlays.python_script.html>`_ is a user-defined script function that gets called by OVITO every time 
    a viewport is repainted or an image is rendered. This allows you to amend or enrich images or movies rendered by OVITO with custom graphics or text, e.g., to
    include additional information like a scale bar.
    
Note that a *Python script modifiers* is meant to be used from within the graphical user interface, but under certain circumstances it may also make sense
to define one from within a non-interactive script (see :py:class:`~ovito.modifiers.PythonScriptModifier` class).

OVITO's Python interpreter
----------------------------------

OVITO includes a built-in script interpreter, which can execute programs written in the Python language.
The current version of OVITO is compatible with the `Python 3.4 <https://docs.python.org/3.4/>`_ language standard. 
You typically execute a Python script from the terminal using the :program:`ovitos` script launcher that comes with OVITO:

.. code-block:: shell-session

	ovitos [-o file] [-g] [script.py] [args...]
	
The :program:`ovitos` program is located in the :file:`bin/` subdirectory of OVITO for Linux, in the 
:file:`Ovito.app/Contents/MacOS/` directory of OVITO for MacOS, and in the main application directory 
on Windows systems. It should not be confused with :program:`ovito`, the main program which
provides the graphical user interface.

Let's assume we've used a text editor to write a simple Python script file named :file:`hello.py`::

	import ovito
	print("Hello, this is OVITO %i.%i.%i" % ovito.version)

We can execute the script from a Linux terminal as follows:

.. code-block:: shell-session

	me@linux:~/ovito-2.7.1-x86_64/bin$ ./ovitos hello.py
	Hello, this is OVITO 2.7.1
	
By default, the :program:`ovitos` script launcher invokes OVITO in console mode, which is a non-graphical mode
where the main window isn't shown. This allows running OVITO scripts on remote machines or
computing clusters that don't possess a graphics display. In OVITO's console mode, scripts can read from and write
to the terminal as if they were executed by a standard Python interpreter. Any command line arguments following the 
script's name are passed to the script via the ``sys.argv`` variable. Furthermore, it is possible to start OVITO's 
interpreter in interactive scripting mode by running :program:`ovitos` without any arguments.

Preloading program state
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The :command:`-o` command line option loads an OVITO state file before executing the
script. This allows you to preload and use an existing visualization setup that has 
been manually prepared using the graphical version of OVITO and saved to a :file:`.ovito` file. This can save you programming
work, because modifiers, parameters, and the camera setup already get loaded from the OVITO file and 
don't need to be set up programatically in the script anymore.

Running scripts in graphical mode
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The :command:`-g` command line option switches from console mode to graphical mode. This displays OVITO's main window
and you can follow your script's actions as they are being executed. This is useful, for instance, if you want to visually 
inspect the results of your script during the development phase.

Using third-party Python modules from OVITO scripts
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

It is not possible to run scripts written for OVITO with a standard Python interpreter (usually named :program:`python`). 
They must be executed with the launcher :program:`ovitos`. However, the Python installation shipping with OVITO
includes only `NumPy <http://www.numpy.org/>`_ and `matplotlib <http://matplotlib.org/>`_ as non-standard extension modules. 

If you want to use other third-party Python modules in your OVITO scripts, it might be possible to install them in the 
built-in interpreter using the normal *setuptools* or *pip* mechanisms. 
(Use :program:`ovitos` instead of :program:`python` to execute the *setup.py* installation script or run 
:command:`ovitos -m pip install <package>`).

Installing Python extension that include native code (e.g. `Scipy <http:://www.scipy.org>`_) in the interpreter shipping with OVITO is currently not possible.
In this case it is recommended to build OVITO from source on your system. OVITO will then use the system's standard Python interpreter instead of its own.
This will make all modules that are installed in the system Python interpreter accessible within OVITO as well. (Note that you still need
to execute OVITO scripts with the :program:`ovitos` launcher.) How to build OVITO from source is described `on this page <http://www.ovito.org/manual/development.html>`_.

Number of parallel threads
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

OVITO uses all available processor cores on a machine to perform computations by default. To restrict OVITO
to a certain number of parallel threads, use the :command:`-nt` command line parameter, e.g. :command:`ovitos -nt 1 myscript.py`.
	
Further uses of Python scripts within OVITO
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In addition to non-interactive scripts that execute program commands and automate tasks, OVITO provides two more uses of the built-in script interpreter:
You can :ref:`write your own modifier function <writing_custom_modifiers>` using Python, which can then also be used within the graphical program like the 
standard modifiers. Or you can write a `custom viewport overlay <../../viewport_overlays.python_script.html>`_, which is a script function
that can draw arbitrary graphical content into an image or movie rendered by OVITO.
