# cmake based build of Subsurface

# Uncomment his to see all commands cmake actually executes
# set(CMAKE_VERBOSE_MAKEFILE ON)

cmake_minimum_required(VERSION 3.16)
project(Subsurface)

# don't process generated files - this is new in 3.10
if (POLICY CMP0071)
	cmake_policy(SET CMP0071 OLD)
endif()

# support Packagename_ROOT environment variable
if (POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW) # CMake 3.12
endif ()

# global settings

set(CMAKE_MODULE_PATH
	${${PROJECT_NAME}_SOURCE_DIR}/cmake/Modules
	${${PROJECT_NAME}_SOURCE_DIR}/../install-root/lib/cmake
	${CMAKE_MODULE_PATH}
)

set(CMAKE_AUTOMOC ON)
include(MacroOutOfSourceBuild)
MACRO_ENSURE_OUT_OF_SOURCE_BUILD(
    "We don't support building in source, please create a build folder elsewhere and remember to run git clean -xdf to remove temporary files created by CMake."
)

#Option for memory debugging
option(SUBSURFACE_ASAN_BUILD "enable memory debugging of Subsurface binary" OFF)

#Option for profiling
option(SUBSURFACE_PROFILING_BUILD "enable profiling of Subsurface binary" OFF)

#Options regarding usage of pkgconfig
option(LIBGIT2_FROM_PKGCONFIG "use pkg-config to retrieve libgit2" OFF)
option(LIBDC_FROM_PKGCONFIG "use pkg-config to retrieve libdivecomputer" OFF)

#Library Handling
option(FORCE_LIBSSH "force linking with libssh to workaround libgit2 bug" ON)
option(LIBGIT2_DYNAMIC "search for libgit2.so before libgit2.a" OFF)

#Options regarding disabling parts of subsurface.
option(NO_PRINTING "disable the printing support" OFF)
option(NO_USERMANUAL "don't include a viewer for the user manual" OFF)

#Options regarding enabling parts of subsurface
option(BUILD_DOCS "build the user manual" OFF)
option(INSTALL_DOCS "include the user manual in the packaging" OFF)
option(BTSUPPORT "enable support for QtBluetooth" ON)
option(FTDISUPPORT "enable support for libftdi based serial" OFF)
option(LIBRAW_SUPPORT "enable support for LibRaw images" ON)

# Options regarding What should we build on subsurface
option(BUILD_TESTS "Build the tests" OFF)

# Option whether we should try to build against Qt6, if it is available
option(BUILD_WITH_QT6 "try to build against Qt6 (incomplete)" OFF)

SET(SUBSURFACE_TARGET_EXECUTABLE "DesktopExecutable" CACHE STRING "The type of application, DesktopExecutable, MobileExecutable, or DownloaderExecutable")
LIST(APPEND SUBSURFACE_ACCEPTED_EXECUTABLES  "DesktopExecutable" "MobileExecutable" "DownloaderExecutable")
SET_PROPERTY(CACHE SUBSURFACE_TARGET_EXECUTABLE PROPERTY STRINGS ${SUBSURFACE_ACCEPTED_EXECUTABLES})

#verify if Platform is correct and warn on wxit with example.
list (FIND SUBSURFACE_ACCEPTED_EXECUTABLES ${SUBSURFACE_TARGET_EXECUTABLE}  _index)
if (_index EQUAL -1)
  message(FATAL_ERROR "Requested platform not supported, please use one of the following:
    ${SUBSURFACE_ACCEPTED_EXECUTABLES}

    inputted value was: ${SUBSURFACE_TARGET_EXECUTABLE}

    Example: -DSUBSURFACE_TARGET_EXECUTABLE=DesktopExecutable")
endif()

# SUBSURFACE_SOURCE may be used in subdirectories (tests)
set(SUBSURFACE_SOURCE ${CMAKE_SOURCE_DIR})
add_definitions(-DSUBSURFACE_SOURCE="${SUBSURFACE_SOURCE}")

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
# MSVC requires C++20 for designated initializers; GCC/Clang accept them as an extension in C++17
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	set(CMAKE_CXX_STANDARD 20)
else()
	set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

#
# TODO: This Compilation part should go on the Target specific CMake.
#
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-inconsistent-missing-override")
	if ((CMAKE_SYSTEM_NAME MATCHES "Darwin") AND
	   ((CMAKE_SYSTEM_VERSION MATCHES "11.4.") OR
	    (CMAKE_OSX_DEPLOYMENT_TARGET MATCHES "10.7") OR
	    (CMAKE_OSX_DEPLOYMENT_TARGET MATCHES "10.8")))
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
	endif()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-inconsistent-missing-override")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	# Warn about possible float conversion errors
	# Use NOT VERSION_LESS since VERSION_GREATER_EQUAL is not available
	# in currently used cmake version.
	if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.9.0")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wfloat-conversion")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wfloat-conversion")
	endif()

elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  # using Intel C++
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # using Visual Studio C++
  # Disable Qt version tagging which causes segment errors on MSVC
  add_definitions(-DQT_NO_VERSION_TAGGING)
endif()

# set up profiling
if (SUBSURFACE_PROFILING_BUILD)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
	SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
	SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")
endif()
# set up ASan
if (SUBSURFACE_ASAN_BUILD)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
	SET(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
endif()

# Compiler-specific warning flags
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	# MSVC: /W4 is similar to -Wall, format warnings are on by default
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else()
	# GCC/Clang
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror=format")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror=format")
endif()

# by default optimize with -O2 even for debug builds (GCC/Clang only)
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	set(GCC_OPTIMIZATION_FLAGS "-O2" CACHE STRING "GCC optimization flags")
	message(STATUS "GCC optimization flags: " ${GCC_OPTIMIZATION_FLAGS})
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${GCC_OPTIMIZATION_FLAGS}")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${GCC_OPTIMIZATION_FLAGS}")
endif()

# pkgconfig for required libraries
find_package(PkgConfig)
include(pkgconfig_helper)

