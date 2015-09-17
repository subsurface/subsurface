# the profile widget
include_directories(.
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_BINARY_DIR}
)

file(GLOB SUBSURFACE_PREFERENCES_UI *.ui)
qt5_wrap_ui(SUBSURFACE_PREFERENCES_UI_HDRS ${SUBSURFACE_PREFERENCES_UI})

source_group("Subsurface Interface Files" FILES ${SUBSURFACE_PREFERENCES_UI})

set(SUBSURFACE_PREFERENCES_LIB_SRCS
	abstractpreferenceswidget.cpp
	preferencesdialog.cpp
	preferences_language.cpp
)

source_group("Subsurface Preferences" FILES ${SUBSURFACE_PREFERENCES_LIB_SRCS})

add_library(subsurface_desktop_preferences STATIC ${SUBSURFACE_PREFERENCES_LIB_SRCS} ${SUBSURFACE_PREFERENCES_UI_HDRS})
target_link_libraries(subsurface_desktop_preferences ${QT_LIBRARIES})

