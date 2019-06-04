set(PLATFORM_SRC unknown_platform.c)
message(STATUS "system name ${CMAKE_SYSTEM_NAME}")
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	if(ANDROID)
		set(PLATFORM_SRC android.cpp)
	else()
		set(PLATFORM_SRC unix.c)
	endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
	set(PLATFORM_SRC android.cpp)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	set(PLATFORM_SRC macos.c)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set(PLATFORM_SRC windows.c)
elseif(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
	set(PLATFORM_SRC unix.c)
endif()

if(FTDISUPPORT)
	set(SERIAL_FTDI serial_ftdi.c)
endif()

if(BTSUPPORT)
	add_definitions(-DBT_SUPPORT)
	set(BT_SRC_FILES desktop-widgets/btdeviceselectiondialog.cpp)
	set(BT_CORE_SRC_FILES
		btdiscovery.cpp
		btdiscovery.h
		qtserialbluetooth.cpp
	)
endif()

if(BLESUPPORT)
	add_definitions(-DBLE_SUPPORT)
	list(APPEND BT_CORE_SRC_FILES qt-ble.cpp qt-ble.h)
endif()

# compile the core library part in C, part in C++
set(SUBSURFACE_CORE_LIB_SRCS
	applicationstate.cpp
	applicationstate.h
	checkcloudconnection.cpp
	checkcloudconnection.h
	cloudstorage.cpp
	cloudstorage.h
	cochran.c
	cochran.h
	color.cpp
	color.h
	configuredivecomputer.cpp
	configuredivecomputer.h
	configuredivecomputerthreads.cpp
	configuredivecomputerthreads.h
	connectionlistmodel.cpp
	connectionlistmodel.h
	datatrak.c
	datatrak.h
	deco.c
	deco.h
	device.c
	device.h
	devicedetails.cpp
	devicedetails.h
	display.h
	dive.c
	dive.h
	divecomputer.cpp
	divecomputer.h
	divelist.c
	divelist.h
	divelogexportlogic.cpp
	divelogexportlogic.h
	divesite-helper.cpp
	divesite.c
	divesite.h
	divesitehelpers.cpp
	divesitehelpers.h
	downloadfromdcthread.cpp
	downloadfromdcthread.h
	equipment.c
	equipment.h
	errorhelper.c
	exif.cpp
	exif.h
	file.c
	file.h
	format.cpp
	format.h
	gas.c
	gas.h
	gas-model.c
	gaspressures.c
	gaspressures.h
	gettext.h
	gettextfromc.cpp
	gettextfromc.h
	git-access.c
	git-access.h
	gpslocation.cpp
	gpslocation.h
	imagedownloader.cpp
	imagedownloader.h
	import-cobalt.c
	import-csv.c
	import-csv.h
	import-divinglog.c
	import-shearwater.c
	import-suunto.c
	libdivecomputer.c
	libdivecomputer.h
	liquivision.c
	load-git.c
	membuffer.c
	membuffer.h
	metadata.cpp
	metadata.h
	metrics.cpp
	metrics.h
	ostctools.c
	parse-xml.c
	parse.c
	parse.h
	planner.c
	planner.h
	plannernotes.c
	pref.h
	profile.c
	profile.h
	qt-gui.h
	qt-init.cpp
	qthelper.cpp
	qthelper.h
	save-git.c
	save-html.c
	save-html.h
	save-profiledata.c
	save-xml.c
	sha1.c
	sha1.h
	ssrf.h
	statistics.c
	statistics.h
	strndup.h
	strtod.c
	subsurface-string.h
	subsurfacestartup.c
	subsurfacestartup.h
	subsurfacesysinfo.cpp
	subsurfacesysinfo.h
	tag.c
	tag.h
	taxonomy.c
	taxonomy.h
	time.c
	trip.c
	trip.h
	uemis-downloader.c
	uemis.c
	uemis.h
	units.h
	units.c
	version.c
	version.h
	videoframeextractor.cpp
	videoframeextractor.h
	webservice.h
	windowtitleupdate.cpp
	windowtitleupdate.h
	worldmap-options.h
	worldmap-save.c
	worldmap-save.h
	xmp_parser.cpp
	xmp_parser.h

	# classes to manage struct preferences for QWidget and QML
	settings/qPref.cpp
	settings/qPref.h
	settings/qPrefCloudStorage.cpp
	settings/qPrefCloudStorage.h
	settings/qPrefDisplay.cpp
	settings/qPrefDisplay.h
	settings/qPrefDiveComputer.cpp
	settings/qPrefDiveComputer.h
	settings/qPrefDivePlanner.cpp
	settings/qPrefDivePlanner.h
	settings/qPrefGeneral.cpp
	settings/qPrefGeneral.h
	settings/qPrefGeocoding.cpp
	settings/qPrefGeocoding.h
	settings/qPrefLanguage.cpp
	settings/qPrefLanguage.h
	settings/qPrefLocationService.cpp
	settings/qPrefLocationService.h
	settings/qPrefPartialPressureGas.cpp
	settings/qPrefPartialPressureGas.h
	settings/qPrefPrivate.cpp
	settings/qPrefPrivate.h
	settings/qPrefProxy.cpp
	settings/qPrefProxy.h
	settings/qPrefTechnicalDetails.cpp
	settings/qPrefTechnicalDetails.h
	settings/qPrefUnit.cpp
	settings/qPrefUnit.h
	settings/qPrefUpdateManager.cpp
	settings/qPrefUpdateManager.h

	#Subsurface Qt have the Subsurface structs QObjectified for easy access via QML.
	subsurface-qt/CylinderObjectHelper.cpp
	subsurface-qt/CylinderObjectHelper.h
	subsurface-qt/DiveListNotifier.cpp
	subsurface-qt/DiveListNotifier.h
	subsurface-qt/DiveObjectHelper.cpp
	subsurface-qt/DiveObjectHelper.h

	${SERIAL_FTDI}
	${PLATFORM_SRC}
	${BT_CORE_SRC_FILES}
)
source_group("Subsurface Core" FILES ${SUBSURFACE_CORE_LIB_SRCS})

add_library(subsurface_corelib STATIC ${SUBSURFACE_CORE_LIB_SRCS} )
target_link_libraries(subsurface_corelib ${QT_LIBRARIES})
