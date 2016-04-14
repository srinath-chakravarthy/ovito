#!
# @brief Add sources from directories
# @details
# Add the sources from the specified \p directories to variable \p var
# and place them in the specified \p folder if non empty.
# @param[out] var name of the variable that receives the result list
# @param[in] folder the name of the folder
# @param[in] directories list of directories to scan
#
function(aux_source_directories var folder)
    set(sources)
    foreach(dir ${ARGN})
        file(GLOB _sources RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${dir}/*.[ch]" "${dir}/*.[ch]pp")
        list(APPEND sources ${_sources})
    endforeach()
    
    if( NOT folder STREQUAL "")
        source_group(${folder} FILES ${sources})
    endif()

    set(${var} ${${var}} ${sources} PARENT_SCOPE)
endfunction()

aux_source_directories(SOURCES "Source Files"            .)
aux_source_directories(SOURCES "Source Files\\basic"     basic)
aux_source_directories(SOURCES "Source Files\\numerics"  numerics)
aux_source_directories(SOURCES "Source Files\\delaunay"  delaunay)
aux_source_directories(SOURCES "Source Files\\points"    points)
aux_source_directories(SOURCES "Source Files\\mesh"      mesh)
aux_source_directories(SOURCES "Source Files\\api"       api)

# Copy the default version info to the build directory
configure_file(basic/version.h.in version.h @ONLY)
list(APPEND SOURCES version.h)
set_source_files_properties(version.h PROPERTIES GENERATED true)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

add_library(geogram STATIC ${SOURCES})

# Make header files of this library available to dependent targets.
TARGET_INCLUDE_DIRECTORIES(geogram INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/..")
SET_PROPERTY(TARGET geogram PROPERTY POSITION_INDEPENDENT_CODE ON)
