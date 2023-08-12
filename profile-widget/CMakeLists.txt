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
	divepixmapcache.cpp
	divepixmapcache.h
	divepercentageitem.cpp
	divepercentageitem.h
	diveprofileitem.cpp
	diveprofileitem.h
	diverectitem.cpp
	diverectitem.h
	divetextitem.cpp
	divetextitem.h
	pictureitem.h
	pictureitem.cpp
	profilescene.cpp
	profilescene.h
	profiletranslations.h
	profileview.cpp
	profileview.h
	tankitem.cpp
	tankitem.h
	tooltipitem.h
	tooltipitem.cpp
)
if (SUBSURFACE_TARGET_EXECUTABLE MATCHES "MobileExecutable")
set(SUBSURFACE_PROFILE_LIB_SRCS
	${SUBSURFACE_PROFILE_LIB_SRCS}
)
else ()
set(SUBSURFACE_PROFILE_LIB_SRCS
	${SUBSURFACE_PROFILE_LIB_SRCS}
	divehandler.cpp
	divehandler.h
	ruleritem.cpp
	ruleritem.h
)
endif ()
source_group("Subsurface Profile" FILES ${SUBSURFACE_PROFILE_LIB_SRCS})

add_library(subsurface_profile STATIC ${SUBSURFACE_PROFILE_LIB_SRCS})
target_link_libraries(subsurface_profile ${QT_LIBRARIES})
