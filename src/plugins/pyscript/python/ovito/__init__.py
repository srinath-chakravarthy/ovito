import os.path
import sys
import pkgutil
import importlib
try:
    # Python 3.x
    import collections.abc as collections
except ImportError:
    # Python 2.x
    import collections

# Allow OVITO plugins and C++ extension modules to be spread over several installation directories.
_package_source_path = __path__ # Make a copy of the original path, which will be used below to automatically import all submodules.
__path__ = pkgutil.extend_path(__path__, __name__)

# Load the native module with the core bindings
import ovito.plugins.PyScript
from .plugins.PyScript import (version, version_string, gui_mode, headless_mode, dataset, task_manager)
from .plugins.PyScript.App import (DataSet)

# Load sub-modules (in the right order because there are dependencies among them)
import ovito.anim
import ovito.data
import ovito.vis
import ovito.io
import ovito.modifiers

# Load the bindings for the GUI classes when running in gui mode.
if ovito.gui_mode:
    import ovito.plugins.PyScriptGui

from .plugins.PyScript.Scene import (ObjectNode, SceneRoot, PipelineObject, PipelineStatus)

__all__ = ['version', 'version_string', 'gui_mode', 'headless_mode', 'dataset', 'task_manager',
    'DataSet', 'ObjectNode']

# Load the whole OVITO package. This is required to make all Python bindings available.
for _, _name, _ in pkgutil.walk_packages(_package_source_path, __name__ + '.'):
    if _name.startswith("ovito.linalg"): continue  # For backward compatibility with OVITO 2.7.1. The old 'ovito.linalg' module has been deprecated but may still be present in some existing OVITO installations.
    if _name.startswith("ovito.plugins"): continue  # Do not load C++ plugin modules at this point, only Python modules
    try:
        #print("Loading submodule ", _name)
        importlib.import_module(_name)
    except:
        print("Error while loading OVITO submodule %s:" % _name, sys.exc_info()[0])
        raise

def _DataSet_scene_nodes(self):
    """ A list-like object containing the :py:class:`~ovito.ObjectNode` instances that are part of the three-dimensional scene.
        Only nodes which are in this list are visible in the viewports. You can add or remove nodes from this list either by calling
        :py:meth:`ObjectNode.add_to_scene` and :py:meth:`ObjectNode.remove_from_scene` or by using the standard Python ``append()`` and ``del`` statements. 
    """
    return self.scene_root.children
DataSet.scene_nodes = property(_DataSet_scene_nodes)

def _get_DataSet_selected_node(self):
    """ The :py:class:`~ovito.ObjectNode` that is currently selected in OVITO's graphical user interface, 
        or ``None`` if no node is selected. """
    return self.selection.nodes[0] if self.selection.nodes else None
def _set_DataSet_selected_node(self, node):
    """ Sets the scene node that is currently selected in OVITO. """
    if node: self.selection.nodes = [node]
    else: del self.selection.nodes[:]
DataSet.selected_node = property(_get_DataSet_selected_node, _set_DataSet_selected_node)

