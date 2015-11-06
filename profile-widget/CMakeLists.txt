# the profile widget
set(SUBSURFACE_PROFILE_LIB_SRCS
	profilewidget2.cpp
	diverectitem.cpp
	divepixmapitem.cpp
	divelineitem.cpp
	divetextitem.cpp
	animationfunctions.cpp
	divecartesianaxis.cpp
	diveprofileitem.cpp
	diveeventitem.cpp
	divetooltipitem.cpp
	ruleritem.cpp
	tankitem.cpp
)
source_group("Subsurface Profile" FILES ${SUBSURFACE_PROFILE_LIB_SRCS})
if (SUBSURFACE_MOBILE)
	add_definitions(-DSUBSURFACE_MOBILE=1)
endif()

add_library(subsurface_profile STATIC ${SUBSURFACE_PROFILE_LIB_SRCS})
target_link_libraries(subsurface_profile ${QT_LIBRARIES})
