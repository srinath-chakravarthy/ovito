==================================
Running scripts
==================================

OVITO's scripting interface serves two main purposes: It allows you to automate visualization and analysis tasks and to extend the 
program (e.g. adding new analysis functions). How you write a script and the way you run it depend on the intended purpose.
The following list gives an overview of the various ways scripts are being used within OVITO:

 1. **Programmatically execute program functions**: Scripts can invoke program actions like the human user does using the graphical interface.
    This requires writing a Python script file containing the commands using an external text editor. The script file is executed by choosing
    *Run Script File* from the *Scripting* menu of OVITO. The script can automatically add modifiers to the current dataset and configure them, for example. 
    Or it may access the results of the current modification pipeline and write the data to an output file in a custom format.
 
 2. **Batch-processing**: Batch-processing scripts also contain instructions to invoke program functions. However, they are meant to be run from the command line
    without any user interaction. A batch-processing script is therefore responsible for performing all necessary actions: loading the simulation data first, 
    optionally processing it using OVITO's modifier framework,
    and finally exporting or rendering the results. Batch scripts are typically run using the :program:`ovitos` script interpreter, which will be introduced
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
to define one in a non-interactive script (see :py:class:`~ovito.modifiers.PythonScriptModifier` class).

OVITO's Python interpreter
----------------------------------

OVITO includes an embedded script interpreter, which can execute programs written in the Python language.
The current version of OVITO is compatible with the `Python 3.4 <https://docs.python.org/3.4/>`_ language standard. 
You typically execute a Python script from the terminal using the :program:`ovitos` script interpreter that comes with OVITO:

.. code-block:: shell-session

	ovitos [-o file] [-g] [script.py] [args...]
	
The :program:`ovitos` program is located in the :file:`bin/` subdirectory of OVITO for Linux, in the 
:file:`Ovito.app/Contents/MacOS/` directory of OVITO for MacOS, and in the main application directory 
on Windows systems. It should not be confused with :program:`ovito`, the main program which
provides the graphical user interface.

Let's assume we've used a text editor to write a simple Python script file named :file:`hello.py`::

	import ovito
	print("Hello, this is OVITO %i.%i.%i" % ovito.version)

We can then execute the script from a Linux terminal as follows:

.. code-block:: shell-session

	me@linux:~/ovito-2.8.2-x86_64/bin$ ./ovitos hello.py
	Hello, this is OVITO 2.8.2
	
By default, the :program:`ovitos` script interpreter runs in the non-graphical console mode, where the main window isn't shown. 
This allows running OVITO scripts on remote machines or computing clusters that don't possess a graphics display. 
The :program:`ovitos` program behaves like a standard Python interpreter. Any command line arguments following the 
script's name are passed to the script via the ``sys.argv`` variable. Furthermore, it is possible to start 
an interactive interpreter session by running :program:`ovitos` without any arguments.

Preloading program state
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The :command:`-o` command line option loads an OVITO state file before executing the
script. This allows you to preload and use an existing visualization setup that has 
previously been prepared using the graphical version of OVITO and saved to a :file:`.ovito` file. This can save you programming
work, because modifiers, parameters, and the camera setup already get loaded from the OVITO state file and 
don't need to be set up programatically in the script anymore.

Running scripts in graphical mode
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The :command:`-g` command line option switches from console mode to graphical mode. This shows OVITO's main window
and you can follow your script's actions as they are being executed in the user interface. This is useful, for instance, if you want to visually 
inspect the results of your script during the development phase.

Number of parallel threads
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

OVITO uses all available processor cores on a machine to perform computations by default. To restrict OVITO
to a certain number of parallel threads, use the :command:`-nt` command line parameter, e.g. :command:`ovitos -nt 1 myscript.py`.

Third-party Python modules
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The embedded script interpreter of OVITO is a preconfigured version of the standard CPython interpreter, which integrates the
:py:mod:`ovito` Python package. This makes it possible to run scripts both within the graphical program OVITO as well as through the :program:`ovitos`
command line interpreter. However, this embedded interpreter shipped with OVITO includes only the `NumPy <http://www.numpy.org/>`_, `matplotlib <http://matplotlib.org/>`_, 
and `PyQt5 <http://pyqt.sourceforge.net/Docs/PyQt5/>`_ packages as preinstalled extension modules.

If you want to use other third-party Python modules from your OVITO scripts, it may be possible to install them in the 
:program:`ovitos` interpreter using the normal *pip* or *setuptools* mechanisms 
(e.g., run :command:`ovitos -m pip install <package>` to install a module via *pip*).

Installing Python extensions that include native code (e.g. `Scipy <http:://www.scipy.org>`_) in the embedded interpreter 
will likely fail. In this case it is recommended to build OVITO from source on your local system. 
The graphical program and :program:`ovitos` will then both make use of your system's standard Python interpreter.
This will make all modules that are installed in your Python interpreter accessible within OVITO and :program:`ovitos` as well.
How to build OVITO from source is described `on this page <http://www.ovito.org/manual/development.html>`_.

Using the ovito package from other Python interpreters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The :py:mod:`ovito` Python package can also be imported by scripts running in an external Python interpreter. 
However, because this module contains native extensions, it must be compiled specifically for that Python interpreter. 
Since there is a chance that the precompiled version of the module shipping with the binary OVITO installation is not compatible 
with your local Python interpreter, it might thus be necessary to `build OVITO from source <http://www.ovito.org/manual/development.html>`_.
Make sure you link against the Python interpreter with which you are going to run your scripts.

Once the graphical program and the :py:mod:`ovito` Python extension module have been built, you can make the module loadable from your 
Python interpreter by adding the following directory to the `PYTHONPATH <https://docs.python.org/3/using/cmdline.html#envvar-PYTHONPATH>`_:

=============== ===========================================================
Platform:        Location of ovito package relative to build path:
=============== ===========================================================
Windows         :file:`plugins/python/`
Linux           :file:`lib/ovito/plugins/python/`
macOS           :file:`Ovito.app/Contents/Resources/python/`
=============== ===========================================================

Further uses of Python scripts within OVITO
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In addition to non-interactive scripts that execute program commands and automate tasks, OVITO provides two more uses of the built-in Python interpreter:
You can :ref:`write your own modifier function <writing_custom_modifiers>`, which can be used within the graphical program just like the 
standard modifiers. Or you can write a `custom viewport overlay <../../viewport_overlays.python_script.html>`_, which is a script 
that draws arbitrary graphical content into an image or movie rendered by OVITO.