# The 'HandleFindXXX' are special libraries that subsurface needs
# to find and configure in a few different ways because of a few
# developers that prefer pkg-config over CMake, so we try to make
# everyone happy. It also sets some variables for each library, so
# if you think a module miss anything, take a look on the specific
# module file.
include(HandleFindGit2)
include(HandleFindLibDiveComputer)
include(HandleFindQLiteHtml)
include(HandleFtdiSupport)
include(HandleVersionGeneration)
include(RunOnBuildDir)
include(cmake_variables_helper)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	include_directories(${CMAKE_OSX_SYSROOT}/usr/include/libxml2)
	LIST(APPEND SUBSURFACE_LINK_LIBRARIES -lxml2 -lxslt -lsqlite3)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows" AND CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	# On Windows with MSVC/vcpkg, set paths directly
	# Use SUBSURFACE_VCPKG_ROOT to avoid conflicts with vcpkg toolchain variables
	# The vcpkg toolchain sets VCPKG_INSTALLED_DIR based on toolchain file location,
	# which can point to VS bundled vcpkg instead of the one we installed deps to
	if(DEFINED SUBSURFACE_VCPKG_ROOT)
		set(_VCPKG_PREFIX "${SUBSURFACE_VCPKG_ROOT}/installed/x64-windows")
	elseif(DEFINED ENV{VCPKG_ROOT})
		set(_VCPKG_PREFIX "$ENV{VCPKG_ROOT}/installed/x64-windows")
	else()
		set(_VCPKG_PREFIX "C:/vcpkg/installed/x64-windows")
	endif()
	message(STATUS "Using vcpkg prefix: ${_VCPKG_PREFIX}")

	# libxml2: headers are in include/libxml2/, library is libxml2.lib
	set(LIBXML2_INCLUDE_DIR "${_VCPKG_PREFIX}/include/libxml2")
	set(LIBXML2_LIBRARY "${_VCPKG_PREFIX}/lib/libxml2.lib")
	# libxslt: headers are in include/, library is libxslt.lib
	set(LIBXSLT_INCLUDE_DIR "${_VCPKG_PREFIX}/include")
	set(LIBXSLT_LIBRARIES "${_VCPKG_PREFIX}/lib/libxslt.lib")
	# sqlite3: library is sqlite3.lib
	set(SQLITE3_LIBRARY "${_VCPKG_PREFIX}/lib/sqlite3.lib")

	include_directories(${LIBXML2_INCLUDE_DIR} ${LIBXSLT_INCLUDE_DIR})

	LIST(APPEND SUBSURFACE_LINK_LIBRARIES ${LIBXML2_LIBRARY} ${LIBXSLT_LIBRARIES} ${SQLITE3_LIBRARY})
else()
	pkg_config_library(LIBXML libxml-2.0 REQUIRED)
	pkg_config_library(LIBSQLITE3 sqlite3 REQUIRED)
	pkg_config_library(LIBXSLT libxslt REQUIRED)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Windows" AND CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	# Use direct path - _VCPKG_PREFIX was set above
	LIST(APPEND SUBSURFACE_LINK_LIBRARIES "${_VCPKG_PREFIX}/lib/zip.lib")
else()
	pkg_config_library(LIBZIP libzip REQUIRED)
endif()

if(NOT ANDROID)
	if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
		pkg_config_library(BLUEZ bluez REQUIRED)
	endif()
	if(CMAKE_SYSTEM_NAME STREQUAL "Windows" AND CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
		find_package(libusb CONFIG QUIET)
		if(libusb_FOUND)
			set(LIBUSB_FOUND TRUE)
			get_target_property(LIBUSB_INCLUDE_DIRS libusb-1.0 INTERFACE_INCLUDE_DIRECTORIES)
			set(LIBUSB_LIBRARIES libusb-1.0)
			include_directories(${LIBUSB_INCLUDE_DIRS})
			LIST(APPEND SUBSURFACE_LINK_LIBRARIES ${LIBUSB_LIBRARIES})
		endif()
		find_package(libraw CONFIG QUIET)
		if(libraw_FOUND)
			set(LIBRAW_FOUND TRUE)
			LIST(APPEND SUBSURFACE_LINK_LIBRARIES libraw::raw_r)
		endif()
		# libmtp is not available via vcpkg, skip for now
	else()
		pkg_config_library(LIBUSB libusb-1.0 QUIET)
		pkg_config_library(LIBMTP libmtp QUIET)
		pkg_config_library(LIBRAW libraw_r )
	endif()
endif()

include_directories(.
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_BINARY_DIR}
	${CMAKE_BINARY_DIR}/desktop-widgets
)

# decide what to do about Qt
# this is messy because we want to support older Qt5 versions, newer Qt5 versions,
# and Qt6 when enabled / available.
if(BUILD_WITH_QT6)
	set(CHECK_QT6 "Qt6")
endif()

# figure out which version of Qt we are building against
# in theory this should get us all the variables set up correctly, but that
# ended up failing in subtle ways to do what was advertized; in the Qt5 case
# some of the variables didn't get set up, so we'll immediately call it again
# for Qt5
find_package(QT NAMES ${CHECK_QT6} Qt5 REQUIRED COMPONENTS Core Widgets)

# right now there are a few things that don't work with Qt6
# let's disable them right here and remember our Qt version
if(QT_VERSION_MAJOR STREQUAL "6")
	set(USINGQT6 ON)
	set(QT5OR6 "")
	# Get Qt6 install prefix from Qt6_DIR (which is like /path/to/Qt/lib/cmake/Qt6)
	# Go up 3 directory levels to get the Qt install prefix
	get_filename_component(QT_INSTALL_PREFIX "${Qt6_DIR}" DIRECTORY)
	get_filename_component(QT_INSTALL_PREFIX "${QT_INSTALL_PREFIX}" DIRECTORY)
	get_filename_component(QT_INSTALL_PREFIX "${QT_INSTALL_PREFIX}" DIRECTORY)
	# for Qt6 we want the Qt5 compatibility package
	LIST(APPEND QT_EXTRA_COMPONENTS Core5Compat)
	LIST(APPEND QT_TEST_LIBRARIES Qt::Core5Compat)

	# as of Qt 6.3 by default QtLocation isn't included, but one can
	# build the module from source; so let's look if the user has installed
	# it (or if a newer version is out that includes it, anyway)
	find_package(Qt6 COMPONENTS Location)
	if(Qt6Location_FOUND)
		message(STATUS "Found Qt6Location")
		LIST(APPEND QT_EXTRA_COMPONENTS Location)
		add_definitions(-DMAP_SUPPORT)
		set(MAPSUPPORT ON)

		# at least on macOS something goes sideways and we fail to load the
		# QML files with a weird error about Qml.WorkerScript missing
		# linking Subsurface against that component appears enough to hack
		# around that, but that feels... wrong
		if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
			find_package(Qt6 COMPONENTS QmlWorkerScript)
			LIST(APPEND SUBSURFACE_LINK_LIBRARIES Qt6::QmlWorkerScript)
		endif()
	endif()
else()
	# as mentioned above, since some variables appear to fail to populate
	# on some platforms, simply run this again the 'old' way
	find_package(Qt5 REQUIRED COMPONENTS Core Widgets)
	set(USINGQT6 OFF)
	set(QT5OR6 "5")
	set(QT_VERSION ${Qt5_VERSION})
	set(QT_INSTALL_PREFIX ${_qt5Core_install_prefix})
	# for Qt5 we want the Location component (which is missing so far in Qt6)
	LIST(APPEND QT_EXTRA_COMPONENTS Location)
	add_definitions(-DMAP_SUPPORT)
	set(MAPSUPPORT ON)
endif()
message(STATUS "building with Qt ${QT_VERSION}")

