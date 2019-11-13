# the undo-commands that perform undoable actions.

# models used both mobile and desktop builds
set(SUBSURFACE_GENERIC_COMMANDS_SRCS
	command_base.cpp
	command_base.h
	command.cpp
	command.h
	command_divelist.cpp
	command_divelist.h
	command_divesite.cpp
	command_divesite.h
	command_edit.cpp
	command_edit.h
	command_edit_trip.cpp
	command_edit_trip.h
	command_private.cpp
	command_private.h
)
if (SUBSURFACE_TARGET_EXECUTABLE MATCHES "DesktopExecutable")
	add_library(subsurface_commands_desktop STATIC ${SUBSURFACE_GENERIC_COMMANDS_SRCS})
	target_link_libraries(subsurface_commands_desktop ${QT_LIBRARIES})
#elseif (SUBSURFACE_TARGET_EXECUTABLE MATCHES "MobileExecutable")
#	add_library(subsurface_commands_mobile STATIC ${SUBSURFACE_GENERIC_COMMANDS_SRCS})
#	target_link_libraries(subsurface_commands_mobile ${QT_LIBRARIES})
endif()
