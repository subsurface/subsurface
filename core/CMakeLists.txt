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
	checkcloudconnection.cpp
	cloudstorage.cpp
	cochran.c
	color.cpp
	configuredivecomputer.cpp
	configuredivecomputerthreads.cpp
	connectionlistmodel.cpp
	datatrak.c
	deco.c
	device.c
	devicedetails.cpp
	dive.c
	divecomputer.cpp
	divelogexportlogic.cpp
	divesite.c
	divesitehelpers.cpp
	divesite-helper.cpp
	divelist.c
	downloadfromdcthread.cpp
	equipment.c
	errorhelper.c
	exif.cpp
	file.c
	format.cpp
	gaspressures.c
	gas-model.c
	gettextfromc.cpp
	git-access.c
	gpslocation.cpp
	imagedownloader.cpp
	isocialnetworkintegration.cpp
	libdivecomputer.c
	liquivision.c
	load-git.c
	membuffer.c
	metadata.cpp
	metrics.cpp
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
	pluginmanager.cpp
	profile.c
	qthelper.cpp
	qt-init.cpp
	save-git.c
	save-xml.c
	save-html.c
	sha1.c
	statistics.c
	strtod.c
	subsurfacestartup.c
	subsurfacesysinfo.cpp
	taxonomy.c
	time.c
	uemis.c
	uemis-downloader.c
	version.c
	windowtitleupdate.cpp
	worldmap-save.c

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

