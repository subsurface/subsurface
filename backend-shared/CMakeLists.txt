# backend functionality shared between Desktop (UI) and Mobile (QML)

set(BACKEND_SRCS
	exportfuncs.cpp
	exportfuncs.h
	plannershared.cpp
	plannershared.h
)

add_library(subsurface_backend_shared STATIC ${BACKEND_SRCS})
target_link_libraries(subsurface_backend_shared ${QT_LIBRARIES})
