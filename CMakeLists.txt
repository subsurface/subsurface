# cmake based build of Subsurface

project(Subsurface)
cmake_minimum_required(VERSION 2.8.11)


# global settings

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
option(LIBGIT2_FROM_PKGCONFIG "use pkg-config to retrieve libgit2" OFF)
option(LIBDC_FROM_PKGCONFIG "use pkg-config to retrieve libdivecomputer" OFF)
option(LIBGRANTLEE_FROM_PKGCONFIG "use pkg-config to retrieve grantlee" OFF)
option(LIBMARBLE_FROM_PKGCONFIG "use pkg-config to retrieve marble" OFF)
option(MAKE_TESTS "Make the tests" ON)
option(NO_MARBLE "disable the marble widget" OFF)
option(NO_DOCS "disable the docs" OFF)
option(NO_PRINTING "disable the printing support" OFF)
option(NO_USERMANUAL "don't include a viewer for the user manual" OFF)
option(USE_LIBGIT23_API "allow building with libgit2 master" OFF)
option(FORCE_LIBSSH "force linking with libssh to workaround libgit2 bug" ON)
option(SUBSURFACE_MOBILE "build the QtQuick version for mobile device" OFF)
option(FBSUPPORT "allow posting to Facebook" ON)
option(BTSUPPORT "enable support for QtBluetooth (requires Qt5.4 or newer)" ON)
option(FTDISUPPORT "enable support for libftdi based serial" OFF)

add_definitions(-DSUBSURFACE_SOURCE="${CMAKE_SOURCE_DIR}")

set(CMAKE_MODULE_PATH
	${CMAKE_MODULE_PATH}
	${${PROJECT_NAME}_SOURCE_DIR}/cmake/Modules
)

include_directories(.
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_BINARY_DIR}
	${CMAKE_BINARY_DIR}/desktop-widgets
	desktop-widgets/
	qt-models
	desktop-widgets/profile
	subsurface-core/
)

# get the version string -- this is only used for Mac Bundle at this point
# the other version data gets updated when running make - this is set when running cmake
execute_process(
	COMMAND sh scripts/get-version linux
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	OUTPUT_VARIABLE SSRF_VERSION_STRING
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

message(STATUS "Creating build files for Subsurface ${SSRF_VERSION_STRING}")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 ")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 ")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  # using Intel C++
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  # using Visual Studio C++
endif()

# pkgconfig for required libraries
find_package(PkgConfig)
include(cmake/Modules/pkgconfig_helper.cmake)

pkg_config_library(LIBXML libxml-2.0 REQUIRED)
pkg_config_library(LIBSQLITE3 sqlite3 REQUIRED)
pkg_config_library(LIBXSLT libxslt REQUIRED)
pkg_config_library(LIBZIP libzip REQUIRED)
pkg_config_library(LIBUSB libusb-1.0 QUIET)

# more libraries with special handling in case we build them ourselves
if(LIBGIT2_FROM_PKGCONFIG)
	pkg_config_library(LIBGIT2 libgit2 REQUIRED)
	set(LIBGIT2_LIBRARIES "")
	if(USE_LIBGIT23_API)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_LIBGIT23_API")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_LIBGIT23_API")
		if(ANDROID)
			# for Android we need to force a static link against ssl and crypto
			# this is a bit hacky, but it seems to work
			set(LIBGIT2_LIBRARIES ${LIBGIT2_LIBRARIES} ${LIBGIT2_LIBRARY_DIRS}/libssl.a ${LIBGIT2_LIBRARY_DIRS}/libcrypto.a)
		endif()
		if(FORCE_LIBSSH)
			pkg_config_library(LIBSSH2 libssh2 REQUIRED)
			set(LIBGIT2_LIBRARIES ${LIBGIT2_LIBRARIES} ${LIBSSH2_LIBRARIES})
		endif()
	endif()
else()
	find_package(LIBGIT2 REQUIRED)
	include_directories(${LIBGIT2_INCLUDE_DIR})
	if(USE_LIBGIT23_API)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_LIBGIT23_API")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_LIBGIT23_API")
		if(FORCE_LIBSSH)
			find_package(Libssh2 QUIET CONFIG)
			if ("${LIBSSH2_VERSION}" STRGREATER "1.6.1")
				set(LIBSSH2_LIBRARIES Libssh2::libssh2)
			endif()
			if(!LIBSSH2_FOUND OR "${LIBSSH2_FOUND}" STREQUAL "")
				pkg_config_library(LIBSSH2 libssh2 REQUIRED)
			endif()
		endif()
		find_package(libcurl QUIET)
		if(!LIBCURL_FOUND OR "${LIBCURL_FOUND}" STREQUAL "")
			pkg_config_library(LIBCURL libcurl REQUIRED)
		endif()
		set(LIBGIT2_LIBRARIES ${LIBGIT2_LIBRARIES} -L${LIBSSH2_LIBRARY_DIRS} ${LIBSSH2_LIBRARIES} ${LIBCURL_LIBRARIES})
	endif()
endif()

if(LIBDC_FROM_PKGCONFIG)
	pkg_config_library(LIBDC libdivecomputer REQUIRED)
	set(LIBDIVECOMPUTER_LIBRARIES "")
else()
	find_package(Libdivecomputer REQUIRED)
	include_directories(${LIBDIVECOMPUTER_INCLUDE_DIR})
endif()

# setup marble

if(NOT NO_MARBLE)
	if(LIBMARBLE_FROM_PKGCONFIG)
		pkg_config_library(MARBLE libmarble REQUIRED)
		set(MARBLE_LIBRARIES "")
	else()
		find_package(MARBLE QUIET)
		if(MARBLE_FOUND)
			include_directories(${MARBLE_INCLUDE_DIR})
		else()
			set(NO_MARBLE ON)
		endif()
	endif()
else()
	message(STATUS "building without marble widget support")
	add_definitions(-DNO_MARBLE)
	set(MARBLE_LIBRARIES "")
endif()

if(FTDISUPPORT)
	message(STATUS "building with libftdi support")
	pkg_config_library(LIBFTDI libftdi QUIET)
	if (NOT LIBFTDI_FOUND)
		pkg_config_library(LIBFTDI libftdi1 REQUIRED)
	endif()
	add_definitions(-DSERIAL_FTDI)
endif()

if(ANDROID)
	set(FBSUPPORT OFF)
	set(NO_PRINTING ON)
endif()

# setup Grantlee

if(NO_PRINTING)
	message(STATUS "building without printing support")
	add_definitions(-DNO_PRINTING)
	set(GRANTLEE_LIBRARIES "")
else()
	if(LIBGRANTLEE_FROM_PKGCONFIG)
		pkg_config_library(GRANTLEE libgrantlee REQUIRED)
		set(GRANTLEE_LIBRARIES "")
	else()
		find_package(Grantlee5 REQUIRED)
		set(GRANTLEE_LIBRARIES Grantlee5::Templates)
	endif()
	set(PRINTING_PKG PrintSupport)
	set(PRINTING_LIB Qt5::PrintSupport)
endif()

if(NO_USERMANUAL)
	message(STATUS "building without usermanual")
	add_definitions(-DNO_USERMANUAL)
else()
	set(WEBKIT_PKG WebKitWidgets)
	set(WEBKIT_LIB Qt5::WebKitWidgets)
endif()

set(SUBSURFACE_LINK_LIBRARIES ${SUBSURFACE_LINK_LIBRARIES} ${LIBDIVECOMPUTER_LIBRARIES} ${LIBGIT2_LIBRARIES} ${LIBUSB_LIBRARIES})

# handle out of tree build correctly
string(COMPARE EQUAL "${${PROJECT_NAME}_SOURCE_DIR}" "${${PROJECT_NAME}_BINARY_DIR}" insource)
if (insource)
	message(STATUS "building in Subsurface source tree - we recommend out of tree builds")
else()
	message(STATUS "out of source build from source in ${${PROJECT_NAME}_SOURCE_DIR}")
endif()
get_filename_component(PARENTDIR ${${PROJECT_NAME}_SOURCE_DIR} PATH)
string(COMPARE EQUAL "${${PROJECT_NAME}_SOURCE_DIR}" "${PARENTDIR}" insourcesubdir)
if(NOT (insource OR insourcedir))
	add_custom_target(link_marble_data ALL COMMAND rm -rf ./marbledata && ln -s ${CMAKE_SOURCE_DIR}/marbledata ${CMAKE_BINARY_DIR}/marbledata)
endif()

# configure Qt.
if(SUBSURFACE_MOBILE)
	set(QT_QUICK_PKG Quick)
	set(QT_QUICK_LIB Qt5::Quick)
endif()
if(ANDROID)
	set(ANDROID_PKG AndroidExtras)
	set(ANDROID_LIB Qt5::AndroidExtras)
endif()
if(BTSUPPORT)
	set(BLUETOOTH_PKG Bluetooth)
	set(BLUETOOTH_LIB Qt5::Bluetooth)
endif()
find_package(Qt5 REQUIRED COMPONENTS Core Concurrent Widgets Network ${WEBKIT_PKG} ${PRINTING_PKG} Svg Test LinguistTools ${QT_QUICK_PKG} ${ANDROID_PKG} ${BLUETOOTH_PKG})
set(QT_LIBRARIES Qt5::Core Qt5::Concurrent Qt5::Widgets Qt5::Network ${WEBKIT_LIB} ${PRINTING_LIB} Qt5::Svg ${QT_QUICK_LIB} ${ANDROID_LIB} ${BLUETOOTH_LIB})
set(QT_TEST_LIBRARIES ${QT_LIBRARIES} Qt5::Test)

if (BTSUPPORT AND "${Qt5Core_VERSION_STRING}" STRLESS "5.4.0")
	set(BTSUPPORT OFF)
	message(STATUS "Turning off Bluetooth support as Qt version ${Qt5Core_VERSION_STRING} is insufficient for that")
	list(REMOVE_ITEM QT_LIBRARIES Qt5::Bluetooth)
endif()

# Generate the ssrf-config.h every 'make'
file(WRITE ${CMAKE_BINARY_DIR}/version.h.in
"#define VERSION_STRING \"@VERSION_STRING@\"
#define GIT_VERSION_STRING \"@GIT_VERSION_STRING@\"
#define CANONICAL_VERSION_STRING \"@CANONICAL_VERSION_STRING@\"
")

file(WRITE ${CMAKE_BINARY_DIR}/version.cmake "
	if(\${APPLE})
		set(VER_OS darwin)
	elseif(\${WIN32})
		set(VER_OS win)
	else()
		set(VER_OS linux)
	endif()
	if(CMAKE_SYSTEM_NAME STREQUAL \"Windows\")
		set(VER_OS win)
	endif()
	execute_process(
		COMMAND sh scripts/get-version \${VER_OS}
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		OUTPUT_VARIABLE VERSION_STRING
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	execute_process(
		COMMAND sh scripts/get-version linux
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		OUTPUT_VARIABLE GIT_VERSION_STRING
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	execute_process(
		COMMAND sh scripts/get-version full
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		OUTPUT_VARIABLE CANONICAL_VERSION_STRING
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	configure_file(\${SRC} \${DST} @ONLY)
	if(CMAKE_SYSTEM_NAME STREQUAL \"Windows\")
		execute_process(
			COMMAND cat ${CMAKE_SOURCE_DIR}/packaging/windows/subsurface.nsi.in
			COMMAND sed -e \"s/VERSIONTOKEN/\${GIT_VERSION_STRING}/\"
			COMMAND sed -e \"s/PRODVTOKEN/\${CANONICAL_VERSION_STRING}/\"
			OUTPUT_FILE ${CMAKE_BINARY_DIR}/staging/subsurface.nsi
		)
	endif()
")

add_custom_target(
	version ALL COMMAND ${CMAKE_COMMAND} ${CMAKE_COMMAND}
	-D SRC=${CMAKE_BINARY_DIR}/version.h.in
	-D DST=${CMAKE_BINARY_DIR}/ssrf-version.h
	-D CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}
	-P ${CMAKE_BINARY_DIR}/version.cmake
)

# set up the different target platforms
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(SUBSURFACE_TARGET subsurface)
	# in some builds we appear to be missing libz for some strange reason...
	set(SUBSURFACE_LINK_LIBRARIES ${SUBSURFACE_LINK_LIBRARIES} -lz)
endif()
if(ANDROID)
	set(SUBSURFACE_TARGET subsurface)
	# To allow us to debug log to logcat
	set(SUBSURFACE_LINK_LIBRARIES ${SUBSURFACE_LINK_LIBRARIES} -llog)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	set(SUBSURFACE_TARGET Subsurface)
	find_library(APP_SERVICES_LIBRARY ApplicationServices)
	find_library(HID_LIB HidApi)
	set(SUBSURFACE_LINK_LIBRARIES ${SUBSURFACE_LINK_LIBRARIES} ${HID_LIB})
	set(EXTRA_LIBS ${APP_SERVICES_LIBRARY})
	set(ICON_FILE ${CMAKE_SOURCE_DIR}/packaging/macosx/Subsurface.icns)
	set(MACOSX_BUNDLE_INFO_STRING "Subsurface")
	set(MACOSX_BUNDLE_ICON_FILE Subsurface.icns)
	set(MACOSX_BUNDLE_GUI_IDENTIFIER "org.subsurface-divelog")
	set(MACOSX_BUNDLE_BUNDLE_NAME "Subsurface")
	set(MACOSX_BUNDLE_BUNDLE_VERSION "${SSRF_VERSION_STRING}")
	set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${SSRF_VERSION_STRING}")
	set(MACOSX_BUNDLE_LONG_VERSION_STRING "${SSRF_VERSION_STRING}")
	set(MACOSX_BUNDLE_COPYRIGHT "Linus Torvalds, Dirk Hohndel, Tomaz Canabrava, and others")
	set_source_files_properties(${ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
	set(SUBSURFACE_PKG MACOSX_BUNDLE ${ICON_FILE})
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set(SUBSURFACE_TARGET subsurface)
	set(SUBSURFACE_LINK_LIBRARIES ${SUBSURFACE_LINK_LIBRARIES} -lwsock32 -lws2_32)
	remove_definitions(-DUNICODE)
	add_definitions(-mwindows -D_WIN32)
endif()

qt5_add_resources(SUBSURFACE_RESOURCES subsurface.qrc)

# include translations
add_subdirectory(translations)
add_subdirectory(subsurface-core)
add_subdirectory(qt-models)
add_subdirectory(profile-widget)

if (NOT SUBSURFACE_MOBILE)
	add_subdirectory(desktop-widgets)
endif()

if(FBSUPPORT)
	add_definitions(-DFBSUPPORT)
endif()

if(BTSUPPORT)
	add_definitions(-DBT_SUPPORT)
endif()


# add pthread to the end of the library list on Linux
# this is only needed on Ubuntu (why do these idiots break everything?)
# but shouldn't hurt on other Linux versions
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT ANDROID)
	set(SUBSURFACE_LINK_LIBRARIES ${SUBSURFACE_LINK_LIBRARIES} -lpthread)
endif()

# create the executables
if(SUBSURFACE_MOBILE)
	set(SUBSURFACE_TARGET subsurface-mobile)
	set(MOBILE_SRC
		qt-mobile/qmlmanager.cpp
		qt-mobile/qmlprofile.cpp
		qt-models/divelistmodel.cpp
		subsurface-mobile-main.cpp
		subsurface-mobile-helper.cpp
	)
	add_definitions(-DSUBSURFACE_MOBILE)
	qt5_add_resources(MOBILE_RESOURCES qt-mobile/qml/mobile-resources.qrc)
	if(ANDROID)
		add_library(subsurface-mobile SHARED ${MOBILE_SRC} ${SUBSURFACE_PKG} ${SUBSURFACE_RESOURCES} ${MOBILE_RESOURCES})
	else()
		add_executable(subsurface-mobile ${MOBILE_SRC} ${SUBSURFACE_PKG} ${SUBSURFACE_RESOURCES} ${MOBILE_RESOURCES})
	endif()
	target_link_libraries(
		${SUBSURFACE_TARGET}
		subsurface_profile
		subsurface_models
		subsurface_corelib
		${SUBSURFACE_LINK_LIBRARIES})
else()
# the main app.
	set(SUBSURFACE_APP
		subsurface-desktop-main.cpp
		subsurface-desktop-helper.cpp
	)
	source_group("Subsurface App" FILES ${SUBSURFACE_APP})

	if(ANDROID)
		# Produce a shared-library instead of a program.
		# Something that androiddeployqt can work with.
		# this is the desktop version, running on android.
		add_library(${SUBSURFACE_TARGET} SHARED ${SUBSURFACE_PKG} ${SUBSURFACE_APP} ${SUBSURFACE_RESOURCES})
	else()
		add_executable(${SUBSURFACE_TARGET} MACOSX_BUNDLE WIN32 ${SUBSURFACE_PKG} ${SUBSURFACE_APP} ${SUBSURFACE_RESOURCES})
	endif()

	target_link_libraries(
		${SUBSURFACE_TARGET}
		subsurface_generated_ui
		subsurface_interface
		subsurface_profile
		subsurface_statistics
		subsurface_models
		subsurface_corelib
		${SUBSURFACE_LINK_LIBRARIES}
	)
	add_dependencies(subsurface_statistics subsurface_generated_ui)
	add_dependencies(subsurface_interface subsurface_generated_ui)
	add_dependencies(subsurface_profile subsurface_generated_ui)
	add_dependencies(subsurface_generated_ui version)
endif()

add_dependencies(subsurface_corelib version)

# add platform specific actions
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	add_custom_command(
		OUTPUT ${CMAKE_BINARY_DIR}/qt.conf
		COMMAND echo \"[Paths]\" > ${CMAKE_BINARY_DIR}/qt.conf \; echo \"Prefix=.\" >> ${CMAKE_BINARY_DIR}/qt.conf
	)
	add_custom_target(
		generate_qtconf
		DEPENDS ${CMAKE_BINARY_DIR}/qt.conf
	)
	add_dependencies(${SUBSURFACE_TARGET} generate_qtconf)
endif()

# build an automated html exporter
add_executable(export-html EXCLUDE_FROM_ALL export-html.cpp ${SUBSURFACE_RESOURCES})
target_link_libraries(export-html subsurface_corelib ${SUBSURFACE_LINK_LIBRARIES})

# install a few things so that one can run Subsurface from the build
# directory
if(NOT insource)
	add_custom_target(themeLink ALL
		COMMAND
		rm -f ${CMAKE_BINARY_DIR}/theme &&
		ln -s ${CMAKE_SOURCE_DIR}/theme ${CMAKE_BINARY_DIR}/theme
	)
	if(NOT NO_PRINTING)
		add_custom_target(printing_templatesLink ALL
			COMMAND
			rm -f ${CMAKE_BINARY_DIR}/printing_templates &&
			ln -s ${CMAKE_SOURCE_DIR}/printing_templates ${CMAKE_BINARY_DIR}/printing_templates
		)
	endif()
	if(NOT NO_DOCS)
		add_custom_target(
			documentationLink ALL
			COMMAND
			mkdir -p ${CMAKE_BINARY_DIR}/Documentation/ &&
			rm -rf ${CMAKE_BINARY_DIR}/Documentation/images &&
			ln -s ${CMAKE_SOURCE_DIR}/Documentation/images ${CMAKE_BINARY_DIR}/Documentation/images
		)
	endif()
else()
	if(NOT NO_DOCS)
		add_custom_target(
			documentationLink ALL
		)
	endif()
endif()
if(NOT NO_DOCS)
	add_custom_target(
		documentation ALL
		COMMAND
		${CMAKE_MAKE_PROGRAM} -C ${CMAKE_SOURCE_DIR}/Documentation OUT=${CMAKE_BINARY_DIR}/Documentation/ doc
		DEPENDS documentationLink
	)
endif()

# install Subsurface
# first some variables with files that need installing
set(DOCFILES
	README
	ReleaseNotes/ReleaseNotes.txt
	SupportedDivecomputers.txt
	${CMAKE_BINARY_DIR}/Documentation/user-manual.html
	${CMAKE_BINARY_DIR}/Documentation/user-manual_es.html
	${CMAKE_BINARY_DIR}/Documentation/user-manual_fr.html
	${CMAKE_BINARY_DIR}/Documentation/user-manual_ru.html
)

set(QTTRANSLATIONS_BASE
	qt_da.qm
	qt_de.qm
	qt_es.qm
	qt_fr.qm
	qt_he.qm
	qt_hu.qm
	qt_pl.qm
	qt_pt.qm
	qt_ru.qm
	qt_sk.qm
	qt_sv.qm
	qt_zh_TW.qm
)

if(NOT DEFINED QT_TRANSLATION_DIR OR QT_TRANSLATION_DIR STREQUAL "")
	set(QT_TRANSLATION_DIR ${Qt5Core_DIR}/../../../translations)
endif()
set(QTTRANSLATIONS "")
foreach(QTTRANSLATION ${QTTRANSLATIONS_BASE})
	if(NOT ${QTTRANSLATION} STREQUAL "")
		set(QTTRANSLATIONS ${QTTRANSLATIONS} ${QT_TRANSLATION_DIR}/${QTTRANSLATION})
	endif()
endforeach()

# now for each platform the install instructions

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	set(RESOURCEDIR ${CMAKE_BINARY_DIR}/Subsurface.app/Contents/Resources)
	set(PLUGINDIR ${CMAKE_BINARY_DIR}/Subsurface.app/Contents/PlugIns)
	install(DIRECTORY marbledata/maps DESTINATION ${RESOURCEDIR}/data)
	install(DIRECTORY marbledata/bitmaps DESTINATION ${RESOURCEDIR}/data)
	install(DIRECTORY Documentation/images DESTINATION ${RESOURCEDIR}/share/Documentation)
	install(FILES ${DOCFILES} DESTINATION ${RESOURCEDIR}/share/Documentation)
	install(DIRECTORY theme DESTINATION ${RESOURCEDIR})
	install(DIRECTORY printing_templates DESTINATION ${RESOURCEDIR})
	install(FILES ${TRANSLATIONS} DESTINATION ${RESOURCEDIR}/translations)
	install(FILES ${QTTRANSLATIONS} DESTINATION ${RESOURCEDIR}/translations)
	install(FILES ${CMAKE_SOURCE_DIR}/gpl-2.0.txt DESTINATION ${RESOURCEDIR})
	# this is a HACK
	install(DIRECTORY ${Grantlee5_DIR}/../../grantlee DESTINATION ${PLUGINDIR})
	# this is a hack - but I don't know how else to find the macdeployqt program if it's not in the PATH
	string(REPLACE moc macdeployqt MACDEPLOYQT ${QT_MOC_EXECUTABLE})
	install(CODE "execute_process(COMMAND ${MACDEPLOYQT} Subsurface.app)")
	install(CODE "message(STATUS \"two ERRORS here about libmysqlclient and libpq not found are harmless\")")

endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# Windows bundling rules
	# We don't have a helpful tool like macdeployqt for Windows, so we hardcode
	# which libs we need.
	# "make install", copies everything into a staging area
	# "make installer", uses makensis to create an installer executable
	set(WINDOWSSTAGING ${CMAKE_BINARY_DIR}/staging)
	install(DIRECTORY marbledata/maps DESTINATION ${WINDOWSSTAGING}/data)
	install(DIRECTORY marbledata/bitmaps DESTINATION ${WINDOWSSTAGING}/data)
	install(DIRECTORY Documentation/images DESTINATION ${WINDOWSSTAGING}/Documentation)
	install(FILES ${DOCFILES} DESTINATION ${WINDOWSSTAGING}/Documentation)
	install(DIRECTORY theme DESTINATION ${WINDOWSSTAGING})
	install(DIRECTORY printing_templates DESTINATION ${WINDOWSSTAGING})
	install(FILES ${TRANSLATIONS} DESTINATION ${WINDOWSSTAGING}/translations)
	install(FILES ${QTTRANSLATIONS} DESTINATION ${WINDOWSSTAGING}/translations)
	install(FILES ${CMAKE_SOURCE_DIR}/gpl-2.0.txt ${CMAKE_SOURCE_DIR}/packaging/windows/subsurface.ico DESTINATION ${WINDOWSSTAGING})
	install(TARGETS ${SUBSURFACE_TARGET} DESTINATION ${WINDOWSSTAGING})
	install(FILES ${CMAKE_BINARY_DIR}/qt.conf DESTINATION ${WINDOWSSTAGING})
	install(DIRECTORY ${CMAKE_INSTALL_PREFIX}/lib/grantlee DESTINATION ${WINDOWSSTAGING})
	if(NOT DEFINED MAKENSIS)
		set(MAKENSIS makensis)
	endif()

	# next figure out the DLLs we need to include in the installer
	# since this needs to run at install time we create a new cmake
	# script that then gets executed at install time with install(CODE...)
	file(WRITE ${CMAKE_BINARY_DIR}/dlllist.cmake "
		message(STATUS \"processing dlllist.cmake\")
		# figure out which command to use for objdump
		execute_process(
			COMMAND ${CMAKE_C_COMPILER} -dumpmachine
			OUTPUT_VARIABLE OBJDUMP
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)
		# figure out where we should search for libraries
		execute_process(
			COMMAND ${CMAKE_C_COMPILER} -print-search-dirs
			COMMAND sed -nE \"/^libraries: =/{s///;s,/lib/?\\\(:|\\\$\\\$\\\),/bin\\\\1,g;p;q;}\"
			OUTPUT_VARIABLE ADDPATH
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)
		# since cmake doesn't appear to give us a variable with
		# all libraries we link against, grab the link.txt script
		# instead and drop the command name from it (before the
		# first space) -- this will fail if the full path for the
		# linker used contains a space...
		execute_process(
			COMMAND tail -1 CMakeFiles/subsurface.dir/link.txt
			COMMAND cut -d\\  -f 2-
			OUTPUT_VARIABLE LINKER_LINE
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)
		# finally run our win-ldd.pl script against that to
		# collect all the required dlls
		execute_process(
			COMMAND sh -c \"OBJDUMP=\${OBJDUMP}-objdump PATH=$ENV{PATH}:\${ADDPATH} perl ${CMAKE_SOURCE_DIR}/scripts/win-ldd.pl ${SUBSURFACE_TARGET}.exe \${LINKER_LINE}\"
			WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
			OUTPUT_VARIABLE DLLS
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)
		# replace newlines with semicolons so this is a cmake list
		string(REPLACE \"\\n\" \";\" DLLLIST \${DLLS})
		# executing 'install' as a command seems hacky, but you
		# can't use the install() cmake function in a script
		foreach(DLL \${DLLLIST})
			execute_process(COMMAND install \${DLL} \${STAGING})
		endforeach()
	")
	# the script we created above is now added as a command to run at
	# install time - so this ensures that subsurface.exe has been
	# built before this is run
	install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -DSTAGING=${WINDOWSSTAGING} -P ${CMAKE_BINARY_DIR}/dlllist.cmake)")

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
endif()

if(ANDROID)
	# Android template directory
	include(${QT_ANDROID_CMAKE})
	if(SUBSURFACE_MOBILE)
		set(ANDROID_PACKAGE_SOURCE_DIR, ${CMAKE_BINARY_DIR}/android-mobile)
		add_qt_android_apk(subsurface-mobile.apk subsurface-mobile
			PACKAGE_SOURCES ${CMAKE_CURRENT_LIST_DIR}/android-mobile
		)
	else()
		set(ANDROID_PACKAGE_SOURCE_DIR, ${CMAKE_BINARY_DIR}/android)
		add_qt_android_apk(subsurface.apk ${SUBSURFACE_TARGET}
			PACKAGE_SOURCES ${CMAKE_CURRENT_LIST_DIR}/android
		)
	endif()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	install(DIRECTORY marbledata/maps DESTINATION share/subsurface/data)
	install(DIRECTORY marbledata/bitmaps DESTINATION share/subsurface/data)
	install(FILES subsurface.debug DESTINATION bin)
	install(FILES subsurface.desktop DESTINATION share/applications)
	install(FILES icons/subsurface-icon.svg DESTINATION share/icons/hicolor/scalable/apps)
	install(DIRECTORY Documentation/images DESTINATION share/subsurface/Documentation)
	install(FILES ${DOCFILES} DESTINATION share/subsurface/Documentation)
	install(DIRECTORY theme DESTINATION share/subsurface)
	install(DIRECTORY printing_templates DESTINATION share/subsurface)
	install(FILES ${TRANSLATIONS} DESTINATION share/subsurface/translations)
	if(SUBSURFACE_MOBILE)
		install(TARGETS subsurface-mobile DESTINATION bin)
	else()
		install(TARGETS ${SUBSURFACE_TARGET} DESTINATION bin)
	endif()
	if(DEFINED LIBMARBLEDEVEL)
		install(
			CODE "file(GLOB SSRFMARBLE_SHLIBS \"${LIBMARBLEDEVEL}/lib/libssrfmarblewidget.so*\")"
			CODE "file(INSTALL \${SSRFMARBLE_SHLIBS} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)"
		)
	endif()
endif()

# get_cmake_property(_variableNames VARIABLES)
# foreach (_variableName ${_variableNames})
#     message(STATUS "${_variableName}=${${_variableName}}")
# endforeach()

if (MAKE_TESTS)
	add_subdirectory(tests)
endif()
