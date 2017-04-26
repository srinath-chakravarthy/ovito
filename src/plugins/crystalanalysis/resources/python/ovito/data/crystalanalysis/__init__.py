# Load dependencies
import ovito
import ovito.data
import ovito.modifiers
import ovito.modifiers.particles
import numpy

# Load the native code modules
import ovito.plugins.Particles
import ovito.plugins.CrystalAnalysis

# Inject selected classes into parent module.
ovito.data.DislocationNetwork = ovito.plugins.CrystalAnalysis.DislocationNetwork
ovito.data.DislocationSegment = ovito.plugins.CrystalAnalysis.DislocationSegment
ovito.data.PartitionMesh = ovito.plugins.CrystalAnalysis.PartitionMesh
ovito.data.__all__ += ['DislocationNetwork', 'DislocationSegment', 'PartitionMesh']

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

