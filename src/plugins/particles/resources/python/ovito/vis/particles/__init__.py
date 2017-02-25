# Load dependencies
import ovito.vis

# Load the native code module
import ovito.plugins.Particles

# Inject selected classes into parent module.
ovito.vis.SimulationCellDisplay = ovito.plugins.Particles.SimulationCellDisplay
ovito.vis.ParticleDisplay = ovito.plugins.Particles.ParticleDisplay
ovito.vis.VectorDisplay = ovito.plugins.Particles.VectorDisplay
ovito.vis.BondsDisplay = ovito.plugins.Particles.BondsDisplay
ovito.vis.SurfaceMeshDisplay = ovito.plugins.Particles.SurfaceMeshDisplay
ovito.vis.TrajectoryLineDisplay = ovito.plugins.Particles.TrajectoryLineDisplay
ovito.vis.__all__ += ['SimulationCellDisplay', 'ParticleDisplay', 'VectorDisplay', 'BondsDisplay',
                      'SurfaceMeshDisplay', 'TrajectoryLineDisplay']

# Inject enum types.
ovito.vis.VectorDisplay.Shading = ovito.vis.ArrowShadingMode
ovito.vis.BondsDisplay.Shading = ovito.vis.ArrowShadingMode
ovito.vis.TrajectoryLineDisplay.Shading = ovito.vis.ArrowShadingMode
