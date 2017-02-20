# Load dependencies
import ovito.io
import ovito.io.particles

# Load the native code modules
import ovito.plugins.Particles.Importers
import ovito.plugins.CrystalAnalysis

# Register export formats.
ovito.io.export_file._formatTable["ca"] = ovito.plugins.CrystalAnalysis.CAExporter
