# install a few things so that one can run Subsurface from the build
# directory
# Use cmake -E commands for cross-platform compatibility (works on Windows/MSVC)
# On Windows/MSVC, use copy_directory instead of symlinks (symlinks require admin privileges)
if(MSVC)
	add_custom_target(themeLink ALL
		COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_BINARY_DIR}/theme
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/theme ${CMAKE_BINARY_DIR}/theme
	)
else()
	add_custom_target(themeLink ALL
		COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_BINARY_DIR}/theme
		COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/theme ${CMAKE_BINARY_DIR}/theme
	)
endif()
if(NOT NO_PRINTING)
	if(MSVC)
		add_custom_target(printing_templatesLink ALL
			COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_BINARY_DIR}/printing_templates
			COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/printing_templates ${CMAKE_BINARY_DIR}/printing_templates
		)
	else()
		add_custom_target(printing_templatesLink ALL
			COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_BINARY_DIR}/printing_templates
			COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/printing_templates ${CMAKE_BINARY_DIR}/printing_templates
		)
	endif()
endif()
if(BUILD_DOCS OR INSTALL_DOCS)
	if(MSVC)
		add_custom_target(
			install_documentation ALL
			COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_BINARY_DIR}/Documentation
			COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/Documentation/output ${CMAKE_BINARY_DIR}/Documentation
		)
	else()
		add_custom_target(
			install_documentation ALL
			COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_BINARY_DIR}/Documentation
			COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/Documentation/output ${CMAKE_BINARY_DIR}/Documentation
		)
	endif()
endif()
if(BUILD_DOCS)
	add_custom_target(
		build_documentation ALL
		COMMAND
		${CMAKE_MAKE_PROGRAM} -C ${CMAKE_SOURCE_DIR}/Documentation doc
	)
	add_dependencies(install_documentation build_documentation)
endif()
