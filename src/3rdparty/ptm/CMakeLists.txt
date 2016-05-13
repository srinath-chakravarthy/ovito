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

SET(PTM_SOURCE_FILES
	alloy_types.c
	canonical.c
	convex_hull_incremental.c
	deformation_gradient.c
	graph_data.c
	index_PTM.c
	neighbour_ordering.c
	normalize_vertices.c
	qcprot.c
	quat.c
	svdpolar/polar_decomposition.c
)

# Build library.
ADD_LIBRARY(PolyhedralTemplateMatching STATIC ${PTM_SOURCE_FILES})

# Make header files of this library available to dependent targets.
TARGET_INCLUDE_DIRECTORIES(PolyhedralTemplateMatching INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/..")

# Since we will link this into the Particles plugin, we need to use the same setting for the fPIC flag.
SET_PROPERTY(TARGET PolyhedralTemplateMatching PROPERTY POSITION_INDEPENDENT_CODE ON)

# Export this target.
INSTALL(TARGETS PolyhedralTemplateMatching EXPORT OVITO 
    ARCHIVE DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}" 
    CONFIGURATIONS "")
