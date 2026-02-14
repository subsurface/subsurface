# install a few things so that one can run Subsurface from the build
# directory
# Use cmake -E commands for cross-platform compatibility (works on Windows/MSVC)
add_custom_target(themeLink ALL
	COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_BINARY_DIR}/theme
	COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/theme ${CMAKE_BINARY_DIR}/theme
)
if(NOT NO_PRINTING)
	add_custom_target(printing_templatesLink ALL
		COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_BINARY_DIR}/printing_templates
		COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/printing_templates ${CMAKE_BINARY_DIR}/printing_templates
	)
endif()
if(BUILD_DOCS OR INSTALL_DOCS)
	add_custom_target(
		install_documentation ALL
		COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_BINARY_DIR}/Documentation
		COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/Documentation/output ${CMAKE_BINARY_DIR}/Documentation
	)
endif()
if(BUILD_DOCS)
	add_custom_target(
		build_documentation ALL
		COMMAND
		${CMAKE_MAKE_PROGRAM} -C ${CMAKE_SOURCE_DIR}/Documentation doc
	)
	add_dependencies(install_documentation build_documentation)
endif()