# Project Target specific configuration should go here,
# if the configuration is too big or would disrupt the normal code flux,
# move it somewhere else (another file) and include it.
if (SUBSURFACE_TARGET_EXECUTABLE MATCHES "DesktopExecutable")
	if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
		set(SUBSURFACE_TARGET Subsurface)
	else()
		set(SUBSURFACE_TARGET subsurface)
	endif()
	list(APPEND QT_EXTRA_COMPONENTS QuickWidgets)
	remove_definitions(-DSUBSURFACE_MOBILE)
	if(NO_USERMANUAL)
		message(STATUS "building without built-in user manual support")
		add_definitions(-DNO_USERMANUAL)
	else()
		set(REQUEST_QLITEHTML ON)
		set(REQUEST_WEBKIT ON)
		set(REQUEST_WEBENGINE ON)
	endif()
	if(NO_PRINTING)
		message(STATUS "building without printing support")
		add_definitions(-DNO_PRINTING)
	else()
		set(REQUEST_WEBKIT ON)
		set(REQUEST_WEBENGINE ON)
	endif()

	# Try QLiteHtml first, but ONLY for Qt6 builds
	if(REQUEST_QLITEHTML AND USINGQT6 AND QLITEHTML_FOUND)
		message(STATUS "building with QLiteHtml (Qt6)")
		LIST(APPEND QT_EXTRA_COMPONENTS PrintSupport)
		LIST(APPEND SUBSURFACE_LINK_LIBRARIES ${QLITEHTML_LIBRARIES})
		add_definitions(-DUSE_QLITEHTML)
		add_definitions(-DNO_PRINTING)
		set(NO_PRINTING ON)
		set(REQUEST_WEBKIT OFF)
		set(REQUEST_WEBENGINE OFF)
	endif()

	# Try WebKit next (Qt5 only, has best printing support)
	if(REQUEST_WEBKIT)
		# Because Qt5WebKitWidgets isn't a part of the "regular" Qt5, we can't get it the normal way
		find_package(Qt5WebKitWidgets QUIET)
		if(Qt5WebKitWidgets_FOUND STREQUAL "1")
			LIST(APPEND QT_EXTRA_COMPONENTS PrintSupport WebKitWidgets)
			LIST(APPEND SUBSURFACE_LINK_LIBRARIES Qt5::WebKitWidgets)
			message(STATUS "building with QtWebKit")
			set(REQUEST_WEBENGINE OFF)
		endif()
	endif()
	if(REQUEST_WEBENGINE)
		if(USINGQT6)
			find_package(Qt6WebEngineWidgets)
			if(Qt6WebEngineWidgets_FOUND STREQUAL "1")
				LIST(APPEND QT_EXTRA_COMPONENTS PrintSupport WebEngineWidgets)
				LIST(APPEND SUBSURFACE_LINK_LIBRARIES Qt6::WebEngineWidgets)
				message(STATUS "building with QtWebEngine, disabling printing support")
				set(NO_PRINTING ON)
				add_definitions(-DNO_PRINTING)
				add_definitions(-DUSE_WEBENGINE)
			else()
				message(STATUS "didn't find QtWebEngine, QtWebKit, or QLiteHtml, building without user manual and printing support")
				add_definitions(-DNO_PRINTING)
				add_definitions(-DNO_USERMANUAL)
				set(NO_PRINTING ON)
				set(NO_USERMANUAL ON)
			endif()
		else()
			find_package(Qt5WebEngineWidgets)
			if(Qt5WebEngineWidgets_FOUND STREQUAL "1")
				LIST(APPEND QT_EXTRA_COMPONENTS PrintSupport WebEngineWidgets)
				LIST(APPEND SUBSURFACE_LINK_LIBRARIES Qt5::WebEngineWidgets)
				message(STATUS "building with QtWebEngine, disabling printing support")
				set(NO_PRINTING ON)
				add_definitions(-DNO_PRINTING)
				add_definitions(-DUSE_WEBENGINE)
			else()
				message(STATUS "didn't find QtWebEngine nor QtWebKit, building without user manual and printing support")
				add_definitions(-DNO_PRINTING)
				add_definitions(-DNO_USERMANUAL)
				set(NO_PRINTING ON)
				set(NO_USERMANUAL ON)
			endif()
		endif()
	endif()
elseif (SUBSURFACE_TARGET_EXECUTABLE MATCHES "MobileExecutable")
	if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
		set(SUBSURFACE_TARGET Subsurface-mobile)
	else()
		set(SUBSURFACE_TARGET subsurface-mobile)
	endif()
	list(APPEND QT_EXTRA_COMPONENTS QuickControls2)
	list(APPEND QT_EXTRA_COMPONENTS QuickWidgets)
	add_definitions(-DSUBSURFACE_MOBILE)

	# add definition to seperate mobile for devices and for desktop
	add_definitions(-DSUBSURFACE_MOBILE_DESKTOP)

	message(STATUS "Building Subsurface-mobile requires BT support")
	set(BTSUPPORT ON)
elseif (SUBSURFACE_TARGET_EXECUTABLE MATCHES "DownloaderExecutable")
	if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
		set(SUBSURFACE_TARGET Subsurface-downloader)
	else()
		set(SUBSURFACE_TARGET subsurface-downloader)
	endif()
	set(BTSUPPORT ON)
	set(LIBRAW_SUPPORT OFF)
	add_definitions(-DSUBSURFACE_DOWNLOADER)
	message(STATUS "building the embedded Subsurface-downloader app")
endif()

