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
	importer/CAImporterEditor.cpp
	objects/patterns/BurgersVectorFamilyEditor.cpp
	objects/patterns/StructurePatternEditor.cpp
	objects/patterns/PatternCatalogEditor.cpp
	objects/dislocations/DislocationDisplayEditor.cpp
	objects/partition_mesh/PartitionMeshDisplayEditor.cpp
	objects/slip_surface/SlipSurfaceDisplayEditor.cpp
	modifier/SmoothSurfaceModifierEditor.cpp
	modifier/SmoothDislocationsModifierEditor.cpp
	modifier/ConstructSurfaceModifierEditor.cpp
	modifier/DislocationAnalysisModifierEditor.cpp
	modifier/ElasticStrainModifierEditor.cpp
#	modifier/GrainSegmentationModifierEditor.cpp
	util/DislocationInformationApplet.cpp
)

OVITO_STANDARD_PLUGIN(CrystalAnalysisGui
	SOURCES ${SourceFiles}
	PLUGIN_DEPENDENCIES CrystalAnalysis ParticlesGui
	GUI_PLUGIN
)
