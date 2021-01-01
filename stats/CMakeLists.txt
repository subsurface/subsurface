# the stats-graph widget
include_directories(.
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_BINARY_DIR}
)

set(SUBSURFACE_STATS_SRCS
	legend.h
	legend.cpp
	statsaxis.h
	statsaxis.cpp
	statscolors.h
	statscolors.cpp
	statsseries.h
	statsseries.cpp
	statsvariables.h
	statsvariables.cpp
	zvalues.h
)

source_group("Subsurface statistics sourcecode" FILES ${SUBSURFACE_STATS_SRCS})

add_library(subsurface_stats STATIC ${SUBSURFACE_STATS_SRCS})
target_link_libraries(subsurface_stats ${QT_LIBRARIES})
