"""
This module contains all modifiers of OVITO.

The abstract base class of all modifier types is the :py:class:`Modifier` class.
Typically you create a modifier instance, set its parameters, and finally insert it into the
modification pipeline of an :py:class:`~ovito.ObjectNode`, e.g.::

    from ovito.modifiers import *
    m = AssignColorModifier()
    m.color = (0.2, 1.0, 0.9)
    node.modifiers.append(m)
    
The following modifier types are available:

============================================== =========================================
Python class name                              User interface name
============================================== =========================================
:py:class:`AffineTransformationModifier`       :guilabel:`Affine transformation`
:py:class:`AmbientOcclusionModifier`           :guilabel:`Ambient occlusion`
:py:class:`AssignColorModifier`                :guilabel:`Assign color`
:py:class:`AtomicStrainModifier`               :guilabel:`Atomic strain`
:py:class:`BinAndReduceModifier`               :guilabel:`Bin and reduce`
:py:class:`BondAngleAnalysisModifier`          :guilabel:`Bond-angle analysis`
:py:class:`CalculateDisplacementsModifier`     :guilabel:`Displacement vectors`
:py:class:`CentroSymmetryModifier`             :guilabel:`Centrosymmetry parameter`
:py:class:`ClearSelectionModifier`             :guilabel:`Clear selection`
:py:class:`ClusterAnalysisModifier`            :guilabel:`Cluster analysis`
:py:class:`ColorCodingModifier`                :guilabel:`Color coding`
:py:class:`CombineParticleSetsModifier`        :guilabel:`Combine particle sets`
:py:class:`CommonNeighborAnalysisModifier`     :guilabel:`Common neighbor analysis`
:py:class:`ComputeBondLengthsModifier`         :guilabel:`Compute bond lengths`
:py:class:`ComputePropertyModifier`            :guilabel:`Compute property`
:py:class:`ConstructSurfaceModifier`           :guilabel:`Construct surface mesh`
:py:class:`CoordinationNumberModifier`         :guilabel:`Coordination analysis`
:py:class:`CoordinationPolyhedraModifier`      :guilabel:`Coordination polyhedra`
:py:class:`CreateBondsModifier`                :guilabel:`Create bonds`
:py:class:`CreateIsosurfaceModifier`           :guilabel:`Create isosurface`
:py:class:`DeleteSelectedParticlesModifier`    :guilabel:`Delete selected particles`
:py:class:`DislocationAnalysisModifier`        :guilabel:`Dislocation analysis (DXA)`
:py:class:`ElasticStrainModifier`              :guilabel:`Elastic strain calculation`
:py:class:`ExpandSelectionModifier`            :guilabel:`Expand selection`
:py:class:`FreezePropertyModifier`             :guilabel:`Freeze property`
:py:class:`HistogramModifier`                  :guilabel:`Histogram`
:py:class:`IdentifyDiamondModifier`            :guilabel:`Identify diamond structure`
:py:class:`InvertSelectionModifier`            :guilabel:`Invert selection`
:py:class:`LoadTrajectoryModifier`             :guilabel:`Load trajectory`
:py:class:`ManualSelectionModifier`            :guilabel:`Manual selection`
:py:class:`PolyhedralTemplateMatchingModifier` :guilabel:`Polyhedral template matching`
:py:class:`PythonScriptModifier`               :guilabel:`Python script`
:py:class:`ScatterPlotModifier`                :guilabel:`Scatter plot`
:py:class:`SelectExpressionModifier`           :guilabel:`Expression select`
:py:class:`SelectParticleTypeModifier`         :guilabel:`Select particle type`
:py:class:`ShowPeriodicImagesModifier`         :guilabel:`Show periodic images`
:py:class:`SliceModifier`                      :guilabel:`Slice`
:py:class:`VoronoiAnalysisModifier`            :guilabel:`Voronoi analysis`
:py:class:`WignerSeitzAnalysisModifier`        :guilabel:`Wigner-Seitz defect analysis`
:py:class:`WrapPeriodicImagesModifier`         :guilabel:`Wrap at periodic boundaries`
============================================== =========================================

*Note that some analysis modifiers are not accessible from Python. That is because they do things that can be achieved equally well using the Numpy python module.*

"""

# Load the native module.
from ..plugins.PyScript.Scene import Modifier
from ..plugins.PyScript.Scene import PythonScriptModifier

__all__ = ['Modifier', 'PythonScriptModifier']
