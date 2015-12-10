"""
This module contains classes related to data visualization and rendering.

**Rendering:**

  * :py:class:`RenderSettings`
  * :py:class:`Viewport`
  * :py:class:`ViewportConfiguration`

**Render engines:**

  * :py:class:`OpenGLRenderer`
  * :py:class:`TachyonRenderer`

**Data visualization:**

  * :py:class:`Display` (base class of all display classes below)
  * :py:class:`BondsDisplay`
  * :py:class:`DislocationDisplay`
  * :py:class:`ParticleDisplay`
  * :py:class:`SimulationCellDisplay`
  * :py:class:`SurfaceMeshDisplay`
  * :py:class:`VectorDisplay`

**Viewport overlays:**

  * :py:class:`CoordinateTripodOverlay`
  * :py:class:`PythonViewportOverlay`

"""

import sip
import PyQt5.QtGui

# Load the native modules.
from PyScriptRendering import *
from PyScriptViewport import *

import ovito

def _get_RenderSettings_custom_range(self):
    """ 
    Specifies the range of animation frames to render if :py:attr:`.range` is ``CUSTOM_INTERVAL``.
    
    :Default: ``(0,100)`` 
    """
    return (self.customRangeStart, self.customRangeEnd)
def _set_RenderSettings_custom_range(self, range):
    if len(range) != 2: raise TypeError("Tuple or list of length two expected.")
    self.customRangeStart = range[0]
    self.customRangeEnd = range[1]
RenderSettings.custom_range = property(_get_RenderSettings_custom_range, _set_RenderSettings_custom_range)

def _get_RenderSettings_size(self):
    """ 
    The size of the image or movie to be generated in pixels. 
    
    :Default: ``(640,480)`` 
    """
    return (self.outputImageWidth, self.outputImageHeight)
def _set_RenderSettings_size(self, size):
    if len(size) != 2: raise TypeError("Tuple or list of length two expected.")
    self.outputImageWidth = size[0]
    self.outputImageHeight = size[1]
RenderSettings.size = property(_get_RenderSettings_size, _set_RenderSettings_size)

def _get_RenderSettings_filename(self):
    """ 
    A string specifying the file path under which the rendered image or movie should be saved.
    
    :Default: ``None``
    """
    if self.saveToFile and self.imageFilename: return self.imageFilename
    else: return None
def _set_RenderSettings_filename(self, filename):
    if filename:
        self.imageFilename = filename
        self.saveToFile = True
    else:
        self.saveToFile = False
RenderSettings.filename = property(_get_RenderSettings_filename, _set_RenderSettings_filename)

# Implement FrameBuffer.image property (requires conversion to SIP).
def _get_FrameBuffer_image(self):
    return PyQt5.QtGui.QImage(sip.wrapinstance(self._image, PyQt5.QtGui.QImage))
FrameBuffer.image = property(_get_FrameBuffer_image)

def _Viewport_render(self, settings = None):
    """ Renders an image or movie of the viewport's view.
    
        :param settings: A settings object, which specifies the resolution, background color, output filename etc. of the image to be rendered. 
                         If ``None``, the global settings are used (given by :py:attr:`DataSet.render_settings <ovito.DataSet.render_settings>`).
        :type settings: :py:class:`RenderSettings`
        :returns: A `QImage <http://pyqt.sourceforge.net/Docs/PyQt5/api/qimage.html>`_ object on success, which contains the rendered picture; 
                  ``None`` if the rendering operation has been canceled by the user.
        
        The rendered image of movie will automatically be saved to disk when the :py:attr:`RenderSettings.filename` attribute has been set to a non-empty string.
        Alternatively, the returned `QImage <http://pyqt.sourceforge.net/Docs/PyQt5/api/qimage.html>`_ object can be saved explicitly from the script. 
        You may even paint additional graphics on top of the image before saving it. For example:
        
        .. literalinclude:: ../example_snippets/render_to_image.py
    """
    if settings is None:
        settings = self.dataset.render_settings
    elif isinstance(settings, dict):
        settings = RenderSettings(settings)
    if ovito.gui_mode:
        if not self.dataset.renderScene(settings, self):
            return None
        return self.dataset.window.frame_buffer_window.frame_buffer.image
    else:
        fb = FrameBuffer(settings.size[0], settings.size[1])
        if not self.dataset.renderScene(settings, self, fb):
            return None
        return fb.image
Viewport.render = _Viewport_render


# Implement the 'overlays' property of the Viewport class. 
def _get_Viewport_overlays(self):
    """ A list-like sequence of viewport overlay objects that are attached to this viewport.
        See the :py:class:`CoordinateTripodOverlay` and :py:class:`PythonViewportOverlay` classes
        for more information.
    """
    
    def ViewportOverlayList__delitem__(self, index):
        if index < 0: index += len(self)
        if index >= len(self): raise IndexError("List index is out of range.")
        self.__vp.removeOverlay(index)

    def ViewportOverlayList__setitem__(self, index, overlay):
        if index < 0: index += len(self)
        if index >= len(self): raise IndexError("List index is out of range.")
        self.__vp.removeOverlay(index)
        self.__vp.insertOverlay(index, overlay)

    def ViewportOverlayList_append(self, overlay):
        self.__vp.insertOverlay(len(self), overlay)
            
    def ViewportOverlayList_insert(self, index, overlay):
        if index < 0: index += len(self)        
        self.__vp.insertOverlay(index, overlay)

    overlay_list = self._overlays
    overlay_list.__vp = self
    type(overlay_list).__delitem__ = ViewportOverlayList__delitem__
    type(overlay_list).__setitem__ = ViewportOverlayList__setitem__
    type(overlay_list).append = ViewportOverlayList_append
    type(overlay_list).insert = ViewportOverlayList_insert
    return overlay_list
Viewport.overlays = property(_get_Viewport_overlays)

def _ViewportConfiguration__len__(self):
    return len(self.viewports)
ViewportConfiguration.__len__ = _ViewportConfiguration__len__

def _ViewportConfiguration__iter__(self):
    return self.viewports.__iter__()
ViewportConfiguration.__iter__ = _ViewportConfiguration__iter__

def _ViewportConfiguration__getitem__(self, index):
    return self.viewports[index]
ViewportConfiguration.__getitem__ = _ViewportConfiguration__getitem__
