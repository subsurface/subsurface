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
	set(BT_SRC_FILES qt-ui/btdeviceselectiondialog.cpp)
	set(BT_CORE_SRC_FILES qtserialbluetooth.cpp)
endif()

# compile the core library, in C.
set(SUBSURFACE_CORE_LIB_SRCS
	cochran.c
	datatrak.c
	deco.c
	device.c
	dive.c
	divesite.c
	divesite.cpp
	divelist.c
	equipment.c
	file.c
	git-access.c
	libdivecomputer.c
	liquivision.c
	load-git.c
	membuffer.c
	ostctools.c
	parse-xml.c
	planner.c
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
	# gettextfrommoc should be added because we are using it on the c-code.
	gettextfromc.cpp
	# dirk ported some core functionality to c++.
	qthelper.cpp
	divecomputer.cpp
	exif.cpp
	subsurfacesysinfo.cpp
	devicedetails.cpp
	configuredivecomputer.cpp
	configuredivecomputerthreads.cpp
	divesitehelpers.cpp
	taxonomy.c
	checkcloudconnection.cpp
	windowtitleupdate.cpp
	divelogexportlogic.cpp
	qt-init.cpp
	qtserialbluetooth.cpp
	metrics.cpp
	color.cpp
	pluginmanager.cpp
	imagedownloader.cpp
	${SERIAL_FTDI}
	${PLATFORM_SRC}
	${BT_CORE_SRC_FILES}
)
source_group("Subsurface Core" FILES ${SUBSURFACE_CORE_LIB_SRCS})

add_library(subsurface_corelib STATIC ${SUBSURFACE_CORE_LIB_SRCS} )
target_link_libraries(subsurface_corelib ${QT_LIBRARIES})

