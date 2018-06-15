set(PLATFORM_SRC unknown_platform.c)
message(STATUS "system name ${CMAKE_SYSTEM_NAME}")
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	if(ANDROID)
		set(PLATFORM_SRC android.cpp)
	else()
		set(PLATFORM_SRC linux.c)
	endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
	set(PLATFORM_SRC android.cpp)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	set(PLATFORM_SRC macos.c)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set(PLATFORM_SRC windows.c)
endif()

if(FTDISUPPORT)
	set(SERIAL_FTDI serial_ftdi.c)
endif()

if(BTSUPPORT)
	add_definitions(-DBT_SUPPORT)
	set(BT_SRC_FILES desktop-widgets/btdeviceselectiondialog.cpp)
	set(BT_CORE_SRC_FILES qtserialbluetooth.cpp btdiscovery.cpp)
endif()

if(BLESUPPORT)
	add_definitions(-DBLE_SUPPORT)
	set(BT_CORE_SRC_FILES ${BT_CORE_SRC_FILES} qt-ble.cpp)
endif()

# compile the core library part in C, part in C++
set(SUBSURFACE_CORE_LIB_SRCS
	# C part
	cochran.c
	datatrak.c
	deco.c
	device.c
	dive.c
	divesite.c
	divesite-helper.cpp
	divelist.c
	equipment.c
	errorhelper.c
	file.c
	gas-model.c
	git-access.c
	libdivecomputer.c
	liquivision.c
	load-git.c
	membuffer.c
	ostctools.c
	parse-xml.c
	parse.c
	import-suunto.c
	import-shearwater.c
	import-cobalt.c
	import-divinglog.c
	import-csv.c
	planner.c
	plannernotes.c
	profile.c
	gaspressures.c
	worldmap-save.c
	save-git.c
	save-xml.c
	save-html.c
	sha1.c
	statistics.c
	strtod.c
	subsurfacestartup.c
	time.c
	uemis.c
	uemis-downloader.c
	version.c

	# C++ part
	checkcloudconnection.cpp
	cloudstorage.cpp
	color.cpp
	configuredivecomputer.cpp
	configuredivecomputerthreads.cpp
	connectionlistmodel.cpp
	devicedetails.cpp
	divecomputer.cpp
	divelogexportlogic.cpp
	divesitehelpers.cpp
	downloadfromdcthread.cpp
	exif.cpp
	format.cpp
	gettextfromc.cpp
	gpslocation.cpp
	imagedownloader.cpp
	isocialnetworkintegration.cpp
	metadata.cpp
	metrics.cpp
	pluginmanager.cpp
	qthelper.cpp
	qt-init.cpp
	subsurfacesysinfo.cpp
	taxonomy.c
	windowtitleupdate.cpp

    # classes to manage struct preferences for QWidget and QML
	settings/qPref.cpp
	settings/qPrefAnimations.cpp
	settings/qPrefCS.cpp
	settings/qPrefDC.cpp
	settings/qPrefDisplay.cpp
	settings/qPrefFacebook.cpp
	settings/qPrefGeneral.cpp
	settings/qPrefGeocoding.cpp
	settings/qPrefLanguage.cpp
	settings/qPrefLocation.cpp

	#Subsurface Qt have the Subsurface structs QObjectified for easy access via QML.
	subsurface-qt/DiveObjectHelper.cpp
	subsurface-qt/CylinderObjectHelper.cpp
	subsurface-qt/SettingsObjectWrapper.cpp

	${SERIAL_FTDI}
	${PLATFORM_SRC}
	${BT_CORE_SRC_FILES}
)
source_group("Subsurface Core" FILES ${SUBSURFACE_CORE_LIB_SRCS})

add_library(subsurface_corelib STATIC ${SUBSURFACE_CORE_LIB_SRCS} )
target_link_libraries(subsurface_corelib ${QT_LIBRARIES})

