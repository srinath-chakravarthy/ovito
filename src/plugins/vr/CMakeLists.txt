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

# Locate the OpenVR library.
FIND_PACKAGE(OpenVR REQUIRED)

SET(SourceFiles
	StartVRAction.cpp
	VRWindow.cpp
	VRRenderingWidget.cpp
	VRSceneRenderer.cpp
	VRCacheModifier.cpp
	VRSettingsObject.cpp
	VRSettingsObjectEditor.cpp
)

OVITO_STANDARD_PLUGIN(VRPlugin
	SOURCES ${SourceFiles}
	GUI_PLUGIN
)

# Link to OpenVR library.
TARGET_INCLUDE_DIRECTORIES(VRPlugin PRIVATE ${OpenVR_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(VRPlugin PRIVATE ${OpenVR_LIBRARIES})

IF(WIN32)
	# Determine location of OpenVR DLL.
	LIST(GET OpenVR_LIBRARIES 0 OpenVR_FIRST_LIBRARY)
	GET_FILENAME_COMPONENT(OPENVR_LIBRARY_PATH "${OpenVR_FIRST_LIBRARY}" PATH)

	# Deploy OpenVR DLL.
	OVITO_INSTALL_DLL("${OPENVR_LIBRARY_PATH}/../../bin/win64/openvr_api.dll")
ENDIF()

# Propagate list of plugins to parent scope.
SET(OVITO_PLUGIN_LIST ${OVITO_PLUGIN_LIST} PARENT_SCOPE)