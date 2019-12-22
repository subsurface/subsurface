# map widget to show locations

# library used by mobile build
set(SUBSURFACE_MAPWIDGET_SRCS
	qmlmapwidgethelper.cpp
)
add_library(subsurface_mapwidget STATIC ${SUBSURFACE_MAPWIDGET_SRCS})
target_link_libraries(subsurface_mapwidget ${QT_LIBRARIES})
