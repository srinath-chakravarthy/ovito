# Load dependencies
import ovito.vis

# Load the native code module
import ovito.plugins.Tachyon

# Inject TachyonRenderer class into parent module.
ovito.vis.TachyonRenderer = ovito.plugins.Tachyon.TachyonRenderer
ovito.vis.__all__ += ['TachyonRenderer']
