###############################################################################
# 
#  Copyright (2013) Alexander Stukowski
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

# Find the CGAL library.
FIND_PACKAGE(CGAL REQUIRED)
IF(NOT CGAL_FOUND)
	MESSAGE(FATAL_ERROR "Could not locate CGAL library, which is required by CrystalAnalysis plugin.")
ENDIF()
# Link to CGAL library.
INCLUDE(${CGAL_USE_FILE})

SET(SourceFiles
	importer/CAImporter.cpp
	exporter/CAExporter.cpp
	objects/patterns/BurgersVectorFamily.cpp
	objects/patterns/StructurePattern.cpp
	objects/patterns/PatternCatalog.cpp
	objects/clusters/ClusterGraphObject.cpp
	objects/dislocations/DislocationNetworkObject.cpp
	objects/dislocations/DislocationDisplay.cpp
	data/ClusterGraph.cpp
	data/DislocationNetwork.cpp
	data/PlanarDefects.cpp
	modifier/SmoothSurfaceModifier.cpp
	modifier/SmoothDislocationsModifier.cpp
	modifier/ConstructSurfaceModifier.cpp
	modifier/dxa/DislocationAnalysisModifier.cpp
	modifier/dxa/DislocationAnalysisEngine.cpp
	modifier/dxa/StructureAnalysis.cpp
	modifier/dxa/CrystalPathFinder.cpp
	modifier/dxa/ElasticMapping.cpp
	modifier/dxa/InterfaceMesh.cpp
	modifier/dxa/DislocationTracer.cpp
	modifier/dxa/PlanarDefectIdentification.cpp
	modifier/elasticstrain/ElasticStrainModifier.cpp
	modifier/elasticstrain/ElasticStrainEngine.cpp
	util/DelaunayTessellation.cpp
	util/DislocationInformationApplet.cpp
)

IF(OVITO_BUILD_PLUGIN_PYSCRIPT)
	LIST(APPEND SourceFiles scripting/PythonInterface.cpp)
ENDIF()

OVITO_STANDARD_PLUGIN(CrystalAnalysis
	SOURCES ${SourceFiles}
	PLUGIN_DEPENDENCIES Particles
	LIB_DEPENDENCIES ${CGAL_LIBRARIES} ${CGAL_3RD_PARTY_LIBRARIES} ${OPENGL_gl_LIBRARY}
	OPTIONAL_PLUGIN_DEPENDENCIES PyScript
	PYTHON_WRAPPERS "${CMAKE_CURRENT_SOURCE_DIR}/resources/python/"
)

IF(NOT CMAKE_COMPILER_IS_GNUCXX)
	TARGET_COMPILE_DEFINITIONS(CrystalAnalysis PRIVATE CGAL_CFG_ARRAY_MEMBER_INITIALIZATION_BUG)
ENDIF()

IF(WIN32)
	# Install CGAL dll and its dependencies (GMP, MPFR, and Boost DLLs).
	FILE(GLOB CGAL_LIBRARY_DLLS "${CGAL_LIBRARIES_DIR}/../bin/CGAL*.dll")
	FOREACH(dll ${CGAL_LIBRARY_DLLS})
		OVITO_INSTALL_DLL("${dll}")
	ENDFOREACH()

	GET_FILENAME_COMPONENT(GMP_BASE_NAME "${GMP_LIBRARIES}" NAME_WE)
	GET_FILENAME_COMPONENT(GMP_LIB_PATH "${GMP_LIBRARIES}" PATH)
	OVITO_INSTALL_DLL("${GMP_LIB_PATH}/${GMP_BASE_NAME}.dll")

	GET_FILENAME_COMPONENT(MPFR_BASE_NAME "${MPFR_LIBRARIES}" NAME_WE)
	GET_FILENAME_COMPONENT(MPFR_LIB_PATH "${MPFR_LIBRARIES}" PATH)
	OVITO_INSTALL_DLL("${MPFR_LIB_PATH}/${MPFR_BASE_NAME}.dll")

ELSEIF(UNIX AND NOT APPLE AND OVITO_MONOLITHIC_BUILD)

	# Deploy CGAL shared library and its dependencies.
	FOREACH(lib ${CGAL_LIBRARIES} ${CGAL_3RD_PARTY_LIBRARIES} ${GMP_LIBRARIES} ${MPFR_LIBRARIES})
		FILE(GLOB lib_versions "${lib}*")
		FOREACH(lib_version ${lib_versions})
			INSTALL(FILES "${lib_version}" DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/")
		ENDFOREACH()
	ENDFOREACH()

ENDIF()