# Implement the 'modifiers' property of the ObjectNode class, which provides access to modifiers in the pipeline. 
def _get_ObjectNode_modifiers(self):
    """The node's modification pipeline.
    
       This list contains the modifiers that are applied to the input data provided by the node's :py:attr:`.source` object. You
       can add and remove modifiers from this list as needed. The first modifier in the list is
       always evaluated first, and its output is passed on to the second modifier and so on. 
       The results of the last modifier are displayed in the viewports and can be access through the 
       :py:attr:`.output` field. 
       
       Example::
       
           node.modifiers.append(WrapPeriodicImagesModifier())
    """    
    
    class ObjectNodeModifierList(collections.MutableSequence):
        """ This helper class emulates a mutable sequence of modifiers. """
        def __init__(self, node): 
            self.node = node
        def _pipelineObjectList(self):
            """ This internal helper function builds a list of PipelineObjects in the node's pipeline. """
            polist = []
            obj = self.node.data_provider
            while isinstance(obj, PipelineObject):
                polist.insert(0, obj)
                obj = obj.source_object
            return polist
        def _modifierList(self):
            """ This internal helper function builds a list containing all modifiers in the node's pipeline. """
            mods = []
            for obj in self._pipelineObjectList():
                for app in obj.modifier_applications:
                    mods.append(app.modifier)
            return mods
        def __len__(self):
            """ Returns the total number of modifiers in the node's pipeline. """
            count = 0
            obj = self.node.data_provider
            while isinstance(obj, PipelineObject):
                count += len(obj.modifier_applications)
                obj = obj.source_object
            return count
        def __iter__(self):
            return self._modifierList().__iter__()
        def __getitem__(self, i):
            return self._modifierList()[i]
        def __setitem__(self, index, newMod):
            """ Replaces an existing modifier in the pipeline with a different one. """
            if isinstance(index, slice):
                raise TypeError("This sequence type does not support slicing.")
            if isinstance(index, int):
                if index < 0: index += len(self)
                count = 0
                for obj in self._pipelineObjectList():
                    for i in range(len(obj.modifier_applications)):
                        if count == index:
                            del obj.modifier_applications[i]
                            obj.insert_modifier(i, newMod)
                            return
                        count += 1
                raise IndexError("List index is out of range.")
            else:
                raise TypeError("Expected integer index")
        def __delitem__(self, index):
            if isinstance(index, slice):
                raise TypeError("This sequence type does not support slicing.")
            if isinstance(index, int):
                if index < 0: index += len(self)
                count = 0
                for obj in self._pipelineObjectList():
                    for i in range(len(obj.modifier_applications)):
                        if count == index:
                            del obj.modifier_applications[i]
                            return
                        count += 1
                raise IndexError("List index is out of range.")
            else:
                raise TypeError("Expected integer index")
        def insert(self, index, mod):
            if index < 0: index += len(self)
            count = 0
            for obj in self._pipelineObjectList():
                for i in range(len(obj.modifier_applications)):
                    if count == index:
                        obj.insert_modifier(i, mod)
                        return
                    count += 1
            self.node.apply_modifier(mod)
        def __str__(self):
            return str(self._modifierList())
                    
    return ObjectNodeModifierList(self)
ObjectNode.modifiers = property(_get_ObjectNode_modifiers)

def _ObjectNode_wait(self, signalError = True, time = None):
    # Blocks script execution until the node's modification pipeline is ready.
    #
    #    :param signalError: If ``True``, the function raises an exception when the modification pipeline could not be successfully evaluated.
    #                        This may be the case if the input file could not be loaded, or if one of the modifiers reported an error.   
    #    :returns: ``True`` if the pipeline evaluation is complete, ``False`` if the operation has been canceled by the user.
    #
    if time is None: time = self.dataset.anim.time 
    if not self.wait_until_ready(time):
        return False
    if signalError:
        state = self.eval_pipeline(time)
        if state.status.type == PipelineStatus.Type.Error:
            raise RuntimeError("Data pipeline evaluation failed with the following error: %s" % state.status.text)
    return True
ObjectNode.wait = _ObjectNode_wait

def _ObjectNode_compute(self, frame = None):
    """ Computes and returns the results of the node's modification pipeline.

        This method requests an update of the node's modification pipeline and waits until the effect of all modifiers in the 
        node's modification pipeline has been computed. If the modification pipeline is already up to date, i.e., results are already 
        available in the node's pipeline cache, the method returns immediately.
        
        The optional *frame* parameter lets you control at which animation time the modification pipeline is evaluated. (Animation frames start at 0.) 
        If it is omitted, the current animation position (:py:attr:`AnimationSettings.current_frame <ovito.anim.AnimationSettings.current_frame>`) is used.
        
        Even if you are not interested in the final output of the modification pipeline, you should still call this method in case you are going to 
        directly access information reported by individual modifiers in the pipeline. This method will ensure that all modifiers 
        have been computed and their output fields are up to date.

        This function raises a ``RuntimeError`` when the modification pipeline could not be successfully evaluated for some reason.
        This may happen due to invalid modifier parameters, for example.

        :returns: A reference to the node's internal :py:class:`~ovito.data.DataCollection` containing the output of the modification pipeline.
                  It is also accessible via the :py:attr:`.output` attribute after calling :py:meth:`.compute`.
    """
    if frame is not None:
        time = self.dataset.anim.frame_to_time(frame)
    else:
        time = self.dataset.anim.time

    if not self.wait(time = time):
        raise RuntimeError("Operation has been canceled by the user.")

    state = self.eval_pipeline(time)
    assert(state.status.type != PipelineStatus.Type.Error)
    assert(state.status.type != PipelineStatus.Type.Pending)

    self.__output = ovito.data.DataCollection()
    self.__output.set_data_objects(state)

    # Wait for worker threads to finish.
    # This is to avoid warning messages 'QThreadStorage: Thread exited after QThreadStorage destroyed'
    # during Python program exit.
    import PyQt5.QtCore
    PyQt5.QtCore.QThreadPool.globalInstance().waitForDone(0)

    return self.__output

