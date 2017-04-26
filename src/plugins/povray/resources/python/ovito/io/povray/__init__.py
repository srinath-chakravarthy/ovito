# Load dependencies
import ovito.io

# Load the native code module
import ovito.plugins.POVRay

# Register export formats.
ovito.io.export_file._formatTable["povray"] = ovito.plugins.POVRay.POVRayExporter
