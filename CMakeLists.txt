###############################################################################
# 
#  Copyright (2014) Alexander Stukowski
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

PROJECT(Ovito)

# Make sure we have a recent version of CMake.
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12 FATAL_ERROR)
CMAKE_POLICY(VERSION 2.8.12)

# Choose CMake policies.
CMAKE_POLICY(SET CMP0023 OLD)   # Allow keyword and plain target_link_libraries signatures to be mixed.
CMAKE_POLICY(SET CMP0020 NEW) 	# Automatically link to qtmain.lib on Windows.
IF(NOT CMAKE_VERSION VERSION_LESS "3")
	CMAKE_POLICY(SET CMP0026 OLD)	# Suppress warning when reading the LOCATION target property.
	CMAKE_POLICY(SET CMP0043 OLD)	# Suppress warning because COMPILE_DEFINITIONS_* target property is used.
	CMAKE_POLICY(SET CMP0045 OLD)	# Don't warn about non-existing targets in GET_TARGET_PROPERTY().
ENDIF()

# Check compiler requirement.
IF(CMAKE_CXX_COMPILER_ID STREQUAL MSVC)
	IF(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "18")
		MESSAGE(FATAL_ERROR "OVITO requires Visual C++ 2013 or newer.")
	ENDIF()
ENDIF()

# This is to enable target debugging within Visual Studio Code. 
# It is ignored oustide of Visual Studio Code/CMake tools. 
INCLUDE(CMakeToolsHelpers OPTIONAL)

