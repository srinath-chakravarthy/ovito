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

IF(WIN32 OR (UNIX AND NOT APPLE AND OVITO_REDISTRIBUTABLE_PACKAGE))
	# Locate the HDF5 library.
	FIND_PACKAGE(HDF5 COMPONENTS hdf5 REQUIRED NO_MODULE)
ENDIF()
# Locate the NetCDF library.
FIND_PACKAGE(NetCDF REQUIRED)

SET(SourceFiles
	NetCDFImporter.cpp
	#NetCDFExporter.cpp
)

IF(OVITO_BUILD_PLUGIN_PYSCRIPT)
	LIST(APPEND SourceFiles PythonInterface.cpp)
ENDIF()

OVITO_STANDARD_PLUGIN(
	NetCDFPlugin
	SOURCES ${SourceFiles}
	PLUGIN_DEPENDENCIES Particles
	OPTIONAL_PLUGIN_DEPENDENCIES PyScript	
	PYTHON_WRAPPERS "${CMAKE_CURRENT_SOURCE_DIR}/resources/python/"
	PRIVATE_LIB_DEPENDENCIES netcdf
)

TARGET_INCLUDE_DIRECTORIES(NetCDFPlugin PRIVATE "${netcdf_INCLUDE_DIRS}")
IF(WIN32)
	TARGET_COMPILE_DEFINITIONS(NetCDFPlugin PRIVATE DLL_NETCDF)
ENDIF()

IF(WIN32)

	# Deploy NetCDF DLLs.
	GET_TARGET_PROPERTY(NETCDF_DLL_LOCATION netcdf IMPORTED_LOCATION_RELEASE)
	OVITO_INSTALL_DLL("${NETCDF_DLL_LOCATION}")

	# Deploy HDF5 DLLs, which are required by NetCDF library.
	GET_TARGET_PROPERTY(HDF5_DLL_LOCATION hdf5-shared IMPORTED_LOCATION_RELEASE)
	IF(NOT HDF5_DLL_LOCATION)
		GET_TARGET_PROPERTY(HDF5_DLL_LOCATION hdf5 IMPORTED_LOCATION_RELEASE)
	ENDIF()
	IF(NOT HDF5_DLL_LOCATION)
		GET_TARGET_PROPERTY(HDF5_DLL_LOCATION hdf5::hdf5-shared IMPORTED_LOCATION_RELEASE)
	ENDIF()
	OVITO_INSTALL_DLL("${HDF5_DLL_LOCATION}")
	GET_TARGET_PROPERTY(HDF5_HL_DLL_LOCATION hdf5_hl-shared IMPORTED_LOCATION_RELEASE)
	IF(NOT HDF5_HL_DLL_LOCATION)
		GET_TARGET_PROPERTY(HDF5_HL_DLL_LOCATION hdf5_hl IMPORTED_LOCATION_RELEASE)
	ENDIF()
	IF(NOT HDF5_HL_DLL_LOCATION)
		GET_TARGET_PROPERTY(HDF5_HL_DLL_LOCATION hdf5::hdf5_hl-shared IMPORTED_LOCATION_RELEASE)
	ENDIF()
	OVITO_INSTALL_DLL("${HDF5_HL_DLL_LOCATION}")

ELSEIF(UNIX AND NOT APPLE AND OVITO_REDISTRIBUTABLE_PACKAGE)

	# Deploy NetCDF shared library.
	GET_TARGET_PROPERTY(NETCDF_LIBRARY netcdf LOCATION)
	OVITO_INSTALL_SHARED_LIB("${NETCDF_LIBRARY}" "${OVITO_RELATIVE_LIBRARY_DIRECTORY}")

	# Deploy HDF5 shared libraries, which are required by NetCDF lib.
	GET_TARGET_PROPERTY(HDF5_LIBRARY hdf5-shared IMPORTED_LOCATION_RELEASE)
	OVITO_INSTALL_SHARED_LIB("${HDF5_LIBRARY}" "${OVITO_RELATIVE_LIBRARY_DIRECTORY}")
	GET_TARGET_PROPERTY(HDF5_HL_LIBRARY hdf5_hl-shared IMPORTED_LOCATION_RELEASE)
	OVITO_INSTALL_SHARED_LIB("${HDF5_HL_LIBRARY}" "${OVITO_RELATIVE_LIBRARY_DIRECTORY}")

ENDIF()

# Build corresponding GUI plugin.
IF(OVITO_BUILD_GUI)
	ADD_SUBDIRECTORY(gui)
ENDIF()

# Propagate list of plugins to parent scope.
SET(OVITO_PLUGIN_LIST ${OVITO_PLUGIN_LIST} PARENT_SCOPE)
