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

# Implement the 'rdf' attribute of the CoordinationNumberModifier class.
def _CoordinationNumberModifier_rdf(self):
    """
    Returns a NumPy array containing the radial distribution function (RDF) computed by the modifier.    
    The returned array is two-dimensional and consists of the [*r*, *g(r)*] data points of the tabulated *g(r)* RDF function.
    
    Note that accessing this array is only possible after the modifier has computed its results. 
    Thus, you have to call :py:meth:`ovito.ObjectNode.compute` first to ensure that this information is up to date, see the example above.
    """
    rdfx = numpy.asarray(self.rdf_x)
    rdfy = numpy.asarray(self.rdf_y)
    return numpy.transpose((rdfx,rdfy))
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
    # Get counts
    ydata = numpy.asarray(self.histogramData)
    # Compute bin center positions
    binsize = (self.xrange_end - self.xrange_start) / len(ydata)
    xdata = numpy.linspace(self.xrange_start + binsize * 0.5, self.xrange_end + binsize * 0.5, len(ydata), endpoint = False)
    return numpy.transpose((xdata,ydata))
ovito.modifiers.HistogramModifier.histogram = property(_HistogramModifier_histogram)

# Implement the 'bin_data' attribute of the BinAndReduceModifier class.
def _BinAndReduceModifier_bin_data(self):
    """
    Returns a NumPy array containing the reduced bin values computed by the modifier.    
    Depending on the selected binning :py:attr:`.direction` the returned array is either
    one or two-dimensional. In the two-dimensional case the outer index of the returned array
    runs over the bins along the second binning axis.
    
    Note that accessing this array is only possible after the modifier has computed its results. 
    Thus, you have to call :py:meth:`ovito.ObjectNode.compute` first to ensure that the binning and reduction operation was performed.
    """
    data = numpy.asarray(self._binData)
    if self._is1D:
        assert(self.bin_count_x == len(data))
        return data
    else:
        assert(self.bin_count_y * self.bin_count_x == len(data))
        return numpy.reshape(data, (self.bin_count_y, self.bin_count_x))
ovito.modifiers.BinAndReduceModifier.bin_data = property(_BinAndReduceModifier_bin_data)

# Implement the ColorCodingModifier custom color map constructor.
def _ColorCodingModifier_Custom(filename):
    gradient = ovito.modifiers.ColorCodingModifier.Image()
    gradient.loadImage(filename)
    return gradient
ovito.modifiers.ColorCodingModifier.Custom = staticmethod(_ColorCodingModifier_Custom)
