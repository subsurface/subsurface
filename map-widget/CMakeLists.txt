# map widget to show locations

# Copy the appropriate MapWidget.qml based on Qt version
set(MAPWIDGET_DEST ${CMAKE_CURRENT_SOURCE_DIR}/qml/MapWidget.qml)
if(USINGQT6)
	set(MAPWIDGET_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/qml/MapWidgetQt6.qml)
	message(STATUS "Using MapWidgetQt6.qml for Qt ${QT_VERSION}")
else()
	set(MAPWIDGET_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/qml/MapWidgetQt5.qml)
	message(STATUS "Using MapWidgetQt5.qml for Qt ${QT_VERSION}")
endif()

# Copy the selected source file to MapWidget.qml
execute_process(
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		${MAPWIDGET_SOURCE}
		${MAPWIDGET_DEST}
)

# library used by mobile build
set(SUBSURFACE_MAPWIDGET_SRCS
	qmlmapwidgethelper.cpp
)
add_library(subsurface_mapwidget STATIC ${SUBSURFACE_MAPWIDGET_SRCS})
target_link_libraries(subsurface_mapwidget ${QT_LIBRARIES})
