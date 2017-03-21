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

IF(OVITO_BUILD_PLUGIN_PYSCRIPT)
	ADD_SUBDIRECTORY(pyscript)
	SET(OVITOS_EXECUTABLE ${OVITOS_EXECUTABLE} PARENT_SCOPE)
ENDIF()

IF(OVITO_BUILD_PLUGIN_PARTICLES)
	ADD_SUBDIRECTORY(particles)
ENDIF()

IF(OVITO_BUILD_PLUGIN_TACHYON)
	ADD_SUBDIRECTORY(tachyon)
ENDIF()

IF(OVITO_BUILD_PLUGIN_MESH)
	ADD_SUBDIRECTORY(mesh)
ENDIF()

IF(OVITO_BUILD_PLUGIN_CRYSTALANALYSIS)
	ADD_SUBDIRECTORY(crystalanalysis)
ENDIF()

IF(OVITO_BUILD_PLUGIN_NETCDFPLUGIN)
	ADD_SUBDIRECTORY(netcdf)
ENDIF()

IF(OVITO_BUILD_PLUGIN_POVRAY)
	ADD_SUBDIRECTORY(povray)
ENDIF()

IF(OVITO_BUILD_PLUGIN_OPENBABELPLUGIN)
	ADD_SUBDIRECTORY(openbabel)
ENDIF()

IF(OVITO_BUILD_PLUGIN_VR)
	ADD_SUBDIRECTORY(vr)
ENDIF()

IF(OVITO_BUILD_PLUGIN_CORRELATION)
	ADD_SUBDIRECTORY(correlation)
ENDIF()

# Propagate list of plugins to parent scope.
SET(OVITO_PLUGIN_LIST ${OVITO_PLUGIN_LIST} PARENT_SCOPE)
