# create the libraries
file(GLOB SUBSURFACE_UI *.ui)
if(NOT USINGQT6)
	qt5_wrap_ui(SUBSURFACE_UI_HDRS ${SUBSURFACE_UI})
else()
	qt_wrap_ui(SUBSURFACE_UI_HDRS ${SUBSURFACE_UI})
endif()
source_group("Subsurface Interface Files" FILES ${SUBSURFACE_UI})

if(BTSUPPORT)
	set(BT_SRC_FILES btdeviceselectiondialog.cpp)
endif()

include_directories(.
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_BINARY_DIR}
)

set(CMAKE_AUTOMOC ON)

add_subdirectory(preferences)

set (SUBSURFACE_UI
	about.ui
	addfilterpreset.ui
	btdeviceselectiondialog.ui
	configuredivecomputerdialog.ui
	divecomponentselection.ui
	divelogexportdialog.ui
	divelogimportdialog.ui
	divesiteimportdialog.ui
	divesitelistview.ui
	diveplanner.ui
	diveshareexportdialog.ui
	downloadfromdivecomputer.ui
	filterwidget.ui
	findmovedimagesdialog.ui
	importgps.ui
	listfilter.ui
	locationinformation.ui
	mainwindow.ui
	plannerDetails.ui
	plannerSettings.ui
	printoptions.ui
	profilewidget.ui
	renumber.ui
	searchbar.ui
	setpoint.ui
	shiftimagetimes.ui
	shifttimes.ui
	statswidget.ui
	tableview.ui
	templateedit.ui
	tripselectiodialog.ui
	urldialog.ui
	webservices.ui
	tab-widgets/TabDiveNotes.ui
	tab-widgets/TabDiveStatistics.ui
	tab-widgets/TabDiveInformation.ui
	tab-widgets/TabDivePhotos.ui
	tab-widgets/TabDiveExtraInfo.ui
	tab-widgets/TabDiveEquipment.ui
)

# the interface, in C++
set(SUBSURFACE_INTERFACE
	about.cpp
	about.h
	configuredivecomputerdialog.cpp
	configuredivecomputerdialog.h
	divelistview.cpp
	divelistview.h
	divelogexportdialog.cpp
	divelogexportdialog.h
	divelogimportdialog.cpp
	divelogimportdialog.h
	divepicturewidget.cpp
	divepicturewidget.h
	diveplanner.cpp
	diveplanner.h
	diveshareexportdialog.cpp
	diveshareexportdialog.h
	divesiteimportdialog.cpp
	divesiteimportdialog.h
	divesitelistview.cpp
	divesitelistview.h
	downloadfromdivecomputer.cpp
	downloadfromdivecomputer.h
	filterconstraintwidget.cpp
	filterconstraintwidget.h
	filterwidget.cpp
	filterwidget.h
	findmovedimagesdialog.cpp
	findmovedimagesdialog.h
	groupedlineedit.cpp
	groupedlineedit.h
	importgps.cpp
	importgps.h
	kmessagewidget.cpp
	kmessagewidget.h
	locationinformation.cpp
	locationinformation.h
	mainwindow.cpp
	mainwindow.h
	modeldelegates.cpp
	modeldelegates.h
	notificationwidget.cpp
	notificationwidget.h
	profilewidget.cpp
	profilewidget.h
	qtwaitingspinner.cpp
	qtwaitingspinner.h
	simplewidgets.cpp
	simplewidgets.h
	starwidget.cpp
	statswidget.h
	statswidget.cpp
	starwidget.h
	subsurfacewebservices.cpp
	subsurfacewebservices.h
	tab-widgets/TabBase.cpp
	tab-widgets/TabBase.h
	tab-widgets/TabDiveNotes.cpp
	tab-widgets/TabDiveNotes.h
	tab-widgets/TabDiveExtraInfo.cpp
	tab-widgets/TabDiveExtraInfo.h
	tab-widgets/TabDiveEquipment.cpp
	tab-widgets/TabDiveEquipment.h
	tab-widgets/TabDiveInformation.cpp
	tab-widgets/TabDiveInformation.h
	tab-widgets/TabDivePhotos.cpp
	tab-widgets/TabDivePhotos.h
	tab-widgets/TabDiveStatistics.cpp
	tab-widgets/TabDiveStatistics.h
	tab-widgets/maintab.cpp
	tab-widgets/maintab.h
	tableview.cpp
	tableview.h
	tagwidget.cpp
	tagwidget.h
	textedit.cpp
	textedit.h
	tripselectiondialog.cpp
	tripselectiondialog.h
	updatemanager.cpp
	updatemanager.h
)
if(MAPSUPPORT)
	LIST(APPEND SUBSURFACE_INTERFACE
		mapwidget.cpp
		mapwidget.h
	)
endif()

if(NOT NO_USERMANUAL)
	set(SUBSURFACE_INTERFACE ${SUBSURFACE_INTERFACE}
		usermanual.cpp
		usermanual.h
	)
endif()

if(NOT NO_PRINTING)
	set(SUBSURFACE_INTERFACE ${SUBSURFACE_INTERFACE}
		printdialog.cpp
		printdialog.h
		printer.cpp
		printer.h
		printoptions.cpp
		printoptions.h
		templateedit.cpp
		templateedit.h
		templatelayout.cpp
		templatelayout.h
	)
endif()

if (BTSUPPORT)
	set(SUBSURFACE_INTERFACE ${SUBSURFACE_INTERFACE}
		btdeviceselectiondialog.cpp
		btdeviceselectiondialog.h
	)
endif()

source_group("Subsurface Interface" FILES ${SUBSURFACE_INTERFACE})

if(NOT USINGQT6)
	qt5_wrap_ui(SUBSURFACE_UI_SRCS ${SUBSURFACE_UI})
else()
	qt_wrap_ui(SUBSURFACE_UI_SRCS ${SUBSURFACE_UI})
endif()

add_library(subsurface_generated_ui STATIC ${SUBSURFACE_UI_HDRS})
target_link_libraries(subsurface_generated_ui ${QT_LIBRARIES})
add_library(subsurface_interface STATIC ${SUBSURFACE_INTERFACE} ${SUBSURFACE_UI_SRCS})
target_link_libraries(subsurface_interface ${QT_LIBRARIES} subsurface_desktop_preferences)
