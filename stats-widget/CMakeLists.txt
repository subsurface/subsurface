# the stats-graph widget
include_directories(.
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_BINARY_DIR}
)

set(SUBSURFACE_STATSGRAPHS_UI
	tab_divestatistics.ui
)

qt5_wrap_ui(SUBSURFACE_STATSGRAPHS_UI_HDRS ${SUBSURFACE_STATSGRAPHS_UI})

source_group("Subsurface stats-graphs ui Files" FILES ${SUBSURFACE_STATSGRAPHS_UI})

set(SUBSURFACE_STATSGRAPHS_LIB_SRCS
	tab_divestatistics.h
	tab_divestatistics.cpp
)

source_group("Subsurface stats-graphs sourcecode" FILES ${SUBSURFACE_STATSGRAPHS_LIB_SRCS})

add_library(subsurface_stats_graphs STATIC ${SUBSURFACE_STATSGRAPHS_LIB_SRCS} ${SUBSURFACE_STATSGRAPHS_UI_HDRS})
target_link_libraries(subsurface_stats_graphs ${QT_LIBRARIES})