ObjectNode.compute = _ObjectNode_compute

def _ObjectNode_output(self):
    """ Provides access to the last results computed by the node's data modification pipeline.
        
        After calling the :py:meth:`.compute` method, this attribute holds a :py:class:`~ovito.data.DataCollection`
        with the output of the node's modification pipeline.
    """    
    if not hasattr(self, "__output"):
        raise RuntimeError("The node's pipeline has not been evaluated yet. Call compute() first before accessing the pipeline output.")
    return self.__output
ObjectNode.output = property(_ObjectNode_output)

def _ObjectNode_remove_from_scene(self):
    """ Removes the node from the scene by deleting it from the :py:attr:`ovito.DataSet.scene_nodes` list.
        The visual representation of the node will disappear from the viewports after calling this method.
    """
    # Remove node from its parent's list of children
    if self.parent_node is not None:
        del self.parent_node.children[self.parent_node.children.index(self)]
    
    # Automatically unselect node
    if self == self.dataset.selected_node:
        self.dataset.selected_node = None
ObjectNode.remove_from_scene = _ObjectNode_remove_from_scene

def _ObjectNode_add_to_scene(self):
    """ Inserts the node into the current scene by appending it to the :py:attr:`ovito.DataSet.scene_nodes` list.
        The visual representation of the node will appear in the viewports.
        
        You can remove the node from the scene again by calling :py:meth:`.remove_from_scene`.
    """
    if not self in self.dataset.scene_nodes:
        self.dataset.scene_nodes.append(self)
    
    # Select node
    self.dataset.selected_node = self
ObjectNode.add_to_scene = _ObjectNode_add_to_scene

# Give SceneRoot class a list-like interface.
SceneRoot.__len__ = lambda self: len(self.children)
SceneRoot.__iter__ = lambda self: iter(self.children)
SceneRoot.__getitem__ = lambda self, i: self.children[i]
def _SceneRoot__setitem__(self, index, newNode):
    if index < 0: index += len(self)
    if index < 0 or index >= len(self):
        raise IndexError("List index is out of range.")
    self.removeChild(self.children[index])
    self.insertChild(index, newNode)
SceneRoot.__setitem__ = _SceneRoot__setitem__
def _SceneRoot__delitem__(self, index):
    if isinstance(index, ObjectNode):
        self.removeChild(index)
        return
    if index < 0 or index >= len(self):
        raise IndexError("List index is out of range.")
    self.removeChild(self.children[index])
SceneRoot.__delitem__ = _SceneRoot__delitem__
def _SceneRoot_append(self, node):
    if node.parent_node == self:
        raise RuntimeError("Cannot add the same node more than once to the scene.")
    self.addChild(node)
SceneRoot.append = _SceneRoot_append
def _SceneRoot_insert(self, index, node):
    if index < 0: index += len(self)
    if index < 0 or index >= len(self):
        raise IndexError("List index is out of range.")
    if node.parent_node == self:
        raise RuntimeError("Cannot insert the same node more than once into the scene.")
    self.insertChild(index, node)
SceneRoot.insert = _SceneRoot_insert
