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

# Implement 'dislocations' attribute of DataCollection class.
def _DataCollection_dislocations(self):
    """
    Returns the :py:class:`DislocationNetwork` object in this :py:class:`!DataCollection`.
    
    Accessing this property raises an ``AttributeError`` if the data collection
    contains no dislocations object.
    """
    for obj in self.objects:
        if isinstance(obj, ovito.data.DislocationNetwork):
            return obj
    raise AttributeError("This DataCollection contains no dislocations.")
ovito.data.DataCollection.dislocations = property(_DataCollection_dislocations)

# Implement the 'points' property of the DislocationSegment class.
def _DislocationSegment_points(self):
    """ The list of points in space that define the shape of this dislocation segment. """
    return numpy.asarray(self._line)
CrystalAnalysis.DislocationSegment.points = property(_DislocationSegment_points)
