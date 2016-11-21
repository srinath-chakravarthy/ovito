# Load dependencies
import ovito.io

# Load the native code module
import POVRay

# Register export formats.
ovito.io.export_file._formatTable["povray"] = POVRay.POVRayExporter
