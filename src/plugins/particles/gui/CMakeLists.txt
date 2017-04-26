###############################################################################
# 
#  Copyright (2016) Alexander Stukowski
#
#  This file is part of OVITO (Open Visualization Tool).
#
#  OVITO is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  OVITO is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
###############################################################################

SET(SourceFiles 
	objects/SimulationCellObjectEditor.cpp
	objects/SimulationCellDisplayEditor.cpp
	objects/ParticleDisplayEditor.cpp
	objects/ParticleTypeEditor.cpp
	objects/ParticleTypePropertyEditor.cpp
	objects/BondTypeEditor.cpp
	objects/BondTypePropertyEditor.cpp
	objects/VectorDisplayEditor.cpp
	objects/BondsDisplayEditor.cpp
	objects/SurfaceMeshDisplayEditor.cpp
	objects/TrajectoryDisplayEditor.cpp
	objects/TrajectoryGeneratorObjectEditor.cpp
	modifier/ParticleModifierEditor.cpp
	modifier/coloring/AssignColorModifierEditor.cpp
	modifier/coloring/ColorCodingModifierEditor.cpp
	modifier/coloring/ColorLegendOverlayEditor.cpp
	modifier/coloring/AmbientOcclusionModifierEditor.cpp
	modifier/modify/SliceModifierEditor.cpp
	modifier/modify/AffineTransformationModifierEditor.cpp
	modifier/modify/ShowPeriodicImagesModifierEditor.cpp
	modifier/modify/CreateBondsModifierEditor.cpp
	modifier/modify/LoadTrajectoryModifierEditor.cpp
	modifier/modify/CombineParticleSetsModifierEditor.cpp
	modifier/analysis/StructureListParameterUI.cpp
	modifier/analysis/binandreduce/BinAndReduceModifierEditor.cpp
	modifier/analysis/bondangle/BondAngleAnalysisModifierEditor.cpp
	modifier/analysis/cna/CommonNeighborAnalysisModifierEditor.cpp
	modifier/analysis/coordination/CoordinationNumberModifierEditor.cpp
	modifier/analysis/displacements/CalculateDisplacementsModifierEditor.cpp
	modifier/analysis/centrosymmetry/CentroSymmetryModifierEditor.cpp
	modifier/analysis/strain/AtomicStrainModifierEditor.cpp
	modifier/analysis/diamond/IdentifyDiamondModifierEditor.cpp	
	modifier/analysis/voronoi/VoronoiAnalysisModifierEditor.cpp
	modifier/analysis/histogram/HistogramModifierEditor.cpp
	modifier/analysis/cluster/ClusterAnalysisModifierEditor.cpp
	modifier/analysis/scatterplot/ScatterPlotModifierEditor.cpp
	modifier/analysis/wignerseitz/WignerSeitzAnalysisModifierEditor.cpp
	modifier/analysis/ptm/PolyhedralTemplateMatchingModifierEditor.cpp
	#modifier/analysis/structural_clustering/StructuralClusteringModifierEditor.cpp	
	modifier/selection/ExpandSelectionModifierEditor.cpp
	modifier/selection/SelectParticleTypeModifierEditor.cpp
	modifier/selection/SelectExpressionModifierEditor.cpp
	modifier/selection/ManualSelectionModifierEditor.cpp
	modifier/properties/ComputePropertyModifierEditor.cpp
	modifier/properties/FreezePropertyModifierEditor.cpp
	modifier/fields/CreateIsosurfaceModifierEditor.cpp
	util/ParticlePropertyComboBox.cpp
	util/CutoffRadiusPresetsUI.cpp
	util/ParticleInformationApplet.cpp
	util/ParticlePickingHelper.cpp
	util/ParticlePropertyParameterUI.cpp
	util/BondPropertyComboBox.cpp
	util/BondPropertyParameterUI.cpp
	util/FieldQuantityComboBox.cpp
	util/FieldQuantityParameterUI.cpp
	util/CreateTrajectoryApplet.cpp
	util/ParticleSettingsPage.cpp
	import/InputColumnMappingDialog.cpp
	import/lammps/LAMMPSTextDumpImporterEditor.cpp
	import/lammps/LAMMPSBinaryDumpImporterEditor.cpp
	import/lammps/LAMMPSDataImporterEditor.cpp
	import/xyz/XYZImporterEditor.cpp
	export/FileColumnParticleExporterEditor.cpp
	export/lammps/LAMMPSDataExporterEditor.cpp
	export/xyz/XYZExporterEditor.cpp
)

# Compile resources.
QT5_ADD_RESOURCES(ResourceFiles resources/particles_gui.qrc)

OVITO_STANDARD_PLUGIN(ParticlesGui
	SOURCES ParticlesGui.cpp ${SourceFiles} ${ResourceFiles}
	PRIVATE_LIB_DEPENDENCIES Qwt
	PLUGIN_DEPENDENCIES Particles
	GUI_PLUGIN
)

# Speed up compilation by using precompiled headers.
ADD_PRECOMPILED_HEADER(ParticlesGui plugins/particles/gui/ParticlesGui.h)