if(ANDROID)
	set(NO_PRINTING ON)
	set(NO_USERMANUAL ON)
	set(BUILD_TESTS OFF)
	list(APPEND QT_EXTRA_COMPONENTS AndroidExtras)
	LIST(APPEND SUBSURFACE_LINK_LIBRARIES -llog)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	# add pthread to the end of the library list on Linux
	# this is only needed on Ubuntu
	# but shouldn't hurt on other Linux versions
	# in some builds we appear to be missing libz for some strange reason...
	LIST(APPEND SUBSURFACE_LINK_LIBRARIES -lz -lpthread)

	# Test for ARM processor (Raspberry Pi) and add libGLESv2 if found
	if (CMAKE_SYSTEM_PROCESSOR STREQUAL "armv7l" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "armv6l")
		message (STATUS "Found ARM processor. Adding libGLESv2")
		LIST(APPEND SUBSURFACE_LINK_LIBRARIES -lGLESv2)
		set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
	endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	execute_process(
		COMMAND bash scripts/get-version.sh
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		OUTPUT_VARIABLE SSRF_VERSION_STRING
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	find_library(APP_SERVICES_LIBRARY ApplicationServices)
	find_library(HID_LIB HidApi)
	LIST(APPEND SUBSURFACE_LINK_LIBRARIES ${HID_LIB})
	set(EXTRA_LIBS ${APP_SERVICES_LIBRARY})
	set(ICON_FILE ${CMAKE_SOURCE_DIR}/packaging/macosx/Subsurface.icns)
	if (SUBSURFACE_TARGET_EXECUTABLE MATCHES "MobileExecutable")
		set(MACOSX_BUNDLE_INFO_STRING "Subsurface-mobile")
		set(MACOSX_BUNDLE_BUNDLE_NAME "Subsurface-mobile")
	else()
		set(MACOSX_BUNDLE_INFO_STRING "Subsurface")
		set(MACOSX_BUNDLE_BUNDLE_NAME "Subsurface")
	endif()
	set(MACOSX_BUNDLE_ICON_FILE Subsurface.icns)
	set(MACOSX_BUNDLE_GUI_IDENTIFIER "org.subsurface-divelog")
	set(MACOSX_BUNDLE_BUNDLE_VERSION "${SSRF_VERSION_STRING}")
	set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${SSRF_VERSION_STRING}")
	set(MACOSX_BUNDLE_LONG_VERSION_STRING "${SSRF_VERSION_STRING}")
	set(MACOSX_BUNDLE_COPYRIGHT "Linus Torvalds, Dirk Hohndel, Tomaz Canabrava, Berthold Stoeger and others")
	# Use custom Info.plist template with Bluetooth permissions
	set(MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/Modules/MacOSXBundleInfo.plist.in")
	set_source_files_properties(${ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
	set(SUBSURFACE_PKG MACOSX_BUNDLE ${ICON_FILE})
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
		LIST(APPEND SUBSURFACE_LINK_LIBRARIES wsock32 ws2_32)
		add_definitions(-D_WIN32)
	else()
		# GCC/MinGW
		LIST(APPEND SUBSURFACE_LINK_LIBRARIES -lwsock32 -lws2_32)
		add_definitions(-mwindows -D_WIN32)
	endif()
	remove_definitions(-DUNICODE)
endif()

if(BTSUPPORT)
	set(BLESUPPORT ON)
	list(APPEND QT_EXTRA_COMPONENTS Bluetooth)
	add_definitions(-DBT_SUPPORT)
	add_definitions(-DBLE_SUPPORT)
endif()

if (LIBRAW_SUPPORT)
	if(LIBRAW_FOUND)
		add_definitions(-DLIBRAW_SUPPORT)
	endif()
else()
	message(STATUS "building without built-in libraw support")
endif()

if(ANDROID)
	# when building for Android, the toolchain file requires all cmake modules
	# to be inside the CMAKE_FIND_ROOT_PATH - which prevents cmake from finding
	# our Qt installation. This is ugly, but it works.
	set(CMAKE_FIND_ROOT_PATH "/;${CMAKE_FIND_ROOT_PATH}")
endif()
set(QT_FIND_COMPONENTS Core Concurrent Widgets Network Svg Positioning Quick ${QT_EXTRA_COMPONENTS})
if (SUBSURFACE_TARGET_EXECUTABLE MATCHES "DesktopExecutable")
	find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_FIND_COMPONENTS} LinguistTools Test QuickTest)
elseif (SUBSURFACE_TARGET_EXECUTABLE MATCHES "MobileExecutable")
	# Kirigami 5.62 and newer require at least Qt 5.12
	if(ANDROID)
		find_package(Qt${QT_VERSION_MAJOR} 5.12 REQUIRED COMPONENTS ${QT_FIND_COMPONENTS} LinguistTools)
	else()
		find_package(Qt${QT_VERSION_MAJOR} 5.12 REQUIRED COMPONENTS ${QT_FIND_COMPONENTS} LinguistTools Test QuickTest)
	endif()
elseif (SUBSURFACE_TARGET_EXECUTABLE MATCHES "DownloaderExecutable")
	# let's pick a version that's not ancient
	find_package(Qt${QT_VERSION_MAJOR} 5.11 REQUIRED COMPONENTS ${QT_FIND_COMPONENTS})
	set(BUILD_TESTS OFF)
endif()

foreach(_QT_COMPONENT ${QT_FIND_COMPONENTS})
	list(APPEND QT_LIBRARIES Qt${QT5OR6}::${_QT_COMPONENT})
endforeach()
if(NOT ANDROID)
	LIST(APPEND QT_TEST_LIBRARIES ${QT_LIBRARIES} Qt${QT5OR6}::Test Qt${QT5OR6}::QuickTest)
endif()

#set up the subsurface_link_libraries variable
LIST(APPEND SUBSURFACE_LINK_LIBRARIES ${LIBDIVECOMPUTER_LIBRARIES} ${LIBGIT2_LIBRARIES} ${LIBUSB_LIBRARIES} ${LIBMTP_LIBRARIES})
if (NOT SUBSURFACE_TARGET_EXECUTABLE MATCHES "DownloaderExecutable")
	if(USINGQT6)
		if(MAPSUPPORT)
			qt_add_resources(SUBSURFACE_RESOURCES map-widget/qml/map-widget.qrc)
			set(SUBSURFACE_MAPWIDGET subsurface_mapwidget)
		endif()
		qt_add_resources(SUBSURFACE_RESOURCES subsurface.qrc profile.qrc stats/statsicons.qrc desktop-widgets/qml/statsview2.qrc)
	else()
		qt5_add_resources(SUBSURFACE_RESOURCES subsurface.qrc profile.qrc stats/statsicons.qrc map-widget/qml/map-widget.qrc desktop-widgets/qml/statsview2.qrc)
		set(SUBSURFACE_MAPWIDGET subsurface_mapwidget)
	endif()
endif()

# hack to build successfully on LGTM
if(DEFINED ENV{LGTM_SRC})
	LIST(APPEND SUBSURFACE_LINK_LIBRARIES -lgssapi_krb5 -lhttp_parser)
endif()

# include translations
if (NOT SUBSURFACE_TARGET_EXECUTABLE MATCHES "DownloaderExecutable")
add_subdirectory(translations)
endif()
add_subdirectory(core)
add_subdirectory(qt-models)
add_subdirectory(commands)
if (NOT SUBSURFACE_TARGET_EXECUTABLE MATCHES "DownloaderExecutable")
add_subdirectory(profile-widget)
if(MAPSUPPORT)
add_subdirectory(map-widget)
endif()
add_subdirectory(mobile-widgets)
add_subdirectory(stats)
endif()
add_subdirectory(backend-shared)

if (SUBSURFACE_TARGET_EXECUTABLE MATCHES "DesktopExecutable")
	add_subdirectory(desktop-widgets)
endif()

# create the executables
if (SUBSURFACE_TARGET_EXECUTABLE MATCHES "MobileExecutable")
	# set up Kirigami using KDE ECM
	# that's available as kde-extra-cmake-modules on Homebrew, on all Linux flavors
	# Android and iOS are built via qmake, Windows build of Subsurface-mobile isn't supported
	find_package(ECM REQUIRED CONFIG PATHS ${CMAKE_CURRENT_SOURCE_DIR}/mobile-widgets/3rdparty/ECM)
	set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})
	set(BREEZEICONS_DIR mobile-widgets/3rdparty/breeze-icons/)

	SET(QML_IMPORT_PATH ${QML_IMPORT_PATH} ${CMAKE_SOURCE_DIR}/mobile-widgets/3rdparty/kirigami/src ${CMAKE_SOURCE_DIR}/mobile-widgets/qml)
	add_subdirectory(mobile-widgets/3rdparty)

	set(MOBILE_SRC
		subsurface-mobile-main.cpp
		subsurface-helper.cpp
	)
	if(USINGQT6)
		qt_add_resources(MOBILE_RESOURCES mobile-widgets/qml/mobile-resources.qrc)
		qt_add_resources(MOBILE_RESOURCES profile.qrc)
		qt_add_resources(MOBILE_RESOURCES mobile-widgets/3rdparty/icons.qrc)
		qt_add_resources(MOBILE_RESOURCES mobile-widgets/3rdparty/kirigami/src/scenegraph/shaders/shaders.qrc)
	else()
		qt5_add_resources(MOBILE_RESOURCES mobile-widgets/qml/mobile-resources.qrc)
		qt5_add_resources(MOBILE_RESOURCES profile.qrc)
		qt5_add_resources(MOBILE_RESOURCES mobile-widgets/3rdparty/icons.qrc)
		qt5_add_resources(MOBILE_RESOURCES mobile-widgets/3rdparty/kirigami/src/scenegraph/shaders/shaders.qrc)
	endif()

	# the following is split across two commands since in cmake 3.12 this would result
	# in a non-sensical "no sources given to target" error if done all as one set of
	# arguments to the add_executable() call
	add_executable(${SUBSURFACE_TARGET} ${SUBSURFACE_PKG} ${SUBSURFACE_RESOURCES})
	target_sources(${SUBSURFACE_TARGET} PUBLIC ${MOBILE_SRC} ${MOBILE_RESOURCES})
	target_link_libraries(
		${SUBSURFACE_TARGET}
		subsurface_mobile
		subsurface_profile
		${SUBSURFACE_MAPWIDGET}
		subsurface_backend_shared
		subsurface_models_mobile
		subsurface_commands
		subsurface_corelib
		subsurface_stats
		kirigamiplugin
		${SUBSURFACE_LINK_LIBRARIES}
	)
