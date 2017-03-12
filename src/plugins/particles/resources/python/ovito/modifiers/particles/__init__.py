# Load system libraries
import numpy

# Load dependencies
import ovito.modifiers

# Load the native code modules.
import ovito.plugins.Particles
import ovito.plugins.Particles.Modifiers

# Inject modifier classes into parent module.
ovito.modifiers.ColorCodingModifier = ovito.plugins.Particles.Modifiers.ColorCodingModifier
ovito.modifiers.AssignColorModifier = ovito.plugins.Particles.Modifiers.AssignColorModifier
ovito.modifiers.AmbientOcclusionModifier = ovito.plugins.Particles.Modifiers.AmbientOcclusionModifier
ovito.modifiers.DeleteSelectedParticlesModifier = ovito.plugins.Particles.Modifiers.DeleteSelectedParticlesModifier
ovito.modifiers.ShowPeriodicImagesModifier = ovito.plugins.Particles.Modifiers.ShowPeriodicImagesModifier
ovito.modifiers.WrapPeriodicImagesModifier = ovito.plugins.Particles.Modifiers.WrapPeriodicImagesModifier
ovito.modifiers.ComputePropertyModifier = ovito.plugins.Particles.Modifiers.ComputePropertyModifier
ovito.modifiers.FreezePropertyModifier = ovito.plugins.Particles.Modifiers.FreezePropertyModifier
ovito.modifiers.ClearSelectionModifier = ovito.plugins.Particles.Modifiers.ClearSelectionModifier
ovito.modifiers.InvertSelectionModifier = ovito.plugins.Particles.Modifiers.InvertSelectionModifier
ovito.modifiers.ManualSelectionModifier = ovito.plugins.Particles.Modifiers.ManualSelectionModifier
ovito.modifiers.ExpandSelectionModifier = ovito.plugins.Particles.Modifiers.ExpandSelectionModifier
ovito.modifiers.SelectExpressionModifier = ovito.plugins.Particles.Modifiers.SelectExpressionModifier
ovito.modifiers.SelectParticleTypeModifier = ovito.plugins.Particles.Modifiers.SelectParticleTypeModifier
ovito.modifiers.SliceModifier = ovito.plugins.Particles.Modifiers.SliceModifier
ovito.modifiers.AffineTransformationModifier = ovito.plugins.Particles.Modifiers.AffineTransformationModifier
ovito.modifiers.BinAndReduceModifier = ovito.plugins.Particles.Modifiers.BinAndReduceModifier
ovito.modifiers.StructureIdentificationModifier = ovito.plugins.Particles.Modifiers.StructureIdentificationModifier
ovito.modifiers.CommonNeighborAnalysisModifier = ovito.plugins.Particles.Modifiers.CommonNeighborAnalysisModifier
ovito.modifiers.BondAngleAnalysisModifier = ovito.plugins.Particles.Modifiers.BondAngleAnalysisModifier
ovito.modifiers.CreateBondsModifier = ovito.plugins.Particles.Modifiers.CreateBondsModifier
ovito.modifiers.CentroSymmetryModifier = ovito.plugins.Particles.Modifiers.CentroSymmetryModifier
ovito.modifiers.ClusterAnalysisModifier = ovito.plugins.Particles.Modifiers.ClusterAnalysisModifier
ovito.modifiers.CoordinationNumberModifier = ovito.plugins.Particles.Modifiers.CoordinationNumberModifier
ovito.modifiers.CalculateDisplacementsModifier = ovito.plugins.Particles.Modifiers.CalculateDisplacementsModifier
ovito.modifiers.HistogramModifier = ovito.plugins.Particles.Modifiers.HistogramModifier
ovito.modifiers.ScatterPlotModifier = ovito.plugins.Particles.Modifiers.ScatterPlotModifier
ovito.modifiers.AtomicStrainModifier = ovito.plugins.Particles.Modifiers.AtomicStrainModifier
ovito.modifiers.WignerSeitzAnalysisModifier = ovito.plugins.Particles.Modifiers.WignerSeitzAnalysisModifier
ovito.modifiers.VoronoiAnalysisModifier = ovito.plugins.Particles.Modifiers.VoronoiAnalysisModifier
ovito.modifiers.IdentifyDiamondModifier = ovito.plugins.Particles.Modifiers.IdentifyDiamondModifier
ovito.modifiers.LoadTrajectoryModifier = ovito.plugins.Particles.Modifiers.LoadTrajectoryModifier
ovito.modifiers.CombineParticleSetsModifier = ovito.plugins.Particles.Modifiers.CombineParticleSetsModifier
ovito.modifiers.ComputeBondLengthsModifier = ovito.plugins.Particles.Modifiers.ComputeBondLengthsModifier
ovito.modifiers.PolyhedralTemplateMatchingModifier = ovito.plugins.Particles.Modifiers.PolyhedralTemplateMatchingModifier
ovito.modifiers.CreateIsosurfaceModifier = ovito.plugins.Particles.Modifiers.CreateIsosurfaceModifier
ovito.modifiers.CoordinationPolyhedraModifier = ovito.plugins.Particles.Modifiers.CoordinationPolyhedraModifier
ovito.modifiers.__all__ += ['ColorCodingModifier', 'AssignColorModifier', 'AmbientOcclusionModifier', 'DeleteSelectedParticlesModifier',
            'ShowPeriodicImagesModifier', 'WrapPeriodicImagesModifier', 'ComputePropertyModifier', 'FreezePropertyModifier',
            'ClearSelectionModifier', 'InvertSelectionModifier', 'ManualSelectionModifier', 'ExpandSelectionModifier',
            'SelectExpressionModifier', 'SelectParticleTypeModifier', 'SliceModifier', 'AffineTransformationModifier', 'BinAndReduceModifier',
            'StructureIdentificationModifier', 'CommonNeighborAnalysisModifier', 'BondAngleAnalysisModifier',
            'CreateBondsModifier', 'CentroSymmetryModifier', 'ClusterAnalysisModifier', 'CoordinationNumberModifier',
            'CalculateDisplacementsModifier', 'HistogramModifier', 'ScatterPlotModifier', 'AtomicStrainModifier',
            'WignerSeitzAnalysisModifier', 'VoronoiAnalysisModifier', 'IdentifyDiamondModifier', 'LoadTrajectoryModifier',
            'CombineParticleSetsModifier', 'ComputeBondLengthsModifier', 'PolyhedralTemplateMatchingModifier',
            'CreateIsosurfaceModifier', 'CoordinationPolyhedraModifier']

