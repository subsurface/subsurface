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

add_executable(subsurface-cli ${CLI_SRCS} ${CLI_HDRS})

target_link_libraries(subsurface-cli
	subsurface_corelib
	${QT_LIBRARIES}
	${LIBDIVECOMPUTER_LIBRARIES}
	${LIBGIT2_LIBRARIES}
	${LIBXSLT_LIBRARIES}
	${LIBXML_LIBRARIES}
	${LIBMTP_LIBRARIES}
	${LIBUSB_LIBRARIES}
	${LIBZIP_LIBRARIES}
	${HID_LIB}
	-lsqlite3 -lz -lxslt -lxml2 -lgit2
)

install(TARGETS subsurface-cli DESTINATION bin)