SET(OVITO_SOURCE_BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
LIST(APPEND CMAKE_MODULE_PATH ${OVITO_SOURCE_BASE_DIR}/cmake)
INCLUDE(${OVITO_SOURCE_BASE_DIR}/cmake/Version.cmake)
INCLUDE(${OVITO_SOURCE_BASE_DIR}/cmake/Plugins.cmake)
INCLUDE(${OVITO_SOURCE_BASE_DIR}/cmake/PrecompiledHeader.cmake)

# Define build options.
OPTION(OVITO_DOUBLE_PRECISION_FP "Use double-precision floating-point numbers." "OFF")
OPTION(OVITO_BUILD_GUI "Build the graphical user interface." "ON")
OPTION(OVITO_BUILD_APPSTORE_VERSION "Build binaries for the Apple App Store and Windows Store." "OFF")

# Define user options that control the building of OVITO's standard plugins.
OPTION(OVITO_BUILD_PLUGIN_PARTICLES "Build the Particles plugin." "ON")
OPTION(OVITO_BUILD_PLUGIN_MESH "Build the Mesh plugin." "ON")
OPTION(OVITO_BUILD_PLUGIN_TACHYON "Build the Tachyon renderer plugin." "ON")
OPTION(OVITO_BUILD_PLUGIN_CRYSTALANALYSIS "Build the CrystalAnalysis plugin." "ON")
OPTION(OVITO_BUILD_PLUGIN_PYSCRIPT "Build the Python scripting plugin." "ON")
OPTION(OVITO_BUILD_PLUGIN_NETCDFPLUGIN "Build the NetCDF plugin." "ON")
OPTION(OVITO_BUILD_PLUGIN_POVRAY "Build the POV-Ray plugin." "ON")
OPTION(OVITO_BUILD_PLUGIN_OPENBABELPLUGIN "Build the OpenBabel plugin (experimental)." "OFF")
OPTION(OVITO_BUILD_PLUGIN_CORRELATION "Build the correlation function plugin." "ON")
OPTION(OVITO_BUILD_PLUGIN_VR "Build the virtual reality plugin (experimental)." "OFF")

# This is a global list of plugin targets that will be built.
# It will get populated by the OVITO_PLUGIN macro.
SET(OVITO_PLUGINS_LIST "")

# Enable software testing framework.
ENABLE_TESTING()

# Activate C++11 language standard.
IF(NOT CMAKE_VERSION VERSION_LESS "3.1")
	SET(CMAKE_CXX_STANDARD 11)
	SET(CMAKE_CXX_STANDARD_REQUIRED ON)
ELSE()
	IF(NOT MSVC)
		IF(CMAKE_COMPILER_IS_GNUCXX)
		    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
		ELSE()
	    	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
		ENDIF()
	ENDIF()
ENDIF()

IF(CYGWIN)
	IF(CMAKE_COMPILER_IS_GNUCXX)
		# Linking fails without -O3
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
	ENDIF()
ENDIF()

IF(APPLE)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -Wno-invalid-offsetof")
ENDIF()

IF(MSVC)
	# Suppress warning related to QVarLengthArray implementation.
	ADD_COMPILE_OPTIONS("/wd4996")
	# Suppress warning on conversion from size_t to int, possible loss of data.
	ADD_COMPILE_OPTIONS("/wd4267")
	# Compiling template code leads to large object files.
	ADD_COMPILE_OPTIONS("/bigobj")	
ENDIF()

IF(UNIX)
	IF(APPLE)
		SET(MACOSX_BUNDLE_NAME "Ovito") 
		SET(MACOSX_BUNDLE_BUNDLE_NAME "${MACOSX_BUNDLE_NAME}") 
		
		# The directory where the main executable goes to.
		SET(OVITO_RELATIVE_BINARY_DIRECTORY ".")
		# The directory where the main libraries go to.
		SET(OVITO_RELATIVE_LIBRARY_DIRECTORY "${MACOSX_BUNDLE_NAME}.app/Contents/MacOS")
		# The directory where the auxiliary files go to.
		SET(OVITO_RELATIVE_SHARE_DIRECTORY "${MACOSX_BUNDLE_NAME}.app/Contents/Resources")
		# The directory where the compiled plugins go to.
		SET(OVITO_RELATIVE_PLUGINS_DIRECTORY "${MACOSX_BUNDLE_NAME}.app/Contents/PlugIns")	
		# The directory where the Python source modules go to.
		SET(OVITO_RELATIVE_PYTHON_DIRECTORY "${MACOSX_BUNDLE_NAME}.app/Contents/Resources/python")	
	ELSE()
		# The directory where the main executable goes to.
		SET(OVITO_RELATIVE_BINARY_DIRECTORY "bin")
		# The directory where the main libraries go to.
		SET(OVITO_RELATIVE_LIBRARY_DIRECTORY "lib/ovito")
		# The directory where the auxiliary files go to.
		SET(OVITO_RELATIVE_SHARE_DIRECTORY "share/ovito")
		# The directory where the compiled plugins go to.
		SET(OVITO_RELATIVE_PLUGINS_DIRECTORY "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/plugins")
		# The directory where the Python source modules go to.
		SET(OVITO_RELATIVE_PYTHON_DIRECTORY "${OVITO_RELATIVE_PLUGINS_DIRECTORY}/python")
	ENDIF()
ELSE()
	# The directory where the main executable goes to.
	SET(OVITO_RELATIVE_BINARY_DIRECTORY ".")
	# The directory where the main libraries go to.
	SET(OVITO_RELATIVE_LIBRARY_DIRECTORY ".")
	# The directory where the auxiliary files go to.
	SET(OVITO_RELATIVE_SHARE_DIRECTORY ".")
	# The directory where the compiled plugins go to.
	SET(OVITO_RELATIVE_PLUGINS_DIRECTORY "${OVITO_RELATIVE_LIBRARY_DIRECTORY}/plugins")
		# The directory where the Python source modules go to.
	SET(OVITO_RELATIVE_PYTHON_DIRECTORY "${OVITO_RELATIVE_PLUGINS_DIRECTORY}/python")	
ENDIF()

# Controls whether a redistributable installation package of OVITO is built.
OPTION(OVITO_REDISTRIBUTABLE_PACKAGE "Create a redistributable program package that includes third-party libraries." "OFF")

IF(APPLE OR WIN32 OR NOT OVITO_REDISTRIBUTABLE_PACKAGE)
	SET(OVITO_DEFAULT_LIBRARY_TYPE SHARED)
    
	# Add the automatically determined parts of the RPATH,
	# which point to directories outside the build tree to the install RPATH
	SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
	    
ELSE()
	SET(OVITO_DEFAULT_LIBRARY_TYPE STATIC)
ENDIF()

# The directory where the main executable goes to.
SET(OVITO_BINARY_DIRECTORY "${${PROJECT_NAME}_BINARY_DIR}/${OVITO_RELATIVE_BINARY_DIRECTORY}")
# The directory where the main libraries go to.
SET(OVITO_LIBRARY_DIRECTORY "${${PROJECT_NAME}_BINARY_DIR}/${OVITO_RELATIVE_LIBRARY_DIRECTORY}")
# The directory where the compiled plugins go to.
SET(OVITO_PLUGINS_DIRECTORY "${${PROJECT_NAME}_BINARY_DIR}/${OVITO_RELATIVE_PLUGINS_DIRECTORY}")
# The directory where the Python source files go to.
SET(OVITO_PYTHON_DIRECTORY "${${PROJECT_NAME}_BINARY_DIR}/${OVITO_RELATIVE_PYTHON_DIRECTORY}")
# The directory where the auxiliary files go to.
SET(OVITO_SHARE_DIRECTORY "${${PROJECT_NAME}_BINARY_DIR}/${OVITO_RELATIVE_SHARE_DIRECTORY}")

# This is the name suffix used for generating plugin libraries.
SET(OVITO_PLUGIN_LIBRARY_SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX})
IF(APPLE)
	# On Mac OS, we use the .so extension instead of the standard .dylib to be compatible
	# with the Python interpreter, which only finds modules having a .so suffix.
	SET(OVITO_PLUGIN_LIBRARY_SUFFIX ".so")
