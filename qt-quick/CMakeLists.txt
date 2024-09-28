# code for qt-quick based charts
include_directories(.
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_BINARY_DIR}
)

set(SUBSURFACE_QTQUICK_SRCS
	chartitem.cpp
	chartitem.h
	chartitemhelper.h
	chartitem_ptr.h
	chartview.cpp
	chartview.h
)

source_group("Subsurface qtquick sourcecode" FILES ${SUBSURFACE_QTQUICK_SRCS})

add_library(subsurface_qtquick STATIC ${SUBSURFACE_QTQUICK_SRCS})
target_link_libraries(subsurface_qtquick ${QT_LIBRARIES})
