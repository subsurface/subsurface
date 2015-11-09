# create the libraries
file(GLOB SUBSURFACE_UI *.ui)
qt5_wrap_ui(SUBSURFACE_UI_HDRS ${SUBSURFACE_UI})
source_group("Subsurface Interface Files" FILES ${SUBSURFACE_UI})

if(BTSUPPORT)
	set(BT_SRC_FILES btdeviceselectiondialog.cpp)
endif()

include_directories(.
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_BINARY_DIR}
)

add_subdirectory(preferences)
if(NOT DISABLE_PLUGINS)
	add_subdirectory(plugins)
endif()

set (SUBSURFACE_UI
	about.ui
	btdeviceselectiondialog.ui
	configuredivecomputerdialog.ui
	divecomponentselection.ui
	divecomputermanagementdialog.ui
	divelogexportdialog.ui
	divelogimportdialog.ui
	diveplanner.ui
	diveshareexportdialog.ui
	downloadfromdivecomputer.ui
	filterwidget.ui
	listfilter.ui
	locationInformation.ui
	maintab.ui
	mainwindow.ui
	plannerDetails.ui
	plannerSettings.ui
	printoptions.ui
	renumber.ui
	searchbar.ui
	setpoint.ui
	shiftimagetimes.ui
	shifttimes.ui
	tableview.ui
	templateedit.ui
	urldialog.ui
	usersurvey.ui
	webservices.ui
)

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
	kmessagewidget.cpp
	maintab.cpp
	mainwindow.cpp
	modeldelegates.cpp
	notificationwidget.cpp
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

if (BTSUPPORT)
	set(SUBSURFACE_INTERFACE ${SUBSURFACE_INTERFACE}
		btdeviceselectiondialog.cpp
	)
endif()

source_group("Subsurface Interface" FILES ${SUBSURFACE_INTERFACE})

# the yearly statistics widget.
set(SUBSURFACE_STATISTICS_LIB_SRCS
	statistics/statisticswidget.cpp
	statistics/yearstatistics.cpp
	statistics/statisticsbar.cpp
	statistics/monthstatistics.cpp
)
source_group("Subsurface Statistics" FILES ${SUBSURFACE_STATISTICS_LIB_SRCS})

qt5_wrap_ui(SUBSURFACE_UI_SRCS ${SUBSURFACE_UI})

add_library(subsurface_statistics STATIC ${SUBSURFACE_STATISTICS_LIB_SRCS})
target_link_libraries(subsurface_statistics ${QT_LIBRARIES})
add_library(subsurface_generated_ui STATIC ${SUBSURFACE_UI_HDRS})
target_link_libraries(subsurface_generated_ui ${QT_LIBRARIES})
add_library(subsurface_interface STATIC ${SUBSURFACE_INTERFACE} ${SUBSURFACE_UI_SRCS})
target_link_libraries(subsurface_interface ${QT_LIBRARIES} ${MARBLE_LIBRARIES} ${GRANTLEE_LIBRARIES} subsurface_desktop_preferences)