ENDIF()

# This macro installs a required third-party DLL in the OVITO directory
# so that it can be distributed together with the program.
MACRO(OVITO_INSTALL_DLL dll)
	MESSAGE("Deploying DLL ${dll}.")
	EXECUTE_PROCESS(COMMAND "${CMAKE_COMMAND}" "-E" "copy_if_different" "${dll}" "${OVITO_BINARY_DIRECTORY}" RESULT_VARIABLE error_var)
	IF(error_var)
		MESSAGE(FATAL_ERROR "Failed to copy DLL into build directory: ${dll}")
	ENDIF()
	INSTALL(FILES "${dll}" DESTINATION "${OVITO_RELATIVE_BINARY_DIRECTORY}")
ENDMACRO()

# This macro installs a required third-party shared library in the OVITO directory
# so that it can be distributed together with the program.
MACRO(OVITO_INSTALL_SHARED_LIB shared_lib destination_dir)
	FILE(MAKE_DIRECTORY "${${PROJECT_NAME}_BINARY_DIR}/${destination_dir}")
	# Strip version number from shared lib filename.
	GET_FILENAME_COMPONENT(shared_lib_ext "${shared_lib}" EXT)
	STRING(REPLACE ${shared_lib_ext} ".so" shared_lib_new "${shared_lib}")
	FILE(GLOB lib_versions "${shared_lib_new}*")
	FOREACH(lib_version ${lib_versions})
		IF(IS_SYMLINK ${lib_version})
			GET_FILENAME_COMPONENT(symlink_target "${lib_version}" REALPATH)
			GET_FILENAME_COMPONENT(symlink_target_name "${symlink_target}" NAME)
			GET_FILENAME_COMPONENT(lib_version_name "${lib_version}" NAME)
			MESSAGE("Installing symlink ${lib_version} to shared library ${symlink_target}")
			EXECUTE_PROCESS(COMMAND "${CMAKE_COMMAND}" -E create_symlink ${symlink_target_name} "${${PROJECT_NAME}_BINARY_DIR}/${destination_dir}/${lib_version_name}")
		ELSE()
			MESSAGE("Installing shared library ${lib_version}")
			CONFIGURE_FILE("${lib_version}" "${${PROJECT_NAME}_BINARY_DIR}/${destination_dir}/" COPYONLY)
		ENDIF()
		INSTALL(FILES "${lib_version}" DESTINATION "${destination_dir}/")
	ENDFOREACH()
ENDMACRO()

# Tell CMake to run Qt moc whenever necessary.
SET(CMAKE_AUTOMOC ON)
# As moc files are generated in the binary dir, tell CMake to always look for includes there.
SET(CMAKE_INCLUDE_CURRENT_DIR ON)
  
# Find and set up Qt5 libraries.
FOREACH(component IN ITEMS Core Concurrent Network Gui)
	FIND_PACKAGE(Qt5${component} REQUIRED)
ENDFOREACH()
IF(OVITO_BUILD_GUI) 
	FOREACH(component IN ITEMS Widgets PrintSupport)
		FIND_PACKAGE(Qt5${component} REQUIRED)
	ENDFOREACH()
	IF(UNIX AND NOT APPLE AND CMAKE_COMPILER_IS_GNUCXX)
		# This is an indirect dependency:
		FIND_PACKAGE(Qt5DBus REQUIRED)
	ENDIF()
ENDIF()

# Find the Boost library.
FIND_PACKAGE(Boost REQUIRED)
IF(NOT Boost_FOUND)
	MESSAGE(FATAL_ERROR "The Boost library could not be found on your system. ${Boost_ERROR_REASON}")
ENDIF()

# Generate build targets.
ADD_SUBDIRECTORY(src)

# Gnerate user documentation.
INCLUDE(cmake/Documentation.cmake)

# Set up CPack for generating distributable program packages.
INCLUDE(cmake/Packaging.cmake)

# Export our targets so that external plugins can use them.
CONFIGURE_FILE(cmake/OVITOConfig.cmake "${${PROJECT_NAME}_BINARY_DIR}/" @ONLY)
IF(NOT CMAKE_VERSION VERSION_LESS "3")
	EXPORT(EXPORT OVITO NAMESPACE "Ovito::" FILE OVITOTargets.cmake)
ENDIF()