elseif (SUBSURFACE_TARGET_EXECUTABLE MATCHES "DesktopExecutable")
	set(SUBSURFACE_APP
		subsurface-desktop-main.cpp
		subsurface-helper.cpp
	)
	source_group("Subsurface App" FILES ${SUBSURFACE_APP})
	if(ANDROID)
		add_library(${SUBSURFACE_TARGET} SHARED ${SUBSURFACE_PKG} ${SUBSURFACE_APP} ${SUBSURFACE_RESOURCES})
	else()
		add_executable(${SUBSURFACE_TARGET} MACOSX_BUNDLE WIN32 ${SUBSURFACE_PKG} ${SUBSURFACE_APP} ${SUBSURFACE_RESOURCES})
	endif()

	# Import Qt6 Bluetooth permission plugin on macOS
	if(USINGQT6 AND CMAKE_SYSTEM_NAME STREQUAL "Darwin")
		qt_import_plugins(${SUBSURFACE_TARGET}
			INCLUDE Qt::QDarwinBluetoothPermissionPlugin
		)
	endif()

	target_link_libraries(
		${SUBSURFACE_TARGET}
		subsurface_generated_ui
		subsurface_interface
		subsurface_profile
		${SUBSURFACE_MAPWIDGET}
		subsurface_backend_shared
		subsurface_models_desktop
		subsurface_commands
		subsurface_corelib
		subsurface_stats
		${SUBSURFACE_LINK_LIBRARIES}
	)
	add_dependencies(subsurface_desktop_preferences subsurface_generated_ui)
	add_dependencies(subsurface_interface subsurface_generated_ui)
	add_dependencies(subsurface_profile subsurface_generated_ui)
	add_dependencies(subsurface_models_desktop subsurface_generated_ui)
	add_dependencies(subsurface_generated_ui version)
elseif (SUBSURFACE_TARGET_EXECUTABLE MATCHES "DownloaderExecutable")
	set(DOWNLOADER_APP
		subsurface-downloader-main.cpp
		cli-downloader.cpp
	)
	source_group("Downloader App" FILES ${DOWNLOADER_APP})
	add_executable(${SUBSURFACE_TARGET} MACOSX_BUNDLE WIN32 ${SUBSURFACE_PKG} ${DOWNLOADER_APP} ${SUBSURFACE_RESOURCES})
	target_link_libraries(
		${SUBSURFACE_TARGET}
		subsurface_backend_shared
		subsurface_models_downloader
		subsurface_commands
		subsurface_corelib
		${SUBSURFACE_LINK_LIBRARIES}
	)
endif()

add_dependencies(subsurface_corelib version)

