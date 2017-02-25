# Load dependencies
import ovito.vis

# Load the native code module
import ovito.plugins.POVRay

# Inject POVRayRenderer class into parent module.
ovito.vis.POVRayRenderer = ovito.plugins.POVRay.POVRayRenderer
ovito.vis.__all__ += ['POVRayRenderer']