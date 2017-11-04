# cmake based build of Subsurface

# Uncomment his to see all commands cmake actually executes
# set(CMAKE_VERBOSE_MAKEFILE ON)

project(Subsurface)
cmake_minimum_required(VERSION 2.8.11)

# global settings

set(CMAKE_AUTOMOC ON)
include(cmake/Modules/MacroOutOfSourceBuild.cmake)
MACRO_ENSURE_OUT_OF_SOURCE_BUILD(
    "We don't support building in source, please create a build folder elsewhere and remember to run git clean -xdf to remove temporary files created by CMake."
)

#Options regarding usage of pkgconfig
option(LIBGIT2_FROM_PKGCONFIG "use pkg-config to retrieve libgit2" OFF)
option(LIBDC_FROM_PKGCONFIG "use pkg-config to retrieve libdivecomputer" OFF)
option(LIBGRANTLEE_FROM_PKGCONFIG "use pkg-config to retrieve grantlee" OFF)

#Library Handling
option(FORCE_LIBSSH "force linking with libssh to workaround libgit2 bug" ON)
option(LIBGIT2_DYNAMIC "search for libgit2.so before libgit2.a" OFF)

#Options regarding disabling parts of subsurface.
option(NO_DOCS "disable the docs" OFF)
option(NO_PRINTING "disable the printing support" OFF)
option(NO_USERMANUAL "don't include a viewer for the user manual" OFF)

#Options regarding enabling parts of subsurface
option(FBSUPPORT "allow posting to Facebook" ON)
option(BTSUPPORT "enable support for QtBluetooth (requires Qt5.4 or newer)" ON)
option(FTDISUPPORT "enable support for libftdi based serial" OFF)
option(USE_WEBENGINE "Use QWebEngine instead of QWebKit" OFF)

# Options regarding What should we build on subsurface
option(MAKE_TESTS "Make the tests" ON)

SET(SUBSURFACE_TARGET_EXECUTABLE "DesktopExecutable" CACHE STRING "The type of application, DesktopExecutable or MobileExecutable")
LIST(APPEND SUBSURFACE_ACCEPTED_EXECUTABLES  "DesktopExecutable" "MobileExecutable")
SET_PROPERTY(CACHE SUBSURFACE_TARGET_EXECUTABLE PROPERTY STRINGS ${SUBSURFACE_ACCEPTED_EXECUTABLES})

#verify if Platform is correct and warn on wxit with example.
list (FIND SUBSURFACE_ACCEPTED_EXECUTABLES ${SUBSURFACE_TARGET_EXECUTABLE}  _index)
if (${_index} EQUAL -1)
  message(FATAL_ERROR "Requested platform not supported, please use one of the following:
    ${SUBSURFACE_ACCEPTED_EXECUTABLES}

    inputted value was: ${SUBSURFACE_TARGET_EXECUTABLE}

    Example: -DSUBSURFACE_TARGET_EXECUTABLE=DesktopExecutable")
endif()

# SUBSURFACE_SOURCE may be used in subdirectories (tests)
set(SUBSURFACE_SOURCE ${CMAKE_SOURCE_DIR})
add_definitions(-DSUBSURFACE_SOURCE="${SUBSURFACE_SOURCE}")

#evenrything's correct, moving on.
set(CMAKE_MODULE_PATH
	${CMAKE_MODULE_PATH}
	${${PROJECT_NAME}_SOURCE_DIR}/cmake/Modules
)

#
# TODO: This Compilation part should go on the Target specific CMake.
#
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 ")
	if((${CMAKE_SYSTEM_NAME} MATCHES "Darwin") AND
	   ((${CMAKE_SYSTEM_VERSION} MATCHES "11.4.") OR
	    (${CMAKE_OSX_DEPLOYMENT_TARGET} MATCHES "10.7") OR
	    (${CMAKE_OSX_DEPLOYMENT_TARGET} MATCHES "10.8")))
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 ")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

	# Warn about possible float conversion errors
	# Use NOT VERSION_LESS since VERSION_GREATER_EQUAL is not available
	# in currently used cmake version.
	if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.9.0")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wfloat-conversion")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wfloat-conversion")
	endif()

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  # using Intel C++
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  # using Visual Studio C++
endif()

# pkgconfig for required libraries
find_package(PkgConfig)
include(cmake/Modules/pkgconfig_helper.cmake)

# The 'HandleFindXXX' are special libraries that subsurface needs
# to find and configure in a few different ways because of a few
# developers that prefer pkg-config over CMake, so we try to make
# everyone happy. It also sets some variables for each library, so
# if you think a module miss anything, take a look on the specific
# module file.
include(cmake/Modules/HandleFindGit2.cmake)
include(cmake/Modules/HandleFindLibDiveComputer.cmake)
if(${SUBSURFACE_TARGET_EXECUTABLE} MATCHES "DesktopExecutable")
	include(cmake/Modules/HandleFindGrantlee.cmake)
	include(cmake/Modules/HandleUserManual.cmake)
endif()
include(cmake/Modules/HandleFtdiSupport.cmake)
include(cmake/Modules/HandleVersionGeneration.cmake)
include(cmake/Modules/RunOnBuildDir.cmake)
include(cmake/Modules/cmake_variables_helper.cmake)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	include_directories(${CMAKE_OSX_SYSROOT}/usr/include/libxml2)
	set(SUBSURFACE_LINK_LIBRARIES ${SUBSURFACE_LINK_LIBRARIES} -lxml2 -lxslt -lsqlite3)
else()
	pkg_config_library(LIBXML libxml-2.0 REQUIRED)
	pkg_config_library(LIBSQLITE3 sqlite3 REQUIRED)
	pkg_config_library(LIBXSLT libxslt REQUIRED)
endif()
pkg_config_library(LIBZIP libzip REQUIRED)
pkg_config_library(LIBUSB libusb-1.0 QUIET)

include_directories(.
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_BINARY_DIR}
	${CMAKE_BINARY_DIR}/desktop-widgets
)

# Project Target specific configuration should go here,
# if the configuration is too big or would disrupt the normal code flux,
# move it somewhere else (another file) and include it.
if(${SUBSURFACE_TARGET_EXECUTABLE} MATCHES "DesktopExecutable")
	if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
		set(SUBSURFACE_TARGET Subsurface)
	else()
		set(SUBSURFACE_TARGET subsurface)
	endif()
	list(APPEND QT_EXTRA_COMPONENTS Quick)
	list(APPEND QT_EXTRA_COMPONENTS Location)
	list(APPEND QT_EXTRA_COMPONENTS QuickWidgets)
	list(APPEND QT_EXTRA_LIBRARIES Qt5::Quick)
	list(APPEND QT_EXTRA_LIBRARIES Qt5::Location)
	list(APPEND QT_EXTRA_LIBRARIES Qt5::QuickWidgets)
	remove_definitions(-DSUBSURFACE_MOBILE)
elseif(${SUBSURFACE_TARGET_EXECUTABLE} MATCHES "MobileExecutable")
	if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
		set(SUBSURFACE_TARGET Subsurface-mobile)
	else()
		set(SUBSURFACE_TARGET subsurface-mobile)
	endif()
	list(APPEND QT_EXTRA_COMPONENTS Quick)
	list(APPEND QT_EXTRA_COMPONENTS QuickControls2)
	list(APPEND QT_EXTRA_LIBRARIES Qt5::Quick)
	list(APPEND QT_EXTRA_LIBRARIES Qt5::QuickControls2)
	add_definitions(-DSUBSURFACE_MOBILE)
endif()

if(ANDROID)
	set(NO_PRINTING ON)
	list(APPEND QT_EXTRA_COMPONENTS AndroidExtras)
	list(APPEND QT_EXTRA_LIBRARIES Qt5::AndroidExtras)
	set(SUBSURFACE_LINK_LIBRARIES ${SUBSURFACE_LINK_LIBRARIES} -llog)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # add pthread to the end of the library list on Linux
        # this is only needed on Ubuntu
        # but shouldn't hurt on other Linux versions
        # in some builds we appear to be missing libz for some strange reason...
	# Add ssh2 at the end for openSUSE builds (for recent cmake?)
        set(SUBSURFACE_LINK_LIBRARIES ${SUBSURFACE_LINK_LIBRARIES} -lssh2 -lz -lpthread)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        execute_process(
                COMMAND sh scripts/get-version linux
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                OUTPUT_VARIABLE SSRF_VERSION_STRING
                OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        find_library(APP_SERVICES_LIBRARY ApplicationServices)
        find_library(HID_LIB HidApi)
        set(SUBSURFACE_LINK_LIBRARIES ${SUBSURFACE_LINK_LIBRARIES} ${HID_LIB})
        set(EXTRA_LIBS ${APP_SERVICES_LIBRARY})
        set(ICON_FILE ${CMAKE_SOURCE_DIR}/packaging/macosx/Subsurface.icns)
	if(${SUBSURFACE_TARGET_EXECUTABLE} MATCHES "MobileExecutable")
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
        set(MACOSX_BUNDLE_COPYRIGHT "Linus Torvalds, Dirk Hohndel, Tomaz Canabrava, and others")
        set_source_files_properties(${ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
        set(SUBSURFACE_PKG MACOSX_BUNDLE ${ICON_FILE})
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(SUBSURFACE_LINK_LIBRARIES ${SUBSURFACE_LINK_LIBRARIES} -lwsock32 -lws2_32)
        remove_definitions(-DUNICODE)
        add_definitions(-mwindows -D_WIN32)
endif()

if(BTSUPPORT)
	list(APPEND QT_EXTRA_COMPONENTS Bluetooth)
	list(APPEND QT_EXTRA_LIBRARIES Qt5::Bluetooth)
endif()

find_package(Qt5 REQUIRED COMPONENTS Core Concurrent Widgets Network Svg Test LinguistTools Positioning ${QT_EXTRA_COMPONENTS})
set(QT_LIBRARIES Qt5::Core Qt5::Concurrent Qt5::Widgets Qt5::Network Qt5::Svg  Qt5::Positioning ${QT_EXTRA_LIBRARIES})
set(QT_TEST_LIBRARIES ${QT_LIBRARIES} Qt5::Test)

#disable bluetooth if Qt version is ancient.
if (BTSUPPORT AND "${Qt5Core_VERSION}" VERSION_LESS 5.4.0)
	set(BTSUPPORT OFF)
	message(STATUS "Turning off Bluetooth support as Qt version ${Qt5Core_VERSION} is insufficient for that")
	list(REMOVE_ITEM QT_LIBRARIES Qt5::Bluetooth)
endif()

# Windows Qt doesn't support BLE at all
# the rest of them need at least 5.6 to be reasonable but really, you want 5.9.1
if (BTSUPPORT AND NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
	if ("${Qt5Core_VERSION}" VERSION_LESS 5.6.0)
		message(STATUS "Turning off BLE support as Qt version ${Qt5Core_VERSION} is insufficient for that")
	else()
		set(BLESUPPORT ON)
		if ("${Qt5Core_VERSION}" VERSION_LESS 5.9.1)
			message(STATUS "BLE support has been enabled, but for best results you really want Qt 5.9.1")
		endif()
	endif()
endif()

if(BTSUPPORT)
	add_definitions(-DBT_SUPPORT)
endif()

if(BLESUPPORT)
	add_definitions(-DBLE_SUPPORT)
endif()

#set up the subsurface_link_libraries variable
set(SUBSURFACE_LINK_LIBRARIES ${SUBSURFACE_LINK_LIBRARIES} ${LIBDIVECOMPUTER_LIBRARIES} ${LIBGIT2_LIBRARIES} ${LIBUSB_LIBRARIES})
qt5_add_resources(SUBSURFACE_RESOURCES subsurface.qrc)

# include translations
add_subdirectory(translations)
add_subdirectory(core)
add_subdirectory(qt-models)
add_subdirectory(profile-widget)

if (${SUBSURFACE_TARGET_EXECUTABLE} MATCHES "DesktopExecutable")
	add_subdirectory(desktop-widgets)
endif()

# create the executables
if(${SUBSURFACE_TARGET_EXECUTABLE} MATCHES "MobileExecutable")
	set(MOBILE_SRC
		mobile-widgets/qmlmanager.cpp
		mobile-widgets/qmlprofile.cpp
		mobile-widgets/qml/kirigami/src/kirigamiplugin.cpp
		mobile-widgets/qml/kirigami/src/settings.cpp
		mobile-widgets/qml/kirigami/src/enums.cpp
		mobile-widgets/qml/kirigami/src/libkirigami/basictheme.cpp
		mobile-widgets/qml/kirigami/src/libkirigami/kirigamipluginfactory.cpp
		mobile-widgets/qml/kirigami/src/libkirigami/platformtheme.cpp
		subsurface-mobile-main.cpp
		subsurface-mobile-helper.cpp
	)
	include_directories(${CMAKE_SOURCE_DIR}/mobile-widgets/qml/kirigami/src/libkirigami)
	if(NOT ANDROID AND NOT iOS)
		set(MOBILE_SRC ${MOBILE_SRC} mobile-widgets/qml/kirigami/src/desktopicon.cpp)
	endif()
	add_definitions(-DKIRIGAMI_BUILD_TYPE_STATIC)
	qt5_add_resources(MOBILE_RESOURCES mobile-widgets/qml/mobile-resources.qrc)
	qt5_add_resources(MOBILE_RESOURCES mobile-widgets/qml/kirigami/kirigami.qrc)
	# When building the mobile application in Android, link it and Qt will do the rest, when doing the mobile application on Desktop, create an executable.
	if(ANDROID)
		add_library(${SUBSURFACE_TARGET} SHARED ${SUBSURFACE_PKG} ${MOBILE_SRC} ${SUBSURFACE_RESOURCES} ${MOBILE_RESOURCES})
	else()
		add_executable(${SUBSURFACE_TARGET} MACOSX_BUNDLE WIN32 ${SUBSURFACE_PKG} ${MOBILE_SRC} ${SUBSURFACE_RESOURCES} ${MOBILE_RESOURCES})
	endif()
	target_link_libraries(
		${SUBSURFACE_TARGET}
		subsurface_profile
		subsurface_models
		subsurface_corelib
		${SUBSURFACE_LINK_LIBRARIES}
	)
elseif(${SUBSURFACE_TARGET_EXECUTABLE} MATCHES "DesktopExecutable")
	set(SUBSURFACE_APP
		subsurface-desktop-main.cpp
		subsurface-desktop-helper.cpp
	)
	source_group("Subsurface App" FILES ${SUBSURFACE_APP})
        if(ANDROID)
                add_library(${SUBSURFACE_TARGET} SHARED ${SUBSURFACE_PKG} ${SUBSURFACE_APP} ${SUBSURFACE_RESOURCES})
        else()
                add_executable(${SUBSURFACE_TARGET} MACOSX_BUNDLE WIN32 ${SUBSURFACE_PKG} ${SUBSURFACE_APP} ${SUBSURFACE_RESOURCES})
        endif()

	if(FBSUPPORT)
		set(FACEBOOK_INTEGRATION facebook_integration)
		add_dependencies(facebook_integration subsurface_generated_ui)
	endif()
	target_link_libraries(
		${SUBSURFACE_TARGET}
		subsurface_generated_ui
		subsurface_interface
		${FACEBOOK_INTEGRATION}
		subsurface_profile
		subsurface_statistics
		subsurface_models
		subsurface_corelib
		${SUBSURFACE_LINK_LIBRARIES}
	)
	add_dependencies(subsurface_desktop_preferences subsurface_generated_ui)
	add_dependencies(subsurface_statistics subsurface_generated_ui)
	add_dependencies(subsurface_interface subsurface_generated_ui)
	add_dependencies(subsurface_profile subsurface_generated_ui)
	add_dependencies(subsurface_models subsurface_generated_ui)
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

# install Subsurface
# first some variables with files that need installing
set(DOCFILES
	README.md
	ReleaseNotes/ReleaseNotes.txt
	SupportedDivecomputers.txt
	${CMAKE_BINARY_DIR}/Documentation/user-manual.html
	${CMAKE_BINARY_DIR}/Documentation/user-manual_es.html
	${CMAKE_BINARY_DIR}/Documentation/user-manual_fr.html
	${CMAKE_BINARY_DIR}/Documentation/user-manual_ru.html
)

# add all the translations that we may need
FILE(STRINGS "subsurface_enabled_translations" QTTRANSLATIONS_BASE)

if(NOT DEFINED QT_TRANSLATION_DIR OR QT_TRANSLATION_DIR STREQUAL "")
	set(QT_TRANSLATION_DIR ${Qt5Core_DIR}/../../../translations)
endif()
set(QTTRANSLATIONS "")
foreach(QTTRANSLATION ${QTTRANSLATIONS_BASE})
	if(NOT ${QTTRANSLATION} STREQUAL "")
		if(EXISTS ${QT_TRANSLATION_DIR}/${QTTRANSLATION})
			set(QTTRANSLATIONS ${QTTRANSLATIONS} ${QT_TRANSLATION_DIR}/${QTTRANSLATION})
		endif()
	endif()
endforeach()

# now for each platform the install instructions
if(ANDROID)
	# Android template directory
	include(${QT_ANDROID_CMAKE})
	if((DEFINED ENV{KEYSTORE}) AND (DEFINED ENV{KEYSTORE_PASSWORD}))
		add_qt_android_apk(${SUBSURFACE_TARGET}.apk ${SUBSURFACE_TARGET}
			PACKAGE_SOURCES ${CMAKE_BINARY_DIR}/android-mobile DEPENDS ${ANDROID_NATIVE_LIBSSL} ${ANDROID_NATIVE_LIBCRYPT}
			BUILDTOOLS_REVISION ${BUILDTOOLS_REVISION}
			KEYSTORE $ENV{KEYSTORE} Subsurface-mobile KEYSTORE_PASSWORD $ENV{KEYSTORE_PASSWORD}
		)
		message(STATUS "KEYSTORE=$ENV{KEYSTORE} KEYSTORE_PASSWORD=$ENV{KEYSTORE_PASSWORD}")
	else()
		add_qt_android_apk(${SUBSURFACE_TARGET}.apk ${SUBSURFACE_TARGET}
			PACKAGE_SOURCES ${CMAKE_BINARY_DIR}/android-mobile DEPENDS ${ANDROID_NATIVE_LIBSSL} ${ANDROID_NATIVE_LIBCRYPT}
			BUILDTOOLS_REVISION ${BUILDTOOLS_REVISION}
		)
		message(STATUS "no KEYSTORE")
	endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	set(APP_BUNDLE_DIR "${SUBSURFACE_TARGET}.app")
	set(EXTRA_MACDEPLOY_ARGS "-qmldir=${APP_BUNDLE_DIR}/Contents/Resources/qml ")
	set(MACDEPLOY_ARGS "${EXTRA_MACDEPLOY_ARGS}-executable=${APP_BUNDLE_DIR}/Contents/MacOS/${SUBSURFACE_TARGET} -always-overwrite")
	set(RESOURCEDIR ${CMAKE_BINARY_DIR}/${APP_BUNDLE_DIR}/Contents/Resources)
	set(PLUGINDIR ${CMAKE_BINARY_DIR}/${APP_BUNDLE_DIR}/Contents/PlugIns)
	install(DIRECTORY Documentation/images DESTINATION ${RESOURCEDIR}/share/Documentation)
	install(FILES ${DOCFILES} DESTINATION ${RESOURCEDIR}/share/Documentation)
	install(DIRECTORY theme DESTINATION ${RESOURCEDIR})
	install(DIRECTORY printing_templates DESTINATION ${RESOURCEDIR})
	install(FILES ${TRANSLATIONS} DESTINATION ${RESOURCEDIR}/translations)
	install(FILES ${QTTRANSLATIONS} DESTINATION ${RESOURCEDIR}/translations)
	install(FILES ${CMAKE_SOURCE_DIR}/gpl-2.0.txt DESTINATION ${RESOURCEDIR})
	install(CODE "execute_process(COMMAND mkdir -p ${RESOURCEDIR}/qml)")
	# this is a HACK
	if((${SUBSURFACE_TARGET_EXECUTABLE} MATCHES "DesktopExecutable") AND (NOT NO_PRINTING))
		install(DIRECTORY ${Grantlee5_DIR}/../../grantlee DESTINATION ${PLUGINDIR})
	endif()
	install(CODE "execute_process(COMMAND mkdir -p ${PLUGINDIR}/geoservices)")
	install(CODE "execute_process(COMMAND cp ${_qt5Core_install_prefix}/plugins/geoservices/libqtgeoservices_googlemaps.dylib ${PLUGINDIR}/geoservices)")
	# this is a hack - but I don't know how else to find the macdeployqt program if it's not in the PATH
	string(REPLACE moc macdeployqt MACDEPLOYQT ${QT_MOC_EXECUTABLE})
	install(CODE "execute_process(COMMAND ${MACDEPLOYQT} ${APP_BUNDLE_DIR} ${MACDEPLOY_ARGS})")
	# and another hack to get the QML Components in the right place
	install(CODE "execute_process(COMMAND mkdir -p ${CMAKE_BINARY_DIR}/${APP_BUNDLE_DIR}/Contents/Frameworks/qml)")
	install(CODE "execute_process(COMMAND cp -a ${_qt5Core_install_prefix}/qml/QtQuick.2 ${CMAKE_BINARY_DIR}/${APP_BUNDLE_DIR}/Contents/Resources/qml)")
	install(CODE "execute_process(COMMAND cp -a ${_qt5Core_install_prefix}/qml/QtLocation ${CMAKE_BINARY_DIR}/${APP_BUNDLE_DIR}/Contents/Resources/qml)")
	install(CODE "execute_process(COMMAND cp -a ${_qt5Core_install_prefix}/qml/QtPositioning ${CMAKE_BINARY_DIR}/${APP_BUNDLE_DIR}/Contents/Resources/qml)")
	install(CODE "message(STATUS \"two ERRORS here about libmysqlclient and libpq not found are harmless\")")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# Windows bundling rules
	# We don't have a helpful tool like macdeployqt for Windows, so we hardcode
	# which libs we need.
	# "make install", copies everything into a staging area
	# "make installer", uses makensis to create an installer executable
	set(WINDOWSSTAGING ${CMAKE_BINARY_DIR}/staging)
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

	# the script we created above is now added as a command to run at
	# install time - so this ensures that subsurface.exe has been
	# built before this is run
	install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} -DSUBSURFACE_TARGET=${SUBSURFACE_TARGET} -DSUBSURFACE_SOURCE=${SUBSURFACE_SOURCE} -DSTAGING=${WINDOWSSTAGING} -P ${CMAKE_SOURCE_DIR}/cmake/Modules/dlllist.cmake)")

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
	if ("${GOOGLEMAPS}" STREQUAL "")
		message(STATUS "Cannot find libqtgeoservices_googlemaps.so")
	else()
		add_custom_target(link_googlemaps_plugin ALL COMMAND
			rm -rf ${CMAKE_BINARY_DIR}/geoservices &&
			mkdir -p ${CMAKE_BINARY_DIR}/geoservices &&
			ln -sf ${GOOGLEMAPS} ${CMAKE_BINARY_DIR}/geoservices)
	endif()
	install(FILES subsurface.debug DESTINATION bin)
	install(FILES subsurface.desktop DESTINATION share/applications)
	install(FILES icons/subsurface-icon.svg DESTINATION share/icons/hicolor/scalable/apps)
	install(DIRECTORY Documentation/images DESTINATION share/subsurface/Documentation)
	install(FILES ${DOCFILES} DESTINATION share/subsurface/Documentation)
	install(DIRECTORY theme DESTINATION share/subsurface)
	install(DIRECTORY printing_templates DESTINATION share/subsurface)
	install(FILES ${TRANSLATIONS} DESTINATION share/subsurface/translations)
        install(TARGETS ${SUBSURFACE_TARGET} DESTINATION bin)
endif()

if (MAKE_TESTS)
	add_subdirectory(tests)
endif()

# useful for debugging CMake issues
# print_all_variables()
