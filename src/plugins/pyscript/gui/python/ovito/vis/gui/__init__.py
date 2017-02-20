# Load dependencies
import ovito
import ovito.vis

# Load the native modules.
import ovito.plugins.PyScript
import ovito.plugins.PyScriptGui

# Inject selected classes into parent module.
ovito.vis.OpenGLRenderer = ovito.plugins.PyScriptGui.OpenGLRenderer