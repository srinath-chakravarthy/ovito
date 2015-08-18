# Load dependencies
import ovito.vis

# Load the native code module
import CrystalAnalysis

# Inject selected classes into parent module.
ovito.vis.DislocationDisplay = CrystalAnalysis.DislocationDisplay

# Inject enum types.
ovito.vis.DislocationDisplay.Shading = ovito.vis.ArrowShadingMode