# Modified from Subsurface's CMakeLists.txt

cmake_minimum_required(VERSION 3.1)
project(smtk2ssrf)

option(COMMANDLINE "Build command line version" OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH FALSE)
set(SSRF_PATH ../)
set(CMAKE_MODULE_PATH ${SSRF_PATH}cmake/Modules)
include(${CMAKE_MODULE_PATH}/MacroOutOfSourceBuild.cmake)
include(${CMAKE_MODULE_PATH}/cmake_variables_helper.cmake)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	include(${CMAKE_MODULE_PATH}/HandleVersionGeneration.cmake)
endif()

MACRO_ENSURE_OUT_OF_SOURCE_BUILD(
    "We don't support building in source, please create a build folder elsewhere and remember to run git clean -xdf to remove temporary files created by CMake."
)

# Find needed packages and Qt5 components. Most of them needed by Subsurface
find_package(PkgConfig)
include(${SSRF_PATH}cmake/Modules/pkgconfig_helper.cmake)
pkg_config_library(LIBXML libxml-2.0 REQUIRED)
pkg_config_library(LIBSQLITE3 sqlite3 REQUIRED)
pkg_config_library(LIBXSLT libxslt REQUIRED)
pkg_config_library(LIBZIP libzip REQUIRED)
pkg_config_library(LIBUSB libusb-1.0 QUIET)
pkg_config_library(GLIB2 glib-2.0 REQUIRED)
pkg_config_library(LIBGIT2 libgit2 REQUIRED)
pkg_config_library(LIBMDB libmdb REQUIRED)
pkg_config_library(LIBDC libdivecomputer REQUIRED)
pkg_config_library(LIBFTDI libftdi1 QUIET)

find_package(Qt5 REQUIRED COMPONENTS Core
				     Concurrent
				     Widgets
				     Network
				     Svg
				     Test
				     LinguistTools
				     Positioning
				     Quick
				     Bluetooth)

set(QT_LIBRARIES Qt5::Core
		 Qt5::Concurrent
		 Qt5::Widgets
		 Qt5::Network
		 Qt5::Svg
		 Qt5::Positioning
		 Qt5::Quick
		 Qt5::Bluetooth)

set(QT5_INCLUDE_PATH ${Qt5_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS} ${Qt5Core_INCLUDE_DIRS})
qt5_add_resources(SUBSURFACE_RESOURCES ${SSRF_PATH}subsurface.qrc)

include_directories(${SSRF_PATH} ${QT5_INCLUDE_PATH} ${LIBMDB_INCLUDEDIR} ${LIBDIVECOMPUTER_INCLUDEDIR})

# Check for libsubsurface_corelib.a
# A previous build of subsurface is needed (windows and linux)
message(STATUS "Checking for Subsurface core library")
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	find_library(SSRF_CORELIB
		NAMES subsurface_corelib
		PATHS ${CMAKE_BINARY_DIR}/../subsurface/core
		)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	find_library(SSRF_CORELIB
		NAMES subsurface_corelib
		PATHS ${SSRF_PATH}build/core
		)
endif()

if(NOT SSRF_CORELIB)
	message(FATAL_ERROR "  libsubsurface_corelib.a not found. Did you build Subsurface previously?")
else()
	message(STATUS "  Found ${SSRF_CORELIB}")
endif()

# Set compiler flags and definitions
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(SMTK_LINK_LIBRARIES ${SMTK_LINK_LIBRARIES} -lssh2 -lz -lpthread)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	remove_definitions(-DUNICODE)
        add_definitions(-mwindows -D_WIN32)
endif()

set(SMTK_LINK_LIBRARIES ${SMTK_LINK_LIBRARIES} ${LIBMDB_LIBRARIES} ${LIBDIVECOMPUTER_LIBRARIES})

set(SMTK_IMPORT_SRCS
	smartrak.c
	smrtk2ssrfc_window.ui
	smrtk2ssrfc_window.h
	smrtk2ssrfc_window.cpp
)
set(SMTK_LINK_LIBRARIES ${SMTK_LINK_LIBRARIES} ${SUBSURFACE_LINK_LIBRARIES} ${SSRF_CORELIB} ${QT_LIBRARIES})
source_group("SmartTrak Import libs" FILES ${SMTK_IMPORT_SRCS})
set(SMTK_IMPORT_TARGET smtk2ssrf)
add_library(smtk_import STATIC ${SMTK_IMPORT_SRCS})


if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# Setting the link target this way resolves an issue with not importing timegettime correctly fron winmm
	target_link_libraries(smtk_import PRIVATE wsock32 ws2_32 ole32 imm32 winmm)
endif()

add_executable(${SMTK_IMPORT_TARGET} smtk_standalone.cpp ${SUBSURFACE_RESOURCES})

# We just want CLI mode on Linux. Silently drop it if cross building to Windows.
if (COMMANDLINE AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
	message(WARNING "Building Command Line mode.")
	target_compile_definitions(${SMTK_IMPORT_TARGET} PRIVATE COMMANDLINE=1)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	file(WRITE ${CMAKE_BINARY_DIR}/qt.conf "[Paths]
Prefix=.
")
endif()

target_link_libraries(${SMTK_IMPORT_TARGET} smtk_import ${SMTK_LINK_LIBRARIES})

# Install instructions
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	install(TARGETS ${SMTK_IMPORT_TARGET} DESTINATION bin)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set(WINDOWSSTAGING ${CMAKE_BINARY_DIR}/staging)
	install(FILES ${SSRF_PATH}gpl-2.0.txt ${SSRF_PATH}packaging/windows/subsurface.ico DESTINATION ${WINDOWSSTAGING})
	install(TARGETS ${SMTK_IMPORT_TARGET} DESTINATION ${WINDOWSSTAGING})
	install(FILES ${CMAKE_BINARY_DIR}/qt.conf DESTINATION ${WINDOWSSTAGING})
	if(NOT DEFINED MAKENSIS)
		set(MAKENSIS makensis)
	endif()

	# this ensures that smtk2ssrf.exe has been built before this is run
	install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} -DSMTK_IMPORT_TARGET=${SMTK_IMPORT_TARGET} -DSMTK_IMPORT_SRCS=${SMTK_IMPORT_SRCS} -DSTAGING=${WINDOWSSTAGING} -P ${CMAKE_SOURCE_DIR}/cmake/Modules/dlllist.cmake)")

	# create the smtk2ssrf-x.y.z.exe installer - this needs to depend
	# on the install target but cmake doesn't allow that, so we depend
	# on the fake target instead
	add_custom_target(
		fake_install
		COMMAND "${CMAKE_COMMAND}" --build . --target install
		DEPENDS ${SMTK_IMPORT_TARGET}
	)

	add_custom_target(
		installer
		COMMAND ${MAKENSIS} ${WINDOWSSTAGING}/smtk-import.nsi
		DEPENDS fake_install
	)
endif()

# useful for debugging CMake issues
# printll_variables()
