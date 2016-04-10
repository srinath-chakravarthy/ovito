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

# Locate the OpenBabel library.
FIND_PACKAGE(OpenBabel REQUIRED)

SET(SourceFiles
	import/OpenBabelImporter.cpp
	import/CIFImporter.cpp
)

IF(OVITO_BUILD_PLUGIN_PYSCRIPT)
	LIST(APPEND SourceFiles PythonInterface.cpp)
ENDIF()

OVITO_STANDARD_PLUGIN(
	OpenBabelPlugin
	SOURCES ${SourceFiles}
	PLUGIN_DEPENDENCIES Particles
	OPTIONAL_PLUGIN_DEPENDENCIES PyScript	
	PYTHON_WRAPPERS "${CMAKE_CURRENT_SOURCE_DIR}/resources/python/"
	PRIVATE_LIB_DEPENDENCIES ${OpenBabel_LIBRARIES}
)

TARGET_INCLUDE_DIRECTORIES(OpenBabelPlugin PRIVATE "${OpenBabel_INCLUDE_DIRS}")

# Link to ZLib library, which is needed by OpenBabel.
TARGET_LINK_LIBRARIES(OpenBabelPlugin PRIVATE ${ZLIB_LIBRARIES})
