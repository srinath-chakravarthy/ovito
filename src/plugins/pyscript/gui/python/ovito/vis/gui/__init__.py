# Load dependencies
import ovito
import ovito.vis

# Load the native modules.
import PyScriptGUI

# Inject selected classes into parent module.
ovito.vis.OpenGLRenderer = PyScriptGUI.OpenGLRenderer