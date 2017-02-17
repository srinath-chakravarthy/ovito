###############################################################################
# 
#  Copyright (2015) Alexander Stukowski
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

# Create the plugins library and manifest directories.
FILE(MAKE_DIRECTORY "${OVITO_PLUGINS_DIRECTORY}")
FILE(MAKE_DIRECTORY "${OVITO_PLUGIN_MANIFESTS_DIRECTORY}")

IF(OVITO_BUILD_PLUGIN_PYSCRIPT)
	ADD_SUBDIRECTORY(pyscript)
	SET(OVITOS_EXECUTABLE ${OVITOS_EXECUTABLE} PARENT_SCOPE)
ELSE()
	FILE(REMOVE ${OVITO_PLUGIN_MANIFESTS_DIRECTORY}/PyScript.json)
	FILE(REMOVE ${OVITO_PLUGIN_MANIFESTS_DIRECTORY}/PyScriptGui.json)
ENDIF()

IF(OVITO_BUILD_PLUGIN_PARTICLES)
	ADD_SUBDIRECTORY(particles)
ELSE()
	FILE(REMOVE ${OVITO_PLUGIN_MANIFESTS_DIRECTORY}/Particles.json)
	FILE(REMOVE ${OVITO_PLUGIN_MANIFESTS_DIRECTORY}/ParticlesGui.json)
ENDIF()

IF(OVITO_BUILD_PLUGIN_TACHYON)
	ADD_SUBDIRECTORY(tachyon)
ELSE()
	FILE(REMOVE ${OVITO_PLUGIN_MANIFESTS_DIRECTORY}/Tachyon.json)
	FILE(REMOVE ${OVITO_PLUGIN_MANIFESTS_DIRECTORY}/TachyonGui.json)
ENDIF()

IF(OVITO_BUILD_PLUGIN_MESH)
	ADD_SUBDIRECTORY(mesh)
ELSE()
	FILE(REMOVE ${OVITO_PLUGIN_MANIFESTS_DIRECTORY}/Mesh.json)
ENDIF()

IF(OVITO_BUILD_PLUGIN_CRYSTALANALYSIS)
	ADD_SUBDIRECTORY(crystalanalysis)
ELSE()
	FILE(REMOVE ${OVITO_PLUGIN_MANIFESTS_DIRECTORY}/CrystalAnalysis.json)
	FILE(REMOVE ${OVITO_PLUGIN_MANIFESTS_DIRECTORY}/CrystalAnalysisGui.json)
ENDIF()

IF(OVITO_BUILD_PLUGIN_NETCDFPLUGIN)
	ADD_SUBDIRECTORY(netcdf)
ELSE()
	FILE(REMOVE ${OVITO_PLUGIN_MANIFESTS_DIRECTORY}/NetCDFPlugin.json)
	FILE(REMOVE ${OVITO_PLUGIN_MANIFESTS_DIRECTORY}/NetCDFPluginGui.json)
ENDIF()

IF(OVITO_BUILD_PLUGIN_POVRAY)
	ADD_SUBDIRECTORY(povray)
ELSE()
	FILE(REMOVE ${OVITO_PLUGIN_MANIFESTS_DIRECTORY}/POVRay.json)
	FILE(REMOVE ${OVITO_PLUGIN_MANIFESTS_DIRECTORY}/POVRayGui.json)
ENDIF()

IF(OVITO_BUILD_PLUGIN_OPENBABELPLUGIN)
	ADD_SUBDIRECTORY(openbabel)
ELSE()
	FILE(REMOVE ${OVITO_PLUGIN_MANIFESTS_DIRECTORY}/OpenBabelPlugin.json)
ENDIF()

IF(OVITO_BUILD_PLUGIN_VR)
	ADD_SUBDIRECTORY(vr)
ELSE()
	FILE(REMOVE ${OVITO_PLUGIN_MANIFESTS_DIRECTORY}/VRPlugin.json)
ENDIF()

# Propagate list of plugins to parent scope.
SET(OVITO_PLUGIN_LIST ${OVITO_PLUGIN_LIST} PARENT_SCOPE)
