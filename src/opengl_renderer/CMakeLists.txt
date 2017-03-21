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

# Find OpenGL library.
FIND_PACKAGE(OpenGL REQUIRED)
ADD_LIBRARY(OpenGLLib SHARED IMPORTED GLOBAL)
SET_PROPERTY(TARGET OpenGLLib PROPERTY IMPORTED_LOCATION "${OPENGL_gl_LIBRARY}")
SET_PROPERTY(TARGET OpenGLLib APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${OPENGL_INCLUDE_DIR}")

SET(SourceFiles 
	OpenGLLinePrimitive.cpp
	OpenGLParticlePrimitive.cpp
	OpenGLTextPrimitive.cpp
	OpenGLImagePrimitive.cpp
	OpenGLArrowPrimitive.cpp
	OpenGLMeshPrimitive.cpp
	OpenGLMarkerPrimitive.cpp
	OpenGLSharedResource.cpp
	OpenGLSceneRenderer.cpp
	StandardSceneRenderer.cpp
)

# Compile resources.
QT5_ADD_RESOURCES(ResourceFiles resources/resources.qrc)

# Build library
ADD_LIBRARY(OpenGLRenderer SHARED ${SourceFiles} ${ResourceFiles})

# Pass name of gui plugin to the code. 
TARGET_COMPILE_DEFINITIONS(OpenGLRenderer PRIVATE "OVITO_PLUGIN_NAME=\"OpenGLRenderer\"")

# Set default include directory.
TARGET_INCLUDE_DIRECTORIES(OpenGLRenderer PUBLIC "$<BUILD_INTERFACE:${OVITO_SOURCE_BASE_DIR}/src>")

# Library is based on Core.
TARGET_LINK_LIBRARIES(OpenGLRenderer PUBLIC Core)

# Link to OpenGL library.
TARGET_LINK_LIBRARIES(OpenGLRenderer PRIVATE ${OPENGL_gl_LIBRARY})

# Link Qt5.
TARGET_LINK_LIBRARIES(OpenGLRenderer PUBLIC Qt5::Core Qt5::Gui)

IF(APPLE)
    # Activate @rpath on MacOS.
    SET_TARGET_PROPERTIES(OpenGLRenderer PROPERTIES MACOSX_RPATH TRUE)
	# This is required to avoid error by install_name_tool on MacOS.
	SET_TARGET_PROPERTIES(OpenGLRenderer PROPERTIES LINK_FLAGS "-headerpad_max_install_names")
ELSEIF(UNIX)
    SET_TARGET_PROPERTIES(OpenGLRenderer PROPERTIES INSTALL_RPATH "$ORIGIN")
ENDIF()
    
# This library is part of the installation package.
INSTALL(TARGETS OpenGLRenderer EXPORT OVITO
	RUNTIME DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}"
	LIBRARY DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}")

# Export target to make it accessible for external plugins.
IF(CMAKE_VERSION VERSION_LESS "3")
	EXPORT(TARGETS OpenGLRenderer NAMESPACE "Ovito::" APPEND FILE "${${PROJECT_NAME}_BINARY_DIR}/OVITOTargets.cmake")
ENDIF()

# Build corresponding GUI plugin.
IF(OVITO_BUILD_GUI)
	ADD_SUBDIRECTORY(gui)
ENDIF()

# Propagate list of plugins to parent scope.
SET(OVITO_PLUGIN_LIST ${OVITO_PLUGIN_LIST} PARENT_SCOPE)
