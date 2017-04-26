# Load dependencies
import ovito.modifiers
import ovito.modifiers.particles

# Load the native code modules.
import ovito.plugins.Particles
import ovito.plugins.CrystalAnalysis

# Inject modifier classes into parent module.
ovito.modifiers.ConstructSurfaceModifier = ovito.plugins.CrystalAnalysis.ConstructSurfaceModifier
ovito.modifiers.SmoothDislocationsModifier = ovito.plugins.CrystalAnalysis.SmoothDislocationsModifier
ovito.modifiers.SmoothSurfaceModifier = ovito.plugins.CrystalAnalysis.SmoothSurfaceModifier
ovito.modifiers.DislocationAnalysisModifier = ovito.plugins.CrystalAnalysis.DislocationAnalysisModifier
ovito.modifiers.ElasticStrainModifier = ovito.plugins.CrystalAnalysis.ElasticStrainModifier
#ovito.modifiers.GrainSegmentationModifier = ovito.plugins.CrystalAnalysis.GrainSegmentationModifier
ovito.modifiers.__all__ += ['ConstructSurfaceModifier', 'SmoothDislocationsModifier', 'SmoothSurfaceModifier', 'DislocationAnalysisModifier',
            'ElasticStrainModifier']
#ovito.modifiers.__all__ += ['GrainSegmentationModifier']

# Copy enum list.
ovito.modifiers.ElasticStrainModifier.Lattice = ovito.modifiers.DislocationAnalysisModifier.Lattice
#ovito.modifiers.GrainSegmentationModifier.Lattice = ovito.modifiers.DislocationAnalysisModifier.Lattice
