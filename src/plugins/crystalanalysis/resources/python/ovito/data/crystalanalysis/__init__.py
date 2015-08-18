# Load dependencies
import ovito
import ovito.data
import ovito.modifiers
import ovito.modifiers.particles
import numpy

# Load the native code module
import CrystalAnalysis

# Inject selected classes into parent module.
ovito.data.DislocationNetwork = CrystalAnalysis.DislocationNetwork
ovito.data.DislocationSegment = CrystalAnalysis.DislocationSegment

# Register attribute keys by which data objects in a DataCollection can be accessed.
CrystalAnalysis.DislocationNetwork._data_attribute_name = "dislocations"

# Implement the 'points' property of the DislocationSegment class.
def _DislocationSegment_points(self):
    """ The list of points in space that define the shape of this dislocation segment. """
    return numpy.asarray(self._line)
CrystalAnalysis.DislocationSegment.points = property(_DislocationSegment_points)
