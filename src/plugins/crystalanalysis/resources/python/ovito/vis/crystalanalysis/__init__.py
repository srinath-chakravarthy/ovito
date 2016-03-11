# Load dependencies
import ovito.vis

# Load the native code modules.
import Particles
import ParticlesImporter
import ParticlesExporter
import CrystalAnalysis

# Inject selected classes into parent module.
ovito.vis.DislocationDisplay = CrystalAnalysis.DislocationDisplay
ovito.vis.PartitionMeshDisplay = CrystalAnalysis.PartitionMeshDisplay

# Inject enum types.
ovito.vis.DislocationDisplay.Shading = ovito.vis.ArrowShadingMode