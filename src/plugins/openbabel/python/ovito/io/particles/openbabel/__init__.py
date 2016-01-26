# Load dependencies
import ovito.io.particles

# Load the native code module
import OpenBabelPlugin

# Inject selected classes into parent module.
ovito.io.particles.OpenBabelImporter = OpenBabelPlugin.OpenBabelImporter
ovito.io.particles.CIFImporter = OpenBabelPlugin.CIFImporter