# add platform specific actions
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	file(WRITE ${CMAKE_BINARY_DIR}/qt.conf "[Paths]
Prefix=.
")
endif()

# build an automated html exporter
add_executable(export-html EXCLUDE_FROM_ALL export-html.cpp ${SUBSURFACE_RESOURCES})
target_link_libraries(export-html subsurface_corelib ${SUBSURFACE_LINK_LIBRARIES})

# install Subsurface
# first some variables with files that need installing
set(DOCFILES
	README.md
	ReleaseNotes/ReleaseNotes.txt
	SupportedDivecomputers.txt
	${CMAKE_BINARY_DIR}/Documentation/user-manual.html
	${CMAKE_BINARY_DIR}/Documentation/user-manual_es.html
        ${CMAKE_BINARY_DIR}/Documentation/user-manual_fr.html
        ${CMAKE_BINARY_DIR}/Documentation/user-manual_de.html
)

# add all the translations that we may need
FILE(STRINGS "subsurface_enabled_translations" QTTRANSLATIONS_BASE)

if(NOT DEFINED QT_TRANSLATION_DIR OR QT_TRANSLATION_DIR STREQUAL "")
	if(USINGQT6)
		set(QT_TRANSLATION_DIR ${QtCore_DIR}/../../../translations)
	else()
		set(QT_TRANSLATION_DIR ${Qt5Core_DIR}/../../../translations)
	endif()
endif()
set(QTTRANSLATIONS "")
foreach(QTTRANSLATION ${QTTRANSLATIONS_BASE})
	if (QTTRANSLATION AND EXISTS ${QT_TRANSLATION_DIR}/${QTTRANSLATION})
		set(QTTRANSLATIONS ${QTTRANSLATIONS} ${QT_TRANSLATION_DIR}/${QTTRANSLATION})
	endif()
endforeach()

# now for each platform the install instructions
if(ANDROID)
	# Android template directory
	include(${QT_ANDROID_CMAKE})
	if((DEFINED ENV{KEYSTORE}) AND (DEFINED ENV{KEYSTORE_PASSWORD}))
		add_qt_android_apk(${SUBSURFACE_TARGET}.apk ${SUBSURFACE_TARGET}
			PACKAGE_SOURCES ${CMAKE_BINARY_DIR}/android-mobile DEPENDS ${ANDROID_NATIVE_LIBSSL} ${ANDROID_NATIVE_LIBCRYPT}
			KEYSTORE $ENV{KEYSTORE} Subsurface-mobile KEYSTORE_PASSWORD $ENV{KEYSTORE_PASSWORD}
		)
		message(STATUS "KEYSTORE=$ENV{KEYSTORE} KEYSTORE_PASSWORD=$ENV{KEYSTORE_PASSWORD}")
	else()
		add_qt_android_apk(${SUBSURFACE_TARGET}.apk ${SUBSURFACE_TARGET}
			PACKAGE_SOURCES ${CMAKE_BINARY_DIR}/android-mobile DEPENDS ${ANDROID_NATIVE_LIBSSL} ${ANDROID_NATIVE_LIBCRYPT}
		)
		message(STATUS "no KEYSTORE")
	endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	set(APP_BUNDLE_DIR "${SUBSURFACE_TARGET}.app")
	# macdeployqt simplifies a lot of this process, but still doesn't get everything right
	# - it misses a couple of resources and frameworks
	# - it seems to always deploy the SQL plugins (even though they aren't needed)
	set(MACDEPLOY_ARGS "-qmldir=${APP_BUNDLE_DIR}/Contents/Resources/qml -appstore-compliant -verbose=0 -executable=${APP_BUNDLE_DIR}/Contents/MacOS/${SUBSURFACE_TARGET} -always-overwrite -libpath=${CMAKE_SOURCE_DIR}/../install-root/lib")
	set(RESOURCEDIR ${CMAKE_BINARY_DIR}/${APP_BUNDLE_DIR}/Contents/Resources)
	set(PLUGINDIR ${CMAKE_BINARY_DIR}/${APP_BUNDLE_DIR}/Contents/PlugIns)
	if(BUILD_DOCS OR INSTALL_DOCS)
		install(DIRECTORY Documentation/images DESTINATION ${RESOURCEDIR}/share/Documentation)
		install(FILES ${DOCFILES} DESTINATION ${RESOURCEDIR}/share/Documentation)
	endif()
	install(DIRECTORY theme DESTINATION ${RESOURCEDIR})
	install(DIRECTORY printing_templates DESTINATION ${RESOURCEDIR})
	install(FILES ${TRANSLATIONS} DESTINATION ${RESOURCEDIR}/translations)
	install(FILES ${QTTRANSLATIONS} DESTINATION ${RESOURCEDIR}/translations)
	install(FILES ${CMAKE_SOURCE_DIR}/gpl-2.0.txt DESTINATION ${RESOURCEDIR})
	install(CODE "execute_process(COMMAND mkdir -p ${RESOURCEDIR}/qml)")
	if(USINGQT6)
		set(QT_INSTALL_PREFIX ${QT6_INSTALL_PREFIX})
	else()
		set(QT_INSTALL_PREFIX ${_qt5Core_install_prefix})
	endif()
	if(MAPSUPPORT)
		install(CODE "execute_process(COMMAND mkdir -p ${PLUGINDIR}/geoservices)")
		# this is really weird. We first try a plugin that ended up in the Qt install prefix
		# then we try one that's in that odd broken install location that the qmake file results in (that includes the QT_INSTALL_PREFIX after our INSTALL_ROOT
		# and finally, for fat binaries, we copy the one that's in the 'logical' spot under the INSTALL_ROOT
		# this should cover all cases and always get us the correct library
		install(CODE "execute_process(COMMAND cp ${QT_INSTALL_PREFIX}/plugins/geoservices/libqtgeoservices_googlemaps.dylib ${PLUGINDIR}/geoservices ERROR_QUIET)")
		install(CODE "execute_process(COMMAND cp ${CMAKE_SOURCE_DIR}/../install-root/${QT_INSTALL_PREFIX}/plugins/geoservices/libqtgeoservices_googlemaps.dylib ${PLUGINDIR}/geoservices ERROR_QUIET)")
		install(CODE "execute_process(COMMAND cp ${CMAKE_SOURCE_DIR}/../install-root/plugins/geoservices/libqtgeoservices_googlemaps.dylib ${PLUGINDIR}/geoservices ERROR_QUIET)")
	endif()
	# this will fail is macdeployqt isn't in the PATH - that seemed to happen in the past, but not recently
	# also, on M1 macOS systems macdeployqt throws a ton of (apparently harmless) errors. Warn the unsuspecting developer
	install(CODE "execute_process(COMMAND macdeployqt ${APP_BUNDLE_DIR} -no-strip ${MACDEPLOY_ARGS})")
	# the next hack is here to delete the sqlite plugin that get's installed even though it isn't needed
	install(CODE "execute_process(COMMAND rm -rf ${CMAKE_BINARY_DIR}/${APP_BUNDLE_DIR}/Contents/PlugIns/sqldrivers)")
	# and another hack to get the QML Components in the right place
	if(MAPSUPPORT)
		if (USINGQT6)
			install(CODE "execute_process(COMMAND rm -rf ${CMAKE_BINARY_DIR}/${APP_BUNDLE_DIR}/Contents/Resources/qml/{QtQuick,QtLocation,QtPositioning})")
			install(CODE "execute_process(COMMAND cp -a ${QT_INSTALL_PREFIX}/qml/QtQuick ${CMAKE_BINARY_DIR}/${APP_BUNDLE_DIR}/Contents/Resources/qml)")
		else()
			install(CODE "execute_process(COMMAND rm -rf ${CMAKE_BINARY_DIR}/${APP_BUNDLE_DIR}/Contents/Resources/qml/{QtQuick.2,QtLocation,QtPositioning})")
			install(CODE "execute_process(COMMAND cp -a ${QT_INSTALL_PREFIX}/qml/QtQuick.2 ${CMAKE_BINARY_DIR}/${APP_BUNDLE_DIR}/Contents/Resources/qml)")
		endif()
		install(CODE "execute_process(COMMAND cp -a ${QT_INSTALL_PREFIX}/qml/QtLocation ${CMAKE_BINARY_DIR}/${APP_BUNDLE_DIR}/Contents/Resources/qml)")
	endif()
	install(CODE "execute_process(COMMAND cp -a ${QT_INSTALL_PREFIX}/qml/QtPositioning ${CMAKE_BINARY_DIR}/${APP_BUNDLE_DIR}/Contents/Resources/qml)")
	if(NOT QT_VERSION VERSION_LESS 5.11.0)
		# and with Qt 5.11 we need another library that isn't copied by macdeployqt
		install(CODE "execute_process(COMMAND rm -rf ${CMAKE_BINARY_DIR}/${APP_BUNDLE_DIR}/Contents/Frameworks/QtPositioningQuick.framework)")
		install(CODE "execute_process(COMMAND cp -a ${QT_INSTALL_PREFIX}/lib/QtPositioningQuick.framework ${CMAKE_BINARY_DIR}/${APP_BUNDLE_DIR}/Contents/Frameworks)")
	endif()
	if(NOT QT_VERSION VERSION_LESS 5.14.0)
		# and with Qt 5.14 we need another library that isn't always copied by macdeployqt
		install(CODE "execute_process(COMMAND rm -rf ${CMAKE_BINARY_DIR}/${APP_BUNDLE_DIR}/Contents/Frameworks/QtQmlWorkerScript.framework)")
		install(CODE "execute_process(COMMAND cp -a ${QT_INSTALL_PREFIX}/lib/QtQmlWorkerScript.framework ${CMAKE_BINARY_DIR}/${APP_BUNDLE_DIR}/Contents/Frameworks)")
	endif()
	if (SUBSURFACE_TARGET_EXECUTABLE MATCHES "MobileExecutable")
		install(CODE "execute_process(COMMAND cp -a ${QT_INSTALL_PREFIX}/qml/QtQuick ${CMAKE_BINARY_DIR}/${APP_BUNDLE_DIR}/Contents/Resources/qml)")
		install(CODE "execute_process(COMMAND cp -a ${QT_INSTALL_PREFIX}/qml/QtGraphicalEffects ${CMAKE_BINARY_DIR}/${APP_BUNDLE_DIR}/Contents/Resources/qml)")
		install(CODE "execute_process(COMMAND cp -a ${QT_INSTALL_PREFIX}/qml/QtQml ${CMAKE_BINARY_DIR}/${APP_BUNDLE_DIR}/Contents/Resources/qml)")
		install(CODE "execute_process(COMMAND cp -a ${QT_INSTALL_PREFIX}/qml/QtPositioning ${CMAKE_BINARY_DIR}/${APP_BUNDLE_DIR}/Contents/Resources/qml)")
	endif()
	install(CODE "execute_process(COMMAND codesign --force --deep --sign - ${APP_BUNDLE_DIR})")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	if (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
		if(NOT DEFINED OBJCOPY)
			set(OBJCOPY x86_64-w64-mingw32.shared-objcopy)
		endif()
		find_program(OBJCOPY_FOUND ${OBJCOPY})
		if (OBJCOPY_FOUND)
			message(STATUS "Build type is 'RelWithDebInfo'. Creating debug symbols in a separate file.")
			add_custom_command(TARGET ${SUBSURFACE_TARGET} POST_BUILD
				COMMAND ${OBJCOPY} --only-keep-debug ${SUBSURFACE_TARGET}.exe ${SUBSURFACE_TARGET}.exe.debug
				COMMAND ${OBJCOPY} --strip-debug --strip-unneeded ${SUBSURFACE_TARGET}.exe
				COMMAND ${OBJCOPY} --add-gnu-debuglink=${SUBSURFACE_TARGET}.exe.debug ${SUBSURFACE_TARGET}.exe
			)
		endif()
	endif()

	# Windows bundling rules
	# We don't have a helpful tool like macdeployqt for Windows, so we hardcode
	# which libs we need.
	# "make install", copies everything into a staging area
	# "make installer", uses makensis to create an installer executable
	set(WINDOWSSTAGING ${CMAKE_BINARY_DIR}/staging)
	if(BUILD_DOCS OR INSTALL_DOCS)
		install(DIRECTORY Documentation/images DESTINATION ${WINDOWSSTAGING}/Documentation)
		install(FILES ${DOCFILES} DESTINATION ${WINDOWSSTAGING}/Documentation)
	endif()
	install(DIRECTORY theme DESTINATION ${WINDOWSSTAGING})
	install(DIRECTORY printing_templates DESTINATION ${WINDOWSSTAGING})
	install(FILES ${TRANSLATIONS} DESTINATION ${WINDOWSSTAGING}/translations)
	install(FILES ${QTTRANSLATIONS} DESTINATION ${WINDOWSSTAGING}/translations)
	install(FILES ${CMAKE_SOURCE_DIR}/gpl-2.0.txt ${CMAKE_SOURCE_DIR}/packaging/windows/subsurface.ico DESTINATION ${WINDOWSSTAGING})
	install(TARGETS ${SUBSURFACE_TARGET} DESTINATION ${WINDOWSSTAGING})
	install(FILES ${CMAKE_BINARY_DIR}/qt.conf DESTINATION ${WINDOWSSTAGING})

	if(NOT QT_VERSION VERSION_LESS 5.11.0)
		# hack to work around the fact that we don't process the dependencies of plugins
		# as of Qt 5.11 this additional DLL is needed and it's only referenced in the qml DLLs
		if(USINGQT6)
			install(FILES ${QT_INSTALL_PREFIX}/bin/Qt6PositioningQuick.dll DESTINATION ${WINDOWSSTAGING})
		else()
			install(FILES ${QT_INSTALL_PREFIX}/bin/Qt5PositioningQuick.dll DESTINATION ${WINDOWSSTAGING})
		endif()
	endif()

	if(NOT DEFINED MAKENSIS)
		set(MAKENSIS makensis)
	endif()

	# Copy DLLs at install time
	if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
		# For MSVC builds, use windeployqt to copy Qt dependencies
		# and copy vcpkg DLLs
		find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT_INSTALL_PREFIX}/bin")
		if(WINDEPLOYQT_EXECUTABLE)
			message(STATUS "Found windeployqt: ${WINDEPLOYQT_EXECUTABLE}")
			install(CODE "
				message(STATUS \"Running windeployqt on ${WINDOWSSTAGING}/${SUBSURFACE_TARGET}.exe\")
				execute_process(
					COMMAND \"${WINDEPLOYQT_EXECUTABLE}\"
						--verbose 2
						--no-opengl-sw
						--no-system-d3d-compiler
						--qmldir \"${CMAKE_SOURCE_DIR}/mobile-widgets/qml\"
						--qmldir \"${CMAKE_SOURCE_DIR}/map-widget\"
						--qmldir \"${CMAKE_SOURCE_DIR}/stats\"
						\"${WINDOWSSTAGING}/${SUBSURFACE_TARGET}.exe\"
					WORKING_DIRECTORY \"${WINDOWSSTAGING}\"
					RESULT_VARIABLE _windeployqt_result
					OUTPUT_VARIABLE _windeployqt_output
					ERROR_VARIABLE _windeployqt_error
				)
				message(STATUS \"windeployqt result: \${_windeployqt_result}\")
				if(_windeployqt_output)
					message(STATUS \"windeployqt output: \${_windeployqt_output}\")
				endif()
				if(_windeployqt_error)
					message(STATUS \"windeployqt errors: \${_windeployqt_error}\")
				endif()
				if(NOT _windeployqt_result EQUAL 0)
					message(WARNING \"windeployqt failed with code \${_windeployqt_result}\")
				endif()
			")
		else()
			message(WARNING "windeployqt not found - Qt DLLs will not be copied automatically")
		endif()

		# Manually copy Qt plugins since windeployqt doesn't always work reliably
		install(CODE "
			message(STATUS \"Copying Qt plugins from ${QT_INSTALL_PREFIX}/plugins...\")
			# Create plugin directories
			file(MAKE_DIRECTORY \"${WINDOWSSTAGING}/plugins/platforms\")
			file(MAKE_DIRECTORY \"${WINDOWSSTAGING}/plugins/styles\")
			file(MAKE_DIRECTORY \"${WINDOWSSTAGING}/plugins/imageformats\")
			file(MAKE_DIRECTORY \"${WINDOWSSTAGING}/plugins/iconengines\")
			file(MAKE_DIRECTORY \"${WINDOWSSTAGING}/plugins/tls\")
			file(MAKE_DIRECTORY \"${WINDOWSSTAGING}/plugins/networkinformation\")
			file(MAKE_DIRECTORY \"${WINDOWSSTAGING}/plugins/geoservices\")
			file(MAKE_DIRECTORY \"${WINDOWSSTAGING}/plugins/position\")
			file(MAKE_DIRECTORY \"${WINDOWSSTAGING}/plugins/generic\")

			# Copy platform plugins (critical - qwindows.dll is required)
			file(GLOB _platform_plugins \"${QT_INSTALL_PREFIX}/plugins/platforms/qwindows.dll\")
			foreach(_plugin \${_platform_plugins})
				file(COPY \"\${_plugin}\" DESTINATION \"${WINDOWSSTAGING}/plugins/platforms\")
			endforeach()

			# Copy style plugins
			file(GLOB _style_plugins \"${QT_INSTALL_PREFIX}/plugins/styles/*.dll\")
			foreach(_plugin \${_style_plugins})
				get_filename_component(_name \"\${_plugin}\" NAME)
				if(NOT _name MATCHES \"d\\\\.dll$\")
					file(COPY \"\${_plugin}\" DESTINATION \"${WINDOWSSTAGING}/plugins/styles\")
				endif()
			endforeach()

			# Copy imageformat plugins (skip debug versions ending in 'd.dll')
			file(GLOB _imgformat_plugins \"${QT_INSTALL_PREFIX}/plugins/imageformats/*.dll\")
			foreach(_plugin \${_imgformat_plugins})
				get_filename_component(_name \"\${_plugin}\" NAME)
				if(NOT _name MATCHES \"d\\\\.dll$\")
					file(COPY \"\${_plugin}\" DESTINATION \"${WINDOWSSTAGING}/plugins/imageformats\")
				endif()
			endforeach()

			# Copy iconengines plugins
			file(GLOB _iconengine_plugins \"${QT_INSTALL_PREFIX}/plugins/iconengines/*.dll\")
			foreach(_plugin \${_iconengine_plugins})
				get_filename_component(_name \"\${_plugin}\" NAME)
				if(NOT _name MATCHES \"d\\\\.dll$\")
					file(COPY \"\${_plugin}\" DESTINATION \"${WINDOWSSTAGING}/plugins/iconengines\")
				endif()
			endforeach()

			# Copy TLS plugins (for HTTPS support)
			file(GLOB _tls_plugins \"${QT_INSTALL_PREFIX}/plugins/tls/*.dll\")
			foreach(_plugin \${_tls_plugins})
				get_filename_component(_name \"\${_plugin}\" NAME)
				if(NOT _name MATCHES \"d\\\\.dll$\" AND NOT _name MATCHES \"openssl\")
					file(COPY \"\${_plugin}\" DESTINATION \"${WINDOWSSTAGING}/plugins/tls\")
				endif()
			endforeach()

			# Copy networkinformation plugins
			file(GLOB _netinfo_plugins \"${QT_INSTALL_PREFIX}/plugins/networkinformation/*.dll\")
			foreach(_plugin \${_netinfo_plugins})
				get_filename_component(_name \"\${_plugin}\" NAME)
				if(NOT _name MATCHES \"d\\\\.dll$\")
					file(COPY \"\${_plugin}\" DESTINATION \"${WINDOWSSTAGING}/plugins/networkinformation\")
				endif()
			endforeach()

			# Copy geoservices plugins
			file(GLOB _geoservices_plugins \"${QT_INSTALL_PREFIX}/plugins/geoservices/*.dll\")
			foreach(_plugin \${_geoservices_plugins})
				get_filename_component(_name \"\${_plugin}\" NAME)
				if(NOT _name MATCHES \"d\\\\.dll$\")
					file(COPY \"\${_plugin}\" DESTINATION \"${WINDOWSSTAGING}/plugins/geoservices\")
				endif()
			endforeach()

			# Copy position plugins
			file(GLOB _position_plugins \"${QT_INSTALL_PREFIX}/plugins/position/*.dll\")
			foreach(_plugin \${_position_plugins})
				get_filename_component(_name \"\${_plugin}\" NAME)
				if(NOT _name MATCHES \"d\\\\.dll$\")
					file(COPY \"\${_plugin}\" DESTINATION \"${WINDOWSSTAGING}/plugins/position\")
				endif()
			endforeach()

			# Copy generic plugins
			file(GLOB _generic_plugins \"${QT_INSTALL_PREFIX}/plugins/generic/*.dll\")
			foreach(_plugin \${_generic_plugins})
				get_filename_component(_name \"\${_plugin}\" NAME)
				if(NOT _name MATCHES \"d\\\\.dll$\")
					file(COPY \"\${_plugin}\" DESTINATION \"${WINDOWSSTAGING}/plugins/generic\")
				endif()
			endforeach()
		")

		# Copy vcpkg DLLs from the bin directory
		install(CODE "
			message(STATUS \"Copying vcpkg DLLs from ${_VCPKG_PREFIX}/bin...\")
			file(GLOB _vcpkg_dlls \"${_VCPKG_PREFIX}/bin/*.dll\")
			foreach(_dll \${_vcpkg_dlls})
				file(COPY \"\${_dll}\" DESTINATION \"${WINDOWSSTAGING}\")
			endforeach()
		")

		# Copy libdivecomputer DLL - derive bin dir from library path
		if(LIBDIVECOMPUTER_LIBRARIES)
			get_filename_component(_LIBDC_LIB_DIR "${LIBDIVECOMPUTER_LIBRARIES}" DIRECTORY)
			get_filename_component(_LIBDC_PREFIX "${_LIBDC_LIB_DIR}" DIRECTORY)
			install(CODE "
				message(STATUS \"Copying libdivecomputer DLLs from ${_LIBDC_PREFIX}/bin...\")
				file(GLOB _libdc_dlls \"${_LIBDC_PREFIX}/bin/*.dll\")
				foreach(_dll \${_libdc_dlls})
					file(COPY \"\${_dll}\" DESTINATION \"${WINDOWSSTAGING}\")
				endforeach()
			")
		endif()

		# Copy MSVC runtime DLLs
		set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
		include(InstallRequiredSystemLibraries)
		if(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
			install(FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION ${WINDOWSSTAGING})
		endif()
	else()
		# For MinGW/MXE builds, use the existing dlllist.cmake script
		install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} -DSUBSURFACE_TARGET=${SUBSURFACE_TARGET} -DSUBSURFACE_SOURCE=${SUBSURFACE_SOURCE} -DSTAGING=${WINDOWSSTAGING} -P ${CMAKE_SOURCE_DIR}/cmake/Modules/dlllist.cmake)")
	endif()

	# create the subsurface-x.y.z.exe installer - this needs to depend
	# on the install target but cmake doesn't allow that, so we depend
	# on the fake target instead
	add_custom_target(
		fake_install
		COMMAND "${CMAKE_COMMAND}" --build . --target install
		DEPENDS ${SUBSURFACE_TARGET}
	)

	add_custom_target(
		installer
		COMMAND ${MAKENSIS} ${WINDOWSSTAGING}/subsurface.nsi
		DEPENDS fake_install
	)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	# the syntax is odd, but this searches for libqtgeoservices_googlemaps.so
	# in the filesystem below our install-root
	# different Linux flavors put the plugin in different directories
	file(GLOB_RECURSE GOOGLEMAPS ${CMAKE_SOURCE_DIR}/../install-root/libqtgeoservices_googlemaps.so)
	if (NOT GOOGLEMAPS)
		message(STATUS "Cannot find libqtgeoservices_googlemaps.so")
	else()
		add_custom_target(link_googlemaps_plugin ALL COMMAND
			rm -rf ${CMAKE_BINARY_DIR}/geoservices &&
			mkdir -p ${CMAKE_BINARY_DIR}/geoservices &&
			ln -sf ${GOOGLEMAPS} ${CMAKE_BINARY_DIR}/geoservices)
	endif()
	install(FILES subsurface.debug DESTINATION bin)
	install(FILES subsurface.desktop DESTINATION share/applications)
	install(CODE "execute_process(COMMAND sh ${CMAKE_SOURCE_DIR}/scripts/add-version-to-metainfo.sh WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})")
	install(FILES metainfo/subsurface.metainfo.xml DESTINATION share/metainfo)
	install(FILES icons/subsurface-icon.svg DESTINATION share/icons/hicolor/scalable/apps)
	if(BUILD_DOCS OR INSTALL_DOCS)
		install(DIRECTORY Documentation/images DESTINATION share/subsurface/Documentation)
		install(FILES ${DOCFILES} DESTINATION share/subsurface/Documentation)
	endif()
	install(DIRECTORY theme DESTINATION share/subsurface)
	install(DIRECTORY printing_templates DESTINATION share/subsurface)
	install(FILES ${TRANSLATIONS} DESTINATION share/subsurface/translations)
	install(TARGETS ${SUBSURFACE_TARGET} DESTINATION bin)
endif()

if (BUILD_TESTS)
	enable_testing()
	add_subdirectory(tests)
endif()

# useful for debugging CMake issues
# print_all_variables()
