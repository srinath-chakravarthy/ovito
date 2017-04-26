# Load dependencies
import ovito.io.particles

# Load the native code module
import ovito.plugins.NetCDFPlugin

# Inject selected classes into parent module.
ovito.io.particles.NetCDFImporter = ovito.plugins.NetCDFPlugin.NetCDFImporter
