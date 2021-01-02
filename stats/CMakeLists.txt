# the stats-graph widget
include_directories(.
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_BINARY_DIR}
)

set(SUBSURFACE_STATS_SRCS
	barseries.h
	barseries.cpp
	boxseries.h
	boxseries.cpp
	chartlistmodel.h
	chartlistmodel.cpp
	informationbox.h
	informationbox.cpp
	legend.h
	legend.cpp
	pieseries.h
	pieseries.cpp
	scatterseries.h
	scatterseries.cpp
	statsaxis.h
	statsaxis.cpp
	statscolors.h
	statscolors.cpp
	statsseries.h
	statsseries.cpp
	statsstate.h
	statsstate.cpp
	statstranslations.h
	statsvariables.h
	statsvariables.cpp
	statsview.h
	statsview.cpp
	zvalues.h
)

source_group("Subsurface statistics sourcecode" FILES ${SUBSURFACE_STATS_SRCS})

add_library(subsurface_stats STATIC ${SUBSURFACE_STATS_SRCS})
target_link_libraries(subsurface_stats ${QT_LIBRARIES})
