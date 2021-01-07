# mobile backend functions

set(SUBSURFACE_MOBILE_SRCS
	qmlinterface.cpp
	qmlmanager.cpp
	statsmanager.cpp
	themeinterface.cpp
)

if (SUBSURFACE_TARGET_EXECUTABLE MATCHES "MobileExecutable")
	add_library(subsurface_mobile STATIC ${SUBSURFACE_MOBILE_SRCS})
	target_link_libraries(subsurface_mobile ${QT_LIBRARIES})
endif()