# Implement the 'rdf' attribute of the CoordinationNumberModifier class.
def _CoordinationNumberModifier_rdf(self):
    """
    Returns a NumPy array containing the radial distribution function (RDF) computed by the modifier.    
    The returned array is two-dimensional and consists of the [*r*, *g(r)*] data points of the tabulated *g(r)* RDF function.
    
    Note that accessing this array is only possible after the modifier has computed its results. 
    Thus, you have to call :py:meth:`ovito.ObjectNode.compute` first to ensure that this information is up to date, see the example above.
    """
    return numpy.transpose((self.rdf_x,self.rdf_y))
ovito.modifiers.CoordinationNumberModifier.rdf = property(_CoordinationNumberModifier_rdf)

# Implement the 'histogram' attribute of the HistogramModifier class.
def _HistogramModifier_histogram(self):
    """
    Returns a NumPy array containing the histogram computed by the modifier.    
    The returned array is two-dimensional and consists of [*x*, *count(x)*] value pairs, where
    *x* denotes the bin center and *count(x)* the number of particles whose property value falls into the bin.
    
    Note that accessing this array is only possible after the modifier has computed its results. 
    Thus, you have to call :py:meth:`ovito.ObjectNode.compute` first to ensure that the histogram was generated.
    """
    # Get bin counts
    ydata = self._histogram_data
    if len(ydata) != self.bin_count:
        raise RuntimeError("The modifier has not computed its results yet.")
    # Compute bin center positions
    binsize = (self.xrange_end - self.xrange_start) / len(ydata)
    xdata = numpy.linspace(self.xrange_start + binsize * 0.5, self.xrange_end + binsize * 0.5, len(ydata), endpoint = False)
    return numpy.transpose((xdata,ydata))
ovito.modifiers.HistogramModifier.histogram = property(_HistogramModifier_histogram)

def _FreezePropertyModifier_take_snapshot(self, frame = None):
    """
    Evaluates the modification pipeline up to this modifier and makes a copy of the current
    values of the :py:attr:`.source_property`.
    
    :param int frame: The animation frame at which to take the snapshot. If ``none``, the 
                       current animation frame is used (see :py:attr:`ovito.anim.AnimationSettings.current_frame`).
                       
    Note that :py:meth:`!take_snapshot` must be called *after* the modifier has been inserted into an 
    :py:class:`~ovito.ObjectNode`'s modification pipeline.
    """
    
    # Make sure modifier has been inserted into a modification pipeline.
    if len(self.modifier_applications) == 0:
        raise RuntimeError("take_snapshot() must be called after the FreezePropertyModifier has been inserted into the pipeline.")
    
    if frame is not None:
        time = self.dataset.anim.frameToTime(frame)
    else:
        time = self.dataset.anim.time
    if not self._take_snapshot(time, ovito.task_manager, True):
        raise RuntimeError("Operation has been canceled by the user.")
ovito.modifiers.FreezePropertyModifier.take_snapshot = _FreezePropertyModifier_take_snapshot
