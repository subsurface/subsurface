# the profile widget
include_directories(.
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_BINARY_DIR}
)

set(SUBSURFACE_PREFERENCES_UI
  preferences_defaults.ui
  preferences_graph.ui
  preferences_network.ui
  preferences_units.ui
  prefs_georeference.ui
  prefs_language.ui
)

qt5_wrap_ui(SUBSURFACE_PREFERENCES_UI_HDRS ${SUBSURFACE_PREFERENCES_UI})

source_group("Subsurface Interface Files" FILES ${SUBSURFACE_PREFERENCES_UI})

set(SUBSURFACE_PREFERENCES_LIB_SRCS
	abstractpreferenceswidget.cpp
	preferencesdialog.cpp
	preferences_language.cpp
	preferences_georeference.cpp
	preferences_defaults.cpp
	preferences_units.cpp
	preferences_graph.cpp
	preferences_network.cpp
)

source_group("Subsurface Preferences" FILES ${SUBSURFACE_PREFERENCES_LIB_SRCS})

add_library(subsurface_desktop_preferences STATIC ${SUBSURFACE_PREFERENCES_LIB_SRCS} ${SUBSURFACE_PREFERENCES_UI_HDRS})
target_link_libraries(subsurface_desktop_preferences ${QT_LIBRARIES})

