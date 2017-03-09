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

SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "OVITO (The Open Visualization Tool) is a scientific visualization and analysis software for atomistic simulation data.")
SET(CPACK_PACKAGE_VENDOR "ovito.org")
SET(CPACK_PACKAGE_CONTACT "Alexander Stukowski <mail@ovito.org>")
SET(CPACK_MONOLITHIC_INSTALL ON)
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.txt")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
SET(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.txt")
SET(CPACK_PACKAGE_VERSION_MAJOR ${OVITO_VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${OVITO_VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${OVITO_VERSION_REVISION})
SET(CPACK_DMG_VOLUME_NAME "OVITO ${OVITO_VERSION_STRING}")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "Ovito")

IF(UNIX AND NOT APPLE)
	SET(CPACK_GENERATOR "TGZ")
	SET(CPACK_INCLUDE_TOPLEVEL_DIRECTORY "1")
	EXECUTE_PROCESS(COMMAND "uname" "-m" OUTPUT_VARIABLE MACHINE_HARDWARE_NAME OUTPUT_STRIP_TRAILING_WHITESPACE)
	SET(CPACK_PACKAGE_FILE_NAME "ovito-${OVITO_VERSION_STRING}-${MACHINE_HARDWARE_NAME}")
ELSEIF(APPLE)
	SET(CPACK_GENERATOR "DragNDrop")
	SET(CPACK_PACKAGE_FILE_NAME "ovito-${OVITO_VERSION_STRING}-macos")
	SET(CPACK_SOURCE_STRIP_FILES OFF)
	SET(CPACK_STRIP_FILES OFF)
ELSEIF(WIN32)
	SET(CPACK_GENERATOR "NSIS")
	SET(CPACK_PACKAGE_FILE_NAME "ovito-${OVITO_VERSION_STRING}-win64")
	SET(CPACK_NSIS_INSTALLED_ICON_NAME "ovito.exe")
	SET(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
	SET(CPACK_NSIS_PACKAGE_NAME "Ovito")
	SET(CPACK_PACKAGE_EXECUTABLES "ovito" "OVITO (The Open Visualization Tool)")
	SET(CPACK_NSIS_CREATE_ICONS_EXTRA "
		File \\\"C:\\\\Program Files (x86)\\\\Microsoft Visual Studio 14.0\\\\VC\\\\redist\\\\1033\\\\vcredist_x64.exe\\\"
		ReadRegStr $1 HKLM \\\"SOFTWARE\\\\Microsoft\\\\DevDiv\\\\vc\\\\Servicing\\\\14.0\\\\RuntimeMinimum\\\" \\\"Install\\\"
		StrCmp $1 1 vc_runtime_installed
			ExecWait '\\\"$INSTDIR\\\\vcredist_x64.exe\\\"  /passive /norestart'
		vc_runtime_installed:
		Delete \\\"$INSTDIR\\\\vcredist_x64.exe\\\"
		")
ENDIF()

IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
	SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}_debug")
ENDIF()

IF(NOT WIN32)
	SET(CPACK_STRIP_FILES "${OVITO_RELATIVE_BINARY_DIRECTORY}/ovito")
	SET(CPACK_SOURCE_STRIP_FILES "")
ENDIF()
IF(APPLE OR WIN32)
	INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/README.txt" DESTINATION "${OVITO_RELATIVE_SHARE_DIRECTORY}")
	INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt" DESTINATION "${OVITO_RELATIVE_SHARE_DIRECTORY}")
	INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/CHANGELOG.txt" DESTINATION "${OVITO_RELATIVE_SHARE_DIRECTORY}")
ELSE()
	INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/README.txt" DESTINATION "./")
	INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt" DESTINATION "./")
	INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/CHANGELOG.txt" DESTINATION "./")
ENDIF()

INCLUDE(CPack)

IF(WIN32 AND OVITO_BUILD_APPSTORE_VERSION)
	# Package Ovito for the Windows Store.
	CONFIGURE_FILE("${OVITO_SOURCE_BASE_DIR}/src/main/resources/AppxManifest.xml" "${OVITO_BINARY_DIRECTORY}/AppxManifest.xml")
	INSTALL(FILES "${OVITO_BINARY_DIRECTORY}/appxmanifest.xml" DESTINATION "./")
	INSTALL(FILES "${OVITO_SOURCE_BASE_DIR}/src/gui/resources/mainwin/window_icon_44.png" DESTINATION "./")
	INSTALL(FILES "${OVITO_SOURCE_BASE_DIR}/src/gui/resources/mainwin/window_icon_150.png" DESTINATION "./")
		
	ADD_CUSTOM_TARGET(appx
					COMMAND "C:\\Program Files (x86)\\Windows Kits\\10\\bin\\x64\\MakeAppx.exe" pack /o /d "${CMAKE_INSTALL_PREFIX}" /p "${CMAKE_INSTALL_PREFIX}\\..\\ovito-${OVITO_VERSION_STRING}.appx"
					#COMMAND SignTool sign /fd SHA256 /a /f "${CMAKE_INSTALL_PREFIX}\\..\\..\\ovito.pfx" "${CMAKE_INSTALL_PREFIX}\\..\\ovito-${OVITO_VERSION_STRING}.appx"
					COMMENT "Creating AppX package")

ENDIF()
