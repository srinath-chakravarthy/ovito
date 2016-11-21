# Load dependencies
import ovito.vis

# Load the native code module
import POVRay

# Inject POVRayRenderer class into parent module.
ovito.vis.POVRayRenderer = POVRay.POVRayRenderer