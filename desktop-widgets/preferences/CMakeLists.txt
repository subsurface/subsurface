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
  preferences_georeference.ui
  preferences_language.ui
)

qt5_wrap_ui(SUBSURFACE_PREFERENCES_UI_HDRS ${SUBSURFACE_PREFERENCES_UI})

source_group("Subsurface Interface Files" FILES ${SUBSURFACE_PREFERENCES_UI})

set(SUBSURFACE_PREFERENCES_LIB_SRCS
	abstractpreferenceswidget.cpp
	abstractpreferenceswidget.h
	preferences_defaults.cpp
	preferences_defaults.h
	preferences_georeference.cpp
	preferences_georeference.h
	preferences_graph.cpp
	preferences_graph.h
	preferences_language.cpp
	preferences_language.h
	preferences_network.cpp
	preferences_network.h
	preferences_units.cpp
	preferences_units.h
	preferencesdialog.cpp
	preferencesdialog.h
)

source_group("Subsurface Preferences" FILES ${SUBSURFACE_PREFERENCES_LIB_SRCS})

add_library(subsurface_desktop_preferences STATIC ${SUBSURFACE_PREFERENCES_LIB_SRCS} ${SUBSURFACE_PREFERENCES_UI_HDRS})
target_link_libraries(subsurface_desktop_preferences ${QT_LIBRARIES})

