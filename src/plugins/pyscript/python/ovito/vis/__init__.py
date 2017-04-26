"""
This module contains classes related to data visualization and rendering.

**Rendering:**

  * :py:class:`RenderSettings`
  * :py:class:`Viewport`
  * :py:class:`ViewportConfiguration`

**Render engines:**

  * :py:class:`OpenGLRenderer`
  * :py:class:`TachyonRenderer`
  * :py:class:`POVRayRenderer`

**Data visualization:**

  * :py:class:`Display` (base class of all display classes below)
  * :py:class:`BondsDisplay`
  * :py:class:`DislocationDisplay`
  * :py:class:`ParticleDisplay`
  * :py:class:`SimulationCellDisplay`
  * :py:class:`SurfaceMeshDisplay`
  * :py:class:`TrajectoryDisplay`
  * :py:class:`VectorDisplay`

**Viewport overlays:**

  * :py:class:`CoordinateTripodOverlay`
  * :py:class:`PythonViewportOverlay`
  * :py:class:`TextLabelOverlay`

"""

import sip
import PyQt5.QtGui
import ovito

# Load the native modules.
from ..plugins.PyScript.Rendering import *
from ..plugins.PyScript.Viewport import *

__all__ = ['RenderSettings', 'Viewport', 'ViewportConfiguration', 'OpenGLRenderer', 'Display',
        'CoordinateTripodOverlay', 'PythonViewportOverlay', 'TextLabelOverlay']

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
        
        The rendered image of movie will automatically be saved to disk when the :py:attr:`RenderSettings.filename` attribute contains a non-empty string.
        Alternatively, you can save the returned `QImage <http://pyqt.sourceforge.net/Docs/PyQt5/api/qimage.html>`_ explicitly using its ``save()`` method. 
        This gives you the opportunity to paint additional graphics on top of the image before saving it. For example:
        
        .. literalinclude:: ../example_snippets/render_to_image.py
    """
    if settings is None:
        settings = self.dataset.render_settings
    elif isinstance(settings, dict):
        settings = RenderSettings(settings)
    if len(self.dataset.scene_nodes) == 0:
        print("Warning: The scene to be rendered is empty. Did you forget to add a node to the scene by calling ObjectNode.add_to_scene()?")
    if ovito.gui_mode:
        # Use the frame buffer of the GUI window for rendering.
        fb_window = self.dataset.container.window.frame_buffer_window
        fb = fb_window.create_frame_buffer(settings.outputImageWidth, settings.outputImageHeight)
        fb_window.show_and_activate()
    else:
        # Create a temporary off-screen frame buffer.
        fb = FrameBuffer(settings.size[0], settings.size[1])
    if not self.dataset.render_scene(settings, self, fb, ovito.task_manager):
        return None
    return fb.image
Viewport.render = _Viewport_render

def _ViewportConfiguration__len__(self):
    return len(self.viewports)
ViewportConfiguration.__len__ = _ViewportConfiguration__len__

def _ViewportConfiguration__iter__(self):
    return self.viewports.__iter__()
ViewportConfiguration.__iter__ = _ViewportConfiguration__iter__

def _ViewportConfiguration__getitem__(self, index):
    return self.viewports[index]
ViewportConfiguration.__getitem__ = _ViewportConfiguration__getitem__
