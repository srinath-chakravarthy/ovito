###############################################################################
# 
#  Copyright (2014) Alexander Stukowski
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
	objects/SimulationCellObject.cpp
	objects/SimulationCellDisplay.cpp
	objects/ParticlePropertyObject.cpp
	objects/ParticleDisplay.cpp
	objects/ParticleType.cpp
	objects/ParticleTypeProperty.cpp
	objects/BondType.cpp
	objects/BondPropertyObject.cpp
	objects/BondTypeProperty.cpp
	objects/VectorDisplay.cpp
	objects/BondsObject.cpp
	objects/BondsDisplay.cpp
	objects/SurfaceMesh.cpp
	objects/SurfaceMeshDisplay.cpp
	objects/TrajectoryObject.cpp
	objects/TrajectoryDisplay.cpp
	objects/TrajectoryGeneratorObject.cpp
	data/ParticleProperty.cpp
	data/BondProperty.cpp
	data/PropertyBase.cpp
	data/BondsStorage.cpp
	modifier/ParticleModifier.cpp
	modifier/AsynchronousParticleModifier.cpp
	modifier/coloring/AssignColorModifier.cpp
	modifier/coloring/ColorCodingModifier.cpp
	modifier/coloring/ColorLegendOverlay.cpp
	modifier/coloring/AmbientOcclusionModifier.cpp
	modifier/coloring/AmbientOcclusionRenderer.cpp
	modifier/modify/SliceModifier.cpp
	modifier/modify/DeleteParticlesModifier.cpp
	modifier/modify/AffineTransformationModifier.cpp
	modifier/modify/ShowPeriodicImagesModifier.cpp
	modifier/modify/WrapPeriodicImagesModifier.cpp
	modifier/modify/CreateBondsModifier.cpp
	modifier/modify/LoadTrajectoryModifier.cpp
	modifier/modify/CombineParticleSetsModifier.cpp
	modifier/analysis/StructureIdentificationModifier.cpp
	modifier/analysis/binandreduce/BinAndReduceModifier.cpp
	modifier/analysis/bondangle/BondAngleAnalysisModifier.cpp
	modifier/analysis/cna/CommonNeighborAnalysisModifier.cpp
	modifier/analysis/coordination/CoordinationNumberModifier.cpp
	modifier/analysis/displacements/CalculateDisplacementsModifier.cpp
	modifier/analysis/centrosymmetry/CentroSymmetryModifier.cpp
	modifier/analysis/strain/AtomicStrainModifier.cpp
	modifier/analysis/diamond/IdentifyDiamondModifier.cpp	
	modifier/analysis/voronoi/VoronoiAnalysisModifier.cpp
	modifier/analysis/histogram/HistogramModifier.cpp
	modifier/analysis/cluster/ClusterAnalysisModifier.cpp
	modifier/analysis/scatterplot/ScatterPlotModifier.cpp
	modifier/analysis/wignerseitz/WignerSeitzAnalysisModifier.cpp
	modifier/analysis/ptm/PolyhedralTemplateMatchingModifier.cpp
	#modifier/analysis/structural_clustering/StructuralClusteringModifier.cpp
	modifier/selection/ExpandSelectionModifier.cpp
	modifier/selection/InvertSelectionModifier.cpp
	modifier/selection/SelectParticleTypeModifier.cpp
	modifier/selection/SelectExpressionModifier.cpp
	modifier/selection/ManualSelectionModifier.cpp
	modifier/selection/ClearSelectionModifier.cpp
	modifier/properties/ComputePropertyModifier.cpp
	modifier/properties/FreezePropertyModifier.cpp
	modifier/properties/ComputeBondLengthsModifier.cpp
	import/ParticleImporter.cpp
	import/ParticleFrameLoader.cpp
	import/InputColumnMapping.cpp
	import/lammps/LAMMPSTextDumpImporter.cpp
	import/lammps/LAMMPSBinaryDumpImporter.cpp
	import/lammps/LAMMPSDataImporter.cpp
	import/vasp/POSCARImporter.cpp
	import/xyz/XYZImporter.cpp
	import/imd/IMDImporter.cpp
	import/parcas/ParcasFileImporter.cpp
	import/cfg/CFGImporter.cpp
	import/pdb/PDBImporter.cpp
	import/fhi_aims/FHIAimsImporter.cpp
	import/fhi_aims/FHIAimsLogFileImporter.cpp
	import/gsd/GSDImporter.cpp
	import/gsd/gsd.c
	export/ParticleExporter.cpp
	export/FileColumnParticleExporter.cpp
	export/OutputColumnMapping.cpp
	export/vasp/POSCARExporter.cpp
	export/fhi_aims/FHIAimsExporter.cpp
	export/lammps/LAMMPSDataExporter.cpp
	export/lammps/LAMMPSDumpExporter.cpp
	export/xyz/XYZExporter.cpp
	export/imd/IMDExporter.cpp
	util/NearestNeighborFinder.cpp
	util/CutoffNeighborFinder.cpp
	util/ParticleExpressionEvaluator.cpp
	util/ParticleSelectionSet.cpp
)

IF(OVITO_BUILD_PLUGIN_PYSCRIPT)
	LIST(APPEND SourceFiles 
		scripting/PythonInterface.cpp
		scripting/ImporterBinding.cpp
		scripting/ExporterBinding.cpp
		scripting/ModifierBinding.cpp
	)
ENDIF()

# List of source files of the GLU library used for polygon tessellation.
SET(GLUSourceFiles
	util/polytess/dict.c
	util/polytess/geom.c
	util/polytess/memalloc.c
	util/polytess/mesh.c
	util/polytess/normal.c
	util/polytess/priorityq.c
	util/polytess/render.c
	util/polytess/sweep.c
	util/polytess/tess.c
	util/polytess/tessmono.c
)

OVITO_STANDARD_PLUGIN(Particles
	SOURCES Particles.cpp ${SourceFiles} ${GLUSourceFiles}
	LIB_DEPENDENCIES muParser
	PRIVATE_LIB_DEPENDENCIES VoroPlusPlus OpenGLRenderer PolyhedralTemplateMatching CombinatoriallyOptimalPointRegistration
	OPTIONAL_PLUGIN_DEPENDENCIES PyScript
	PYTHON_WRAPPERS "${CMAKE_CURRENT_SOURCE_DIR}/resources/python/"
)

# Speed up compilation by using precompiled headers.
ADD_PRECOMPILED_HEADER(Particles plugins/particles/Particles.h)

# Build corresponding GUI plugin.
IF(OVITO_BUILD_GUI)
	ADD_SUBDIRECTORY(gui)
ENDIF()

# Propagate list of plugins to parent scope.
SET(OVITO_PLUGIN_LIST ${OVITO_PLUGIN_LIST} PARENT_SCOPE)
