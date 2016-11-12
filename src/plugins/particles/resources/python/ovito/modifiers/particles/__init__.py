# Load system libraries
import numpy

# Load dependencies
import ovito.modifiers

# Load the native code modules.
import Particles
import ParticlesModify

# Inject modifier classes into parent module.
ovito.modifiers.ColorCodingModifier = ParticlesModify.ColorCodingModifier
ovito.modifiers.AssignColorModifier = ParticlesModify.AssignColorModifier
ovito.modifiers.AmbientOcclusionModifier = ParticlesModify.AmbientOcclusionModifier
ovito.modifiers.DeleteSelectedParticlesModifier = ParticlesModify.DeleteSelectedParticlesModifier
ovito.modifiers.ShowPeriodicImagesModifier = ParticlesModify.ShowPeriodicImagesModifier
ovito.modifiers.WrapPeriodicImagesModifier = ParticlesModify.WrapPeriodicImagesModifier
ovito.modifiers.ComputePropertyModifier = ParticlesModify.ComputePropertyModifier
ovito.modifiers.FreezePropertyModifier = ParticlesModify.FreezePropertyModifier
ovito.modifiers.ClearSelectionModifier = ParticlesModify.ClearSelectionModifier
ovito.modifiers.InvertSelectionModifier = ParticlesModify.InvertSelectionModifier
ovito.modifiers.ManualSelectionModifier = ParticlesModify.ManualSelectionModifier
ovito.modifiers.ExpandSelectionModifier = ParticlesModify.ExpandSelectionModifier
ovito.modifiers.SelectExpressionModifier = ParticlesModify.SelectExpressionModifier
ovito.modifiers.SelectParticleTypeModifier = ParticlesModify.SelectParticleTypeModifier
ovito.modifiers.SliceModifier = ParticlesModify.SliceModifier
ovito.modifiers.AffineTransformationModifier = ParticlesModify.AffineTransformationModifier
ovito.modifiers.BinAndReduceModifier = ParticlesModify.BinAndReduceModifier
ovito.modifiers.StructureIdentificationModifier = ParticlesModify.StructureIdentificationModifier
ovito.modifiers.CommonNeighborAnalysisModifier = ParticlesModify.CommonNeighborAnalysisModifier
ovito.modifiers.BondAngleAnalysisModifier = ParticlesModify.BondAngleAnalysisModifier
ovito.modifiers.CreateBondsModifier = ParticlesModify.CreateBondsModifier
ovito.modifiers.CentroSymmetryModifier = ParticlesModify.CentroSymmetryModifier
ovito.modifiers.ClusterAnalysisModifier = ParticlesModify.ClusterAnalysisModifier
ovito.modifiers.CoordinationNumberModifier = ParticlesModify.CoordinationNumberModifier
ovito.modifiers.CalculateDisplacementsModifier = ParticlesModify.CalculateDisplacementsModifier
ovito.modifiers.HistogramModifier = ParticlesModify.HistogramModifier
ovito.modifiers.ScatterPlotModifier = ParticlesModify.ScatterPlotModifier
ovito.modifiers.AtomicStrainModifier = ParticlesModify.AtomicStrainModifier
ovito.modifiers.WignerSeitzAnalysisModifier = ParticlesModify.WignerSeitzAnalysisModifier
ovito.modifiers.VoronoiAnalysisModifier = ParticlesModify.VoronoiAnalysisModifier
ovito.modifiers.IdentifyDiamondModifier = ParticlesModify.IdentifyDiamondModifier
ovito.modifiers.LoadTrajectoryModifier = ParticlesModify.LoadTrajectoryModifier
ovito.modifiers.CombineParticleSetsModifier = ParticlesModify.CombineParticleSetsModifier
ovito.modifiers.ComputeBondLengthsModifier = ParticlesModify.ComputeBondLengthsModifier
ovito.modifiers.PolyhedralTemplateMatchingModifier = ParticlesModify.PolyhedralTemplateMatchingModifier

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
    self._take_snapshot(time, True)
ovito.modifiers.FreezePropertyModifier.take_snapshot = _FreezePropertyModifier_take_snapshot
