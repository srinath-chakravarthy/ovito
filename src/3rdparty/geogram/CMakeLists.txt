# @brief Add sources from directories
# @details
# Add the sources from the specified \p directories to variable \p var
# and place them in the specified \p folder if non empty.
# @param[out] var name of the variable that receives the result list
# @param[in] folder the name of the folder
# @param[in] directories list of directories to scan
 
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
aux_source_directories(SOURCES "Source Files\\mesh"      mesh)
aux_source_directories(SOURCES "Source Files\\api"       api)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/..)
ADD_LIBRARY(geogram STATIC ${SOURCES})

# Make header files of this library available to dependent targets.
TARGET_INCLUDE_DIRECTORIES(geogram INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/..")
SET_PROPERTY(TARGET geogram PROPERTY POSITION_INDEPENDENT_CODE ON)

# Export this target.
INSTALL(TARGETS geogram EXPORT OVITO 
    ARCHIVE DESTINATION "${OVITO_RELATIVE_LIBRARY_DIRECTORY}" 
    CONFIGURATIONS "")
