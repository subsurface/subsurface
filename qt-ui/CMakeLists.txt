# create the libraries
file(GLOB SUBSURFACE_UI *.ui)
qt5_wrap_ui(SUBSURFACE_UI_HDRS ${SUBSURFACE_UI})
qt5_add_resources(SUBSURFACE_RESOURCES subsurface.qrc)
source_group("Subsurface Interface Files" FILES ${SUBSURFACE_UI})

if(BTSUPPORT)
	set(BT_SRC_FILES btdeviceselectiondialog.cpp)
endif()

# the interface, in C++
set(SUBSURFACE_INTERFACE
	updatemanager.cpp
	about.cpp
	divecomputermanagementdialog.cpp
	divelistview.cpp
	diveplanner.cpp
	diveshareexportdialog.cpp
	downloadfromdivecomputer.cpp
	globe.cpp
	graphicsview-common.cpp
	kmessagewidget.cpp
	maintab.cpp
	mainwindow.cpp
	modeldelegates.cpp
	metrics.cpp
	notificationwidget.cpp
	preferences.cpp
	simplewidgets.cpp
	starwidget.cpp
	subsurfacewebservices.cpp
	tableview.cpp
	divelogimportdialog.cpp
	tagwidget.cpp
	groupedlineedit.cpp
	divelogexportdialog.cpp
	divepicturewidget.cpp
	usersurvey.cpp
	configuredivecomputerdialog.cpp
	undocommands.cpp
	locationinformation.cpp
	qtwaitingspinner.cpp
)

if(NOT NO_USERMANUAL)
	set(SUBSURFACE_INTERFACE ${SUBSURFACE_INTERFACE}
		usermanual.cpp
	)
endif()

if(NOT NO_PRINTING)
	set(SUBSURFACE_INTERFACE ${SUBSURFACE_INTERFACE}
		templateedit.cpp
		printdialog.cpp
		printoptions.cpp
		printer.cpp
		templatelayout.cpp
	)
endif()

if (FBSUPPORT)
	set(SUBSURFACE_INTERFACE ${SUBSURFACE_INTERFACE}
		socialnetworks.cpp
	)
endif()

if (BTSUPPORT)
	set(SUBSURFACE_INTERFACE ${SUBSURFACE_INTERFACE}
		btdeviceselectiondialog.cpp
	)
endif()

source_group("Subsurface Interface" FILES ${SUBSURFACE_INTERFACE})

# the profile widget
set(SUBSURFACE_PROFILE_LIB_SRCS
	profile/profilewidget2.cpp
	profile/diverectitem.cpp
	profile/divepixmapitem.cpp
	profile/divelineitem.cpp
	profile/divetextitem.cpp
	profile/animationfunctions.cpp
	profile/divecartesianaxis.cpp
	profile/diveprofileitem.cpp
	profile/diveeventitem.cpp
	profile/divetooltipitem.cpp
	profile/ruleritem.cpp
	profile/tankitem.cpp
)
source_group("Subsurface Profile" FILES ${SUBSURFACE_PROFILE_LIB_SRCS})

# the yearly statistics widget.
set(SUBSURFACE_STATISTICS_LIB_SRCS
	statistics/statisticswidget.cpp
	statistics/yearstatistics.cpp
	statistics/statisticsbar.cpp
	statistics/monthstatistics.cpp
)
source_group("Subsurface Statistics" FILES ${SUBSURFACE_STATISTICS_LIB_SRCS})

add_library(subsurface_profile STATIC ${SUBSURFACE_PROFILE_LIB_SRCS})
target_link_libraries(subsurface_profile ${QT_LIBRARIES})
add_library(subsurface_statistics STATIC ${SUBSURFACE_STATISTICS_LIB_SRCS})
target_link_libraries(subsurface_statistics ${QT_LIBRARIES})
add_library(subsurface_generated_ui STATIC ${SUBSURFACE_UI_HDRS})
target_link_libraries(subsurface_generated_ui ${QT_LIBRARIES})
add_library(subsurface_interface STATIC ${SUBSURFACE_INTERFACE})
target_link_libraries(subsurface_interface ${QT_LIBRARIES} ${MARBLE_LIBRARIES})
