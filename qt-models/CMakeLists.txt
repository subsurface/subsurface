# the data models that will interface
# with the views.

# models used both mobile and desktop builds
set(SUBSURFACE_GENERIC_MODELS_LIB_SRCS
	diveplotdatamodel.cpp
	diveimportedmodel.cpp
)

# models exclusively used in desktop builds
set(SUBSURFACE_DESKTOP_MODELS_LIB_SRCS
	divepicturemodel.cpp
	cleanertablemodel.cpp
	models.cpp
	tankinfomodel.cpp
	treemodel.cpp
	maplocationmodel.cpp
	cylindermodel.cpp
	yearlystatisticsmodel.cpp
	weigthsysteminfomodel.cpp
	weightmodel.cpp
	filtermodels.cpp
	divecomputermodel.cpp
	divetripmodel.cpp
	diveplannermodel.cpp
	divecomputerextradatamodel.cpp
	completionmodels.cpp
	divelocationmodel.cpp
	ssrfsortfilterproxymodel.cpp
)

# models exclusively used in mobile builds
set(SUBSURFACE_MOBILE_MODELS_LIB_SRCS
	divelistmodel.cpp
	messagehandlermodel.cpp
	gpslistmodel.cpp
)

if(${SUBSURFACE_TARGET_EXECUTABLE} MATCHES "DesktopExecutable")
	add_library(subsurface_models_desktop STATIC ${SUBSURFACE_GENERIC_MODELS_LIB_SRCS}
		${SUBSURFACE_DESKTOP_MODELS_LIB_SRCS})
	target_link_libraries(subsurface_models_desktop ${QT_LIBRARIES})
elseif(${SUBSURFACE_TARGET_EXECUTABLE} MATCHES "MobileExecutable")
	add_library(subsurface_models_mobile STATIC ${SUBSURFACE_GENERIC_MODELS_LIB_SRCS}
		${SUBSURFACE_MOBILE_MODELS_LIB_SRCS})
	target_link_libraries(subsurface_models_mobile ${QT_LIBRARIES})
endif()
