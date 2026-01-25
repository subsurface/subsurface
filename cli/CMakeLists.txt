# SPDX-License-Identifier: GPL-2.0
# CLI tool for webUI integration

set(CLI_SRCS
	main.cpp
	commands.cpp
	config.cpp
	dive_ref.cpp
	json_serializer.cpp
)

set(CLI_HDRS
	commands.h
	config.h
	dive_ref.h
	json_serializer.h
)

# Profile rendering needs Qt resources for event icons
if(USINGQT6)
	qt_add_resources(CLI_RESOURCES ${CMAKE_SOURCE_DIR}/profile.qrc)
else()
	qt5_add_resources(CLI_RESOURCES ${CMAKE_SOURCE_DIR}/profile.qrc)
endif()

add_executable(subsurface-cli ${CLI_SRCS} ${CLI_HDRS} ${CLI_RESOURCES})

target_link_libraries(subsurface-cli
	# backend_shared and profile have circular dependencies, so list
	# backend_shared twice for GNU ld's single-pass symbol resolution
	subsurface_backend_shared
	subsurface_profile
	subsurface_backend_shared
	subsurface_corelib
	${QT_LIBRARIES}
	${LIBDIVECOMPUTER_LIBRARIES}
	${LIBZIP_LIBRARIES}
	-lsqlite3
	-lxslt
	-lxml2
	-lgit2
)

install(TARGETS subsurface-cli DESTINATION bin)
