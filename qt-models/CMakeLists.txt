# the data models that will interface
# with the views.

# models used both mobile and desktop builds
set(SUBSURFACE_GENERIC_MODELS_LIB_SRCS
	cleanertablemodel.cpp
	cleanertablemodel.h
	completionmodels.cpp
	completionmodels.h
	cylindermodel.cpp
	cylindermodel.h
	diveimportedmodel.cpp
	diveimportedmodel.h
	divelocationmodel.cpp
	divelocationmodel.h
	diveplannermodel.cpp
	diveplannermodel.h
	diveplotdatamodel.cpp
	diveplotdatamodel.h
	divetripmodel.cpp
	divetripmodel.h
	filterconstraintmodel.cpp
	filterconstraintmodel.h
	filterpresetmodel.cpp
	filterpresetmodel.h
	maplocationmodel.cpp
	maplocationmodel.h
	models.cpp
	models.h
	tankinfomodel.cpp
	tankinfomodel.h
	weightsysteminfomodel.cpp
	weightsysteminfomodel.h
)

# models exclusively used in desktop builds
set(SUBSURFACE_DESKTOP_MODELS_LIB_SRCS
	divecomputerextradatamodel.cpp
	divecomputerextradatamodel.h
	divecomputermodel.cpp
	divecomputermodel.h
	divepicturemodel.cpp
	divepicturemodel.h
	divesiteimportmodel.cpp
	divesiteimportmodel.h
	filtermodels.cpp
	filtermodels.h
	treemodel.cpp
	treemodel.h
	weightmodel.cpp
	weightmodel.h
	yearlystatisticsmodel.cpp
	yearlystatisticsmodel.h
)

# models exclusively used in mobile builds
set(SUBSURFACE_MOBILE_MODELS_LIB_SRCS
	divesummarymodel.cpp
	divesummarymodel.h
	gpslistmodel.cpp
	gpslistmodel.h
	messagehandlermodel.cpp
	messagehandlermodel.h
	mobilelistmodel.cpp
	mobilelistmodel.h
)

if (SUBSURFACE_TARGET_EXECUTABLE MATCHES "DesktopExecutable")
	add_library(subsurface_models_desktop STATIC ${SUBSURFACE_GENERIC_MODELS_LIB_SRCS}
		${SUBSURFACE_DESKTOP_MODELS_LIB_SRCS})
	target_link_libraries(subsurface_models_desktop ${QT_LIBRARIES})
elseif (SUBSURFACE_TARGET_EXECUTABLE MATCHES "MobileExecutable")
	add_library(subsurface_models_mobile STATIC ${SUBSURFACE_GENERIC_MODELS_LIB_SRCS}
		${SUBSURFACE_MOBILE_MODELS_LIB_SRCS})
	target_link_libraries(subsurface_models_mobile ${QT_LIBRARIES})
elseif (SUBSURFACE_TARGET_EXECUTABLE MATCHES "DownloaderExecutable")
	add_library(subsurface_models_downloader STATIC ${SUBSURFACE_GENERIC_MODELS_LIB_SRCS})
	target_link_libraries(subsurface_models_downloader ${QT_LIBRARIES})
endif()
