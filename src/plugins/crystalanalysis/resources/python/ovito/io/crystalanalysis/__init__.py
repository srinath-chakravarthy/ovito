# Load dependencies
import ovito.io
import ovito.io.particles

# Load the native code modules
import CrystalAnalysis

# Register export formats.
ovito.io.export_file._formatTable["ca"] = CrystalAnalysis.CAExporter
