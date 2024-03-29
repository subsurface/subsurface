set(PLATFORM_SRC unknown_platform.c)
message(STATUS "system name ${CMAKE_SYSTEM_NAME}")
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	if(ANDROID)
		set(PLATFORM_SRC android.cpp serial_usb_android.cpp)
	else()
		set(PLATFORM_SRC unix.cpp)
	endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
	set(PLATFORM_SRC android.cpp serial_usb_android.cpp)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	set(PLATFORM_SRC macos.cpp)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set(PLATFORM_SRC windows.cpp)
elseif(CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
	set(PLATFORM_SRC unix.cpp)
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
	checkcloudconnection.cpp
	checkcloudconnection.h
	cloudstorage.cpp
	cloudstorage.h
	cochran.cpp
	cochran.h
	color.cpp
	color.h
	configuredivecomputer.cpp
	configuredivecomputer.h
	configuredivecomputerthreads.cpp
	configuredivecomputerthreads.h
	connectionlistmodel.cpp
	connectionlistmodel.h
	datatrak.cpp
	datatrak.h
	deco.cpp
	deco.h
	device.cpp
	device.h
	devicedetails.cpp
	devicedetails.h
	dive.cpp
	dive.h
	divecomputer.c
	divecomputer.h
	dive.h
	divefilter.cpp
	divefilter.h
	divelist.c
	divelist.h
	divelog.cpp
	divelog.h
	divelogexportlogic.cpp
	divelogexportlogic.h
	divesite-helper.cpp
	divesite.c
	divesite.h
	divesitehelpers.cpp
	divesitehelpers.h
	downloadfromdcthread.cpp
	downloadfromdcthread.h
	event.c
	event.h
	eventtype.cpp
	eventtype.h
	equipment.c
	equipment.h
	errorhelper.cpp
	exif.cpp
	exif.h
	extradata.h
	file.cpp
	file.h
	filterconstraint.cpp
	filterconstraint.h
	filterpreset.cpp
	filterpreset.h
	format.cpp
	format.h
	fulltext.cpp
	fulltext.h
	gas.c
	gas.h
	gas-model.c
	gaspressures.c
	gaspressures.h
	gettext.h
	gettextfromc.cpp
	gettextfromc.h
	git-access.cpp
	git-access.h
	globals.cpp
	globals.h
	imagedownloader.cpp
	imagedownloader.h
	import-cobalt.cpp
	import-csv.cpp
	import-csv.h
	import-divinglog.cpp
	import-shearwater.cpp
	import-suunto.cpp
	import-seac.cpp
	interpolate.h
	libdivecomputer.cpp
	libdivecomputer.h
	liquivision.cpp
	load-git.cpp
	membuffer.cpp
	membuffer.h
	metadata.cpp
	metadata.h
	metrics.cpp
	metrics.h
	ostctools.c
	owning_ptrs.h
	parse-gpx.cpp
	parse-xml.cpp
	parse.cpp
	parse.h
	picture.c
	picture.h
	pictureobj.cpp
	pictureobj.h
	planner.cpp
	planner.h
	plannernotes.cpp
	pref.h
	pref.c
	profile.cpp
	profile.h
	qt-gui.h
	qt-init.cpp
	qthelper.cpp
	qthelper.h
	range.h
	sample.cpp
	sample.h
	save-git.cpp
	save-html.cpp
	save-html.h
	save-profiledata.c
	save-xml.cpp
	selection.cpp
	selection.h
	sha1.c
	sha1.h
	ssrf.h
	statistics.c
	statistics.h
	string-format.h
	string-format.cpp
	strtod.c
	subsurface-float.h
	subsurface-string.cpp
	subsurface-string.h
	subsurfacestartup.cpp
	subsurfacestartup.h
	subsurfacesysinfo.cpp
	subsurfacesysinfo.h
	tag.cpp
	tag.h
	taxonomy.c
	taxonomy.h
	time.cpp
	timer.c
	timer.h
	trip.c
	trip.h
	uemis-downloader.cpp
	uemis.c
	uemis.h
	units.h
	units.c
	uploadDiveShare.cpp
	uploadDiveShare.h
	uploadDiveLogsDE.cpp
	uploadDiveLogsDE.h
	version.c
	version.h
	videoframeextractor.cpp
	videoframeextractor.h
	webservice.h
	windowtitleupdate.cpp
	windowtitleupdate.h
	worldmap-options.h
	worldmap-save.cpp
	worldmap-save.h
	xmlparams.cpp
	xmlparams.h
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
	settings/qPrefEquipment.cpp
	settings/qPrefEquipment.h
	settings/qPrefGeneral.cpp
	settings/qPrefGeneral.h
	settings/qPrefGeocoding.cpp
	settings/qPrefGeocoding.h
	settings/qPrefLanguage.cpp
	settings/qPrefLanguage.h
	settings/qPrefLog.cpp
	settings/qPrefLog.h
	settings/qPrefMedia.cpp
	settings/qPrefMedia.h
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
	subsurface-qt/divelistnotifier.cpp
	subsurface-qt/divelistnotifier.h

	${SERIAL_FTDI}
	${PLATFORM_SRC}
	${BT_CORE_SRC_FILES}
)
source_group("Subsurface Core" FILES ${SUBSURFACE_CORE_LIB_SRCS})

add_library(subsurface_corelib STATIC ${SUBSURFACE_CORE_LIB_SRCS} )
target_link_libraries(subsurface_corelib ${QT_LIBRARIES})
