# the profile widget
set(SUBSURFACE_PROFILE_LIB_SRCS
	animationfunctions.cpp
	animationfunctions.h
	divecartesianaxis.cpp
	divecartesianaxis.h
	diveeventitem.cpp
	diveeventitem.h
	divelineitem.cpp
	divelineitem.h
	divepixmapitem.cpp
	divepixmapitem.h
	diveprofileitem.cpp
	diveprofileitem.h
	diverectitem.cpp
	diverectitem.h
	divetextitem.cpp
	divetextitem.h
	divetooltipitem.cpp
	divetooltipitem.h
	profilewidget2.cpp
	profilewidget2.h
# 	qmlprofile.h
	ruleritem.cpp
	ruleritem.h
	tankitem.cpp
	tankitem.h
)
source_group("Subsurface Profile" FILES ${SUBSURFACE_PROFILE_LIB_SRCS})

add_library(subsurface_profile STATIC ${SUBSURFACE_PROFILE_LIB_SRCS})
target_link_libraries(subsurface_profile ${QT_LIBRARIES})
