# Load dependencies
import ovito.modifiers
import ovito.modifiers.particles

# Load the native code modules.
import CrystalAnalysis

# Inject modifier classes into parent module.
ovito.modifiers.ConstructSurfaceModifier = CrystalAnalysis.ConstructSurfaceModifier
ovito.modifiers.SmoothDislocationsModifier = CrystalAnalysis.SmoothDislocationsModifier
ovito.modifiers.SmoothSurfaceModifier = CrystalAnalysis.SmoothSurfaceModifier
ovito.modifiers.DislocationAnalysisModifier = CrystalAnalysis.DislocationAnalysisModifier
ovito.modifiers.ElasticStrainModifier = CrystalAnalysis.ElasticStrainModifier

# Copy enum list.
ovito.modifiers.ElasticStrainModifier.Lattice = ovito.modifiers.DislocationAnalysisModifier.Lattice
