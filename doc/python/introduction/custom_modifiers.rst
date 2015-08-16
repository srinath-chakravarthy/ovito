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

-----------------------------------
Inserting a custom modifier
-----------------------------------

Creating a custom modifier essentially requires writing a Python function with the following signature::

  def modify(frame, input, output):
      ...

The meaning of the parameters and the implementation of this function will be described 
in later sections. You can insert a custom modifier into the modification pipeline either using 
OVITO's graphical user interface or programmatically:

  1. Within the graphical user interface, select *Python script* from the modifier drop-down list to insert
     a Python script modifier into the modification pipeline. OVITO provides a text input field
     which allows you to enter the code for the ``modify()`` function. The 
     `corresponding page <../../particles.modifiers.python_script.html>`_ in the
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
Python modifier function
-----------------------------------

The custom modifier function defined above is called by OVITO every time the modification pipeline
is evaluated. It receives the data produced by the upstream part of the pipeline (e.g. a particle set
loaded by a :py:class:`~ovito.io.FileSource` and further processed by other modifiers that 
precede the custom modifier in the pipeline). Our Python modifier function has the possibility to modify or extend
the data as needed. After the user-defined function returns, the output continuous down the pipeline (i.e. is handed over to the next
modifier in the pipeline), and, eventually, the final results are stored in the :py:attr:`~ovito.ObjectNode.output` cache 
of the :py:class:`~ovito.ObjectNode` and displayed in the viewports.

