# the profile widget
set(SUBSURFACE_PREFERENCES_LIB_SRCS
	abstractpreferenceswidget.cpp
)

source_group("Subsurface Preferences" FILES ${SUBSURFACE_PREFERENCES_LIB_SRCS})

add_library(subsurface_desktop_preferences STATIC ${SUBSURFACE_PREFERENCES_LIB_SRCS})
target_link_libraries(subsurface_desktop_preferences ${QT_LIBRARIES})