# Try to find the muParser library
#  MUPARSER_FOUND - system has muParser lib
#  MuParser_INCLUDE_DIRS - the include directories needed
#  MuParser_LIBRARIES - libraries needed

FIND_PATH(MUPARSER_INCLUDE_DIR NAMES muParser.h)
FIND_LIBRARY(MUPARSER_LIBRARY NAMES muparser)

SET(MuParser_INCLUDE_DIRS ${MUPARSER_INCLUDE_DIR})
SET(MuParser_LIBRARIES ${MUPARSER_LIBRARY})

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(MuParser DEFAULT_MSG MUPARSER_LIBRARY MUPARSER_INCLUDE_DIR)

MARK_AS_ADVANCED(MUPARSER_INCLUDE_DIR MUPARSER_LIBRARY)
