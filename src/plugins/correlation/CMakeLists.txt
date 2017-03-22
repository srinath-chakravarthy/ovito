###############################################################################
# 
#  Copyright (2017) Alexander Stukowski
#  Copyright (2016) Lars Pastewka
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

# Locate the FFTW library.
FIND_PACKAGE(FFTW3 REQUIRED)

SET(SourceFiles
	CorrelationFunctionModifier.cpp
)

OVITO_STANDARD_PLUGIN(
    CorrelationFunctionPlugin
    SOURCES ${SourceFiles}
    PLUGIN_DEPENDENCIES Particles
    PRIVATE_LIB_DEPENDENCIES ${FFTW3_LIBRARIES}
)

TARGET_INCLUDE_DIRECTORIES(CorrelationFunctionPlugin PRIVATE "${FFTW3_INCLUDE_DIRS}")

# Build corresponding GUI plugin.
IF(OVITO_BUILD_GUI)
	ADD_SUBDIRECTORY(gui)
ENDIF()

IF(WIN32)
	# Deploy FFTW3 dll.
	LIST(GET FFTW3_LIBRARIES 0 FFTW3_FIRST_LIBRARY)
	GET_FILENAME_COMPONENT(FFTW3_LIBRARY_DIR "${FFTW3_FIRST_LIBRARY}" PATH)
	GET_FILENAME_COMPONENT(FFTW3_LIBRARY_NAME "${FFTW3_FIRST_LIBRARY}" NAME_WE)
	OVITO_INSTALL_DLL("${FFTW3_LIBRARY_DIR}/${FFTW3_LIBRARY_NAME}.dll")
ELSEIF(UNIX AND NOT APPLE AND OVITO_REDISTRIBUTABLE_PACKAGE)
	# Deploy FFTW3 shared library.
	OVITO_INSTALL_SHARED_LIB("${FFTW3_LIBRARIES}" "${OVITO_RELATIVE_LIBRARY_DIRECTORY}")
ENDIF()

# Propagate list of plugins to parent scope.
SET(OVITO_PLUGIN_LIST ${OVITO_PLUGIN_LIST} PARENT_SCOPE)
