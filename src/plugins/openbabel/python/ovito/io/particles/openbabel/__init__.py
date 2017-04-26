# Load dependencies
import ovito.io.particles

# Load the native code module
import ovito.plugins.OpenBabelPlugin

# Inject selected classes into parent module.
ovito.io.particles.OpenBabelImporter = ovito.plugins.OpenBabelPlugin.OpenBabelImporter
ovito.io.particles.CIFImporter = ovito.plugins.OpenBabelPlugin.CIFImporter
