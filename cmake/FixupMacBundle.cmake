IF(APPLE)
	# Install the Info.plist file.
	CONFIGURE_FILE("${OVITO_SOURCE_BASE_DIR}/src/main/resources/Info.plist" "${OVITO_BINARY_DIRECTORY}/${MACOSX_BUNDLE_NAME}.app/Contents/Info.plist")
	SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${OVITO_BINARY_DIRECTORY}/${MACOSX_BUNDLE_NAME}.app/Contents/Info.plist")

	# Copy the application icon into the resource directory.
	INSTALL(FILES "${OVITO_SOURCE_BASE_DIR}/src/main/resources/ovito.icns" DESTINATION "${OVITO_RELATIVE_SHARE_DIRECTORY}")

	SET(QT_PLUGINS_DIR "${_qt5Core_install_prefix}/plugins")

	# Install needed Qt plugins by copying directories from the qt installation
	# One can cull what gets copied by using 'REGEX "..." EXCLUDE'
	SET(plugin_dest_dir "${MACOSX_BUNDLE_NAME}.app/Contents/PlugIns")
	SET(qtconf_dest_dir "${MACOSX_BUNDLE_NAME}.app/Contents/Resources")
	INSTALL(DIRECTORY "${QT_PLUGINS_DIR}/imageformats" DESTINATION ${plugin_dest_dir} COMPONENT Runtime PATTERN "*_debug.dylib" EXCLUDE)
	INSTALL(DIRECTORY "${QT_PLUGINS_DIR}/platforms" DESTINATION ${plugin_dest_dir} COMPONENT Runtime PATTERN "*_debug.dylib" EXCLUDE)

	# Install a qt.conf file.
	# This inserts some cmake code into the install script to write the file
	INSTALL(CODE "
	    file(WRITE \"\${CMAKE_INSTALL_PREFIX}/${qtconf_dest_dir}/qt.conf\" \"[Paths]\\nPlugins = PlugIns/\")
	    " COMPONENT Runtime)

	# Purge any previous version of the nested bundle to avoid errors during bundle fixup.
	IF(OVITO_BUILD_PLUGIN_PYSCRIPT)
		INSTALL(CODE "
			FILE(REMOVE_RECURSE \"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app/Contents/MacOS/Ovito.App\")
			" COMPONENT Runtime)
	ENDIF()

	# Use BundleUtilities to get all other dependencies for the application to work.
	# It takes a bundle or executable along with possible plugins and inspects it
	# for dependencies.  If they are not system dependencies, they are copied.
	
	# Now the work of copying dependencies into the bundle/package
	# The quotes are escaped and variables to use at install time have their $ escaped
	# An alternative is the do a configure_file() on a script and use install(SCRIPT  ...).
	# Note that the image plugins depend on QtSvg and QtXml, and it got those copied
	# over.
	INSTALL(CODE "
		CMAKE_POLICY(SET CMP0011 NEW)
		CMAKE_POLICY(SET CMP0009 NEW)

		# Use BundleUtilities to get all other dependencies for the application to work.
		# It takes a bundle or executable along with possible plugins and inspects it
		# for dependencies.  If they are not system dependencies, they are copied.
		SET(APPS \"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app\")

		# Directories to look for dependencies
		SET(DIRS 
			${QT_LIBRARY_DIRS} 
			\"\${CMAKE_INSTALL_PREFIX}/${OVITO_RELATIVE_PLUGINS_DIRECTORY}\" 
			\"\${CMAKE_INSTALL_PREFIX}/${plugin_dest_dir}/imageformats\"
			\"\${CMAKE_INSTALL_PREFIX}/${plugin_dest_dir}/platforms\"
			/opt/local/lib)

		# Returns the path that others should refer to the item by when the item is embedded inside a bundle.
		# This ensures that all plugin libraries go into the plugins/ directory of the bundle.
		FUNCTION(gp_item_default_embedded_path_override item default_embedded_path_var)
		    # Embed plugin libraries (.so) in the PlugIns/ subdirectory:
            IF(item MATCHES \"\\\\${OVITO_PLUGIN_LIBRARY_SUFFIX}$\" AND (item MATCHES \"^@rpath\" OR item MATCHES \"PlugIns/\"))
	    	    SET(path \"@executable_path/../PlugIns\")
		        SET(\${default_embedded_path_var} \"\${path}\" PARENT_SCOPE)
    		    MESSAGE(\"     Embedding path override: \${item} -> \${path}\")
            ENDIF()
		ENDFUNCTION()
		# This is needed to correctly install Matplotlib's shared libraries in the .dylibs/ subdirectory:
		FUNCTION(gp_resolved_file_type_override resolved_file type)
		    IF(resolved_file MATCHES \"@loader_path/\" AND resolved_file MATCHES \"/.dylibs/\")
				SET(\${type} \"system\" PARENT_SCOPE)
			ENDIF()
		ENDFUNCTION()
		FILE(GLOB_RECURSE QTPLUGINS
			\"\${CMAKE_INSTALL_PREFIX}/${plugin_dest_dir}/*${CMAKE_SHARED_LIBRARY_SUFFIX}\")
		FILE(GLOB_RECURSE OVITO_PLUGINS
			\"\${CMAKE_INSTALL_PREFIX}/${OVITO_RELATIVE_PLUGINS_DIRECTORY}/*${OVITO_PLUGIN_LIBRARY_SUFFIX}\")
		FILE(GLOB_RECURSE PYTHON_DYNLIBS
			\"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app/Contents/Frameworks/Python.framework/*.so\")
		SET(BUNDLE_LIBS \${QTPLUGINS} \${OVITO_PLUGINS} \${PYTHON_DYNLIBS})
		SET(BU_CHMOD_BUNDLE_ITEMS ON)	# Make copies of system libraries writable before install_name_tool tries to change them.
		INCLUDE(BundleUtilities)
		FIXUP_BUNDLE(\"\${APPS}\" \"\${BUNDLE_LIBS}\" \"\${DIRS}\" IGNORE_ITEM \"Python\")
		" COMPONENT Runtime)

	IF(OVITO_BUILD_PLUGIN_PYSCRIPT)
		# Remove __pycache__ files from installation bundle.
		INSTALL(CODE "
			MESSAGE(\"Removing __pycache__ files.\")
			EXECUTE_PROCESS(COMMAND find \"\${CMAKE_INSTALL_PREFIX}\" -name __pycache__ -delete)
			" COMPONENT Runtime)

		# Create a nested bundle for 'ovitos'.
		# This is to prevent the program icon from showing up in the dock when 'ovitos' is run.
		INSTALL(CODE "
			EXECUTE_PROCESS(COMMAND \"\${CMAKE_COMMAND}\" -E make_directory \"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app/Contents/MacOS/Ovito.App/Contents/MacOS\")
			FILE(RENAME \"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app/Contents/MacOS/ovitos.exe\" \"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app/Contents/MacOS/Ovito.App/Contents/MacOS/ovitos\")
			EXECUTE_PROCESS(COMMAND \"\${CMAKE_COMMAND}\" -E create_symlink \"../../../Resources\" \"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app/Contents/MacOS/Ovito.App/Contents/Resources\")
			EXECUTE_PROCESS(COMMAND \"\${CMAKE_COMMAND}\" -E create_symlink \"../../../Frameworks\" \"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app/Contents/MacOS/Ovito.App/Contents/Frameworks\")
			EXECUTE_PROCESS(COMMAND \"\${CMAKE_COMMAND}\" -E create_symlink \"../../../PlugIns\" \"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app/Contents/MacOS/Ovito.App/Contents/PlugIns\")
			CONFIGURE_FILE(\"${CMAKE_SOURCE_DIR}/src/main/resources/Info.plist\" \"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app/Contents/MacOS/Ovito.App/Contents/Info.plist\")
			EXECUTE_PROCESS(COMMAND defaults write \"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app/Contents/MacOS/Ovito.App/Contents/Info\" LSUIElement 1)
			FILE(GLOB DylibsToSymlink \"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app/Contents/MacOS/*.dylib\")
			FOREACH(FILE_ENTRY \${DylibsToSymlink})
				GET_FILENAME_COMPONENT(FILE_ENTRY_NAME \"\${FILE_ENTRY}\" NAME)
				EXECUTE_PROCESS(COMMAND \"\${CMAKE_COMMAND}\" -E create_symlink \"../../../\${FILE_ENTRY_NAME}\" \"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app/Contents/MacOS/Ovito.App/Contents/MacOS/\${FILE_ENTRY_NAME}\")
			ENDFOREACH()
		" COMPONENT Runtime)
	ENDIF()	

	# Sign bundle (starting from the inside out with all executables/libraries, 
	# then the nested 'ovitos' bundle, finally the main bundle).
	SET(SigningIdentity "Alexander Stukowski")
	INSTALL(CODE "
		CMAKE_POLICY(SET CMP0012 NEW)

		SET(FilesToBeSigned)
		IF(${OVITO_BUILD_PLUGIN_PYSCRIPT})
			LIST(APPEND FilesToBeSigned \"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app/Contents/MacOS/Ovito.app/Contents/MacOS/ovitos\")
			LIST(APPEND FilesToBeSigned \"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app/Contents/MacOS/ovitos\")
			FILE(GLOB_RECURSE PythonDylibsToBeSigned \"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app/Contents/Frameworks/Python.framework/Versions/*.dylib\")
			LIST(APPEND FilesToBeSigned \${PythonDylibsToBeSigned})
		ENDIF()
		FILE(GLOB FrameworksToBeSigned \"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app/Contents/Frameworks/*.framework\")
		LIST(APPEND FilesToBeSigned \${FrameworksToBeSigned})
		FILE(GLOB_RECURSE DylibsToBeSigned \"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app/Contents/PlugIns/*.dylib\")
		LIST(APPEND FilesToBeSigned \${DylibsToBeSigned})
		FILE(GLOB_RECURSE DylibsToBeSigned \"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app/Contents/MacOS/*.dylib\")
		LIST(APPEND FilesToBeSigned \${DylibsToBeSigned})
		FILE(GLOB SolibsToBeSigned \"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app/Contents/PlugIns/*.so\")
		LIST(APPEND FilesToBeSigned \${SolibsToBeSigned})
		IF(${OVITO_BUILD_PLUGIN_PYSCRIPT})
			LIST(APPEND FilesToBeSigned \"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app/Contents/MacOS/Ovito.app\")
		ENDIF()
		LIST(APPEND FilesToBeSigned \"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app\")

		# Sign executable files one by one.
		FOREACH(FILE_ENTRY \${FilesToBeSigned})
			MESSAGE(\"Signing \${FILE_ENTRY}\")
			EXECUTE_PROCESS(COMMAND codesign -s \"${SigningIdentity}\" --force \"\${FILE_ENTRY}\")
		ENDFOREACH()

		# Verify signing process:
		EXECUTE_PROCESS(COMMAND codesign --verify --verbose --deep \"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app\")
		EXECUTE_PROCESS(COMMAND spctl --assess --type execute \"\${CMAKE_INSTALL_PREFIX}/${MACOSX_BUNDLE_NAME}.app\")
		" COMPONENT Runtime)

ENDIF()
