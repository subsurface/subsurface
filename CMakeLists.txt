# cmake based build of Subsurface

project(Subsurface)
cmake_minimum_required(VERSION 2.8.11)

# global settings

set(CMAKE_AUTOMOC ON)
include(cmake/Modules/MacroOutOfSourceBuild.cmake)
MACRO_ENSURE_OUT_OF_SOURCE_BUILD(
    "We don't support building in source, please create a build folder elsewhere and remember to run git clean -xdf to remoev temporary files created by CMake."
)

#Options regarding usage of pkgconfig
option(LIBGIT2_FROM_PKGCONFIG "use pkg-config to retrieve libgit2" OFF)
option(LIBDC_FROM_PKGCONFIG "use pkg-config to retrieve libdivecomputer" OFF)
option(LIBGRANTLEE_FROM_PKGCONFIG "use pkg-config to retrieve grantlee" OFF)
option(LIBMARBLE_FROM_PKGCONFIG "use pkg-config to retrieve marble" OFF)

#Library Handling
option(USE_LIBGIT23_API "allow building with libgit2 master" OFF)
option(FORCE_LIBSSH "force linking with libssh to workaround libgit2 bug" ON)

#Options regarding disabling parts of subsurface.
option(NO_MARBLE "disable the marble widget" OFF)
option(NO_DOCS "disable the docs" OFF)
option(NO_PRINTING "disable the printing support" OFF)
option(NO_USERMANUAL "don't include a viewer for the user manual" OFF)

#Options regarding enabling parts of subsurface
option(FBSUPPORT "allow posting to Facebook" ON)
option(BTSUPPORT "enable support for QtBluetooth (requires Qt5.4 or newer)" ON)
option(FTDISUPPORT "enable support for libftdi based serial" OFF)

# Options regarding What should we build on subsurface
option(MAKE_TESTS "Make the tests" ON)

#Extra features
option(SMARTTRAK_IMPORT "enable building SmartTrak divelogs import tool (requires glib2 and libmdb)" OFF)

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

add_definitions(-DSUBSURFACE_SOURCE="${CMAKE_SOURCE_DIR}")

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
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 ")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
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
include(cmake/Modules/HandleFindMarble.cmake)
include(cmake/Modules/HandleFindGrantlee.cmake)
include(cmake/Modules/HandleFtdiSupport.cmake)
include(cmake/Modules/HandleUserManual.cmake)
include(cmake/Modules/HandleVersionGeneration.cmake)
include(cmake/Modules/RunOnBuildDir.cmake)

pkg_config_library(LIBXML libxml-2.0 REQUIRED)
pkg_config_library(LIBSQLITE3 sqlite3 REQUIRED)
pkg_config_library(LIBXSLT libxslt REQUIRED)
pkg_config_library(LIBZIP libzip REQUIRED)
pkg_config_library(LIBUSB libusb-1.0 QUIET)

if(SMARTTRAK_IMPORT)
	pkg_config_library(GLIB2 glib-2.0 REQUIRED)
	pkg_config_library(LIBMDB libmdb REQUIRED)
endif()

include_directories(.
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_BINARY_DIR}
	${CMAKE_BINARY_DIR}/desktop-widgets
	desktop-widgets/
	qt-models
	desktop-widgets/profile
	subsurface-core/
)

# Project Target specific configuration should go here,
# if the configuration is too big or would disrupt the normal code flux,
# move it somewhere else (another file) and include it.
if(${SUBSURFACE_TARGET_EXECUTABLE} MATCHES "DesktopExecutable")
    set(SUBSURFACE_TARGET subsurface)
elseif(${SUBSURFACE_TARGET_EXECUTABLE} MATCHES "MobileExecutable")
    set(SUBSURFACE_TARGET subsurface-mobile)
    list(APPEND QT_EXTRA_COMPONENTS Quick)
    list(APPEND QT_EXTRA_LIBRARIES Qt5::Quick)
    add_definitions(-DSUBSURFACE_MOBILE)
endif()

if(ANDROID)
	set(FBSUPPORT OFF)
	set(NO_PRINTING ON)
	list(APPEND QT_EXTRA_COMPONENTS AndroidExtras)
	list(APPEND QT_EXTRA_LIBRARIES Qt5::AndroidExtras)
	set(SUBSURFACE_LINK_LIBRARIES ${SUBSURFACE_LINK_LIBRARIES} -llog)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # add pthread to the end of the library list on Linux
        # this is only needed on Ubuntu
        # but shouldn't hurt on other Linux versions
        # in some builds we appear to be missing libz for some strange reason...
        set(SUBSURFACE_LINK_LIBRARIES ${SUBSURFACE_LINK_LIBRARIES} -lz -lpthread)
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
		set(MACOSX_BUNDLE_ICON_FILE Subsurface.icns)
		set(MACOSX_BUNDLE_GUI_IDENTIFIER "org.subsurface-divelog")
		set(MACOSX_BUNDLE_BUNDLE_NAME "Subsurface-mobile")
	else()
		set(MACOSX_BUNDLE_INFO_STRING "Subsurface")
		set(MACOSX_BUNDLE_ICON_FILE Subsurface.icns)
		set(MACOSX_BUNDLE_GUI_IDENTIFIER "org.subsurface-divelog")
		set(MACOSX_BUNDLE_BUNDLE_NAME "Subsurface")
	endif()
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
        add_definitions(-DBT_SUPPORT)
endif()

find_package(Qt5 REQUIRED COMPONENTS Core Concurrent Widgets Network Svg Test LinguistTools Positioning ${QT_EXTRA_COMPONENTS})
set(QT_LIBRARIES Qt5::Core Qt5::Concurrent Qt5::Widgets Qt5::Network Qt5::Svg  Qt5::Positioning ${QT_EXTRA_LIBRARIES})
set(QT_TEST_LIBRARIES ${QT_LIBRARIES} Qt5::Test)

#disable bluetooth if Qt version is ancient.
if (BTSUPPORT AND "${Qt5Core_VERSION_STRING}" STRLESS "5.4.0")
	set(BTSUPPORT OFF)
	message(STATUS "Turning off Bluetooth support as Qt version ${Qt5Core_VERSION_STRING} is insufficient for that")
	list(REMOVE_ITEM QT_LIBRARIES Qt5::Bluetooth)
endif()

#set up the subsurface_link_libraries variable
set(SUBSURFACE_LINK_LIBRARIES ${SUBSURFACE_LINK_LIBRARIES} ${LIBDIVECOMPUTER_LIBRARIES} ${LIBGIT2_LIBRARIES} ${LIBUSB_LIBRARIES})
qt5_add_resources(SUBSURFACE_RESOURCES subsurface.qrc)

# include translations
add_subdirectory(translations)
add_subdirectory(subsurface-core)
add_subdirectory(qt-models)
add_subdirectory(profile-widget)

if(SMARTTRAK_IMPORT)
	add_subdirectory(smtk-import)
endif()

if (${SUBSURFACE_TARGET_EXECUTABLE} MATCHES "DesktopExecutable")
	add_subdirectory(desktop-widgets)
endif()

if(FBSUPPORT)
	add_definitions(-DFBSUPPORT)
	set(FACEBOOK_INTEGRATION facebook_integration)
endif()

# create the executables
if(${SUBSURFACE_TARGET_EXECUTABLE} MATCHES "MobileExecutable")
	set(SUBSURFACE_TARGET subsurface-mobile)
	set(MOBILE_SRC
		qt-mobile/qmlmanager.cpp
		qt-mobile/qmlprofile.cpp
		qt-models/divelistmodel.cpp
		subsurface-mobile-main.cpp
		subsurface-mobile-helper.cpp
	)
	qt5_add_resources(MOBILE_RESOURCES qt-mobile/qml/mobile-resources.qrc)
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
                add_library(${SUBSURFACE_TARGET} MACOSX_BUNDLE WIN32 ${SUBSURFACE_PKG} ${SUBSURFACE_APP} ${SUBSURFACE_RESOURCES})
        else()
                add_executable(${SUBSURFACE_TARGET} MACOSX_BUNDLE WIN32 ${SUBSURFACE_PKG} ${SUBSURFACE_APP} ${SUBSURFACE_RESOURCES})
        endif()

        if(SMARTTRAK_IMPORT)
		set(SMTK_IMPORT_TARGET smtk2ssrf)
		add_executable(smtk2ssrf smtk-import/smtk_standalone.cpp ${SUBSURFACE_RESOURCES})
		target_link_libraries(smtk2ssrf smtk_import)
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

FILE(STRINGS "subsurface_enabled_translations" QTTRANSLATIONS_BASE)

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
if(ANDROID)
	# Android template directory
	include(${QT_ANDROID_CMAKE})
	set(ANDROID_PACKAGE_SOURCE_DIR, ${CMAKE_BINARY_DIR}/android-mobile)
	add_qt_android_apk(subsurface-mobile.apk subsurface-mobile
		PACKAGE_SOURCES ${CMAKE_CURRENT_LIST_DIR}/android-mobile
	)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	if(${SUBSURFACE_TARGET_EXECUTABLE} MATCHES "MobileExecutable")
		set(APP_BUNDLE_DIR Subsurface-mobile.app)
		set(MACDEPLOY_ARGS "-qmldir=${APP_BUNDLE_DIR}/Contents/Frameworks/qml")
	else()
		set(APP_BUNDLE_DIR Subsurface.app)
	endif()
	set(RESOURCEDIR ${CMAKE_BINARY_DIR}/${APP_BUNDLE_DIR}/Contents/Resources)
	set(PLUGINDIR ${CMAKE_BINARY_DIR}/${APP_BUNDLE_DIR}/Contents/PlugIns)
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
	install(CODE "execute_process(COMMAND ${MACDEPLOYQT} ${APP_BUNDLE_DIR} ${MACDEPLOY_ARGS})")
	# and another hack to get the QML Components in the right place
	if(${SUBSURFACE_TARGET_EXECUTABLE} MATCHES "MobileExecutable")
		install(CODE "execute_process(COMMAND mkdir -p ${CMAKE_BINARY_DIR}/${APP_BUNDLE_DIR}/Contents/Frameworks/qml)")
		install(CODE "execute_process(COMMAND cp -a ${_qt5Core_install_prefix}/qml/QtQuick ${CMAKE_BINARY_DIR}/${APP_BUNDLE_DIR}/Contents/Frameworks/qml)")
	endif()
	if(NOT NO_MARBLE)
		# more hackery - this time for QtXml which is needed by libssrfmarblewidget
		install(CODE "execute_process(COMMAND cp -a ${_qt5Core_install_prefix}/lib/QtXml.framework ${CMAKE_BINARY_DIR}/${APP_BUNDLE_DIR}/Contents/Frameworks)")
		install(CODE "execute_process(COMMAND rm -rf ${CMAKE_BINARY_DIR}/${APP_BUNDLE_DIR}/Contents/Frameworks/QtXml.framework/Versions/5/Headers)")
		install(CODE "execute_process(COMMAND rm -rf ${CMAKE_BINARY_DIR}/${APP_BUNDLE_DIR}/Contents/Frameworks/QtXml.framework/Headers)")
		install(CODE "execute_process(COMMAND rm -rf ${CMAKE_BINARY_DIR}/${APP_BUNDLE_DIR}/Contents/Frameworks/QtXml.framework/QtXml.prl)")
		install(CODE "execute_process(COMMAND rm -rf ${CMAKE_BINARY_DIR}/${APP_BUNDLE_DIR}/Contents/Frameworks/QtXml.framework/Versions/5/*_debug)")
		install(CODE "execute_process(COMMAND rm -rf ${CMAKE_BINARY_DIR}/${APP_BUNDLE_DIR}/Contents/Frameworks/QtXml.framework//*_debug)")
		install(CODE "execute_process(COMMAND install_name_tool -id @executable_path/../Frameworks/QtXml ${CMAKE_BINARY_DIR}/${APP_BUNDLE_DIR}/Contents/Frameworks/QtXml.framework/QtXml)")
		install(CODE "execute_process(COMMAND install_name_tool -change @rpath/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/QtCore ${CMAKE_BINARY_DIR}/${APP_BUNDLE_DIR}/Contents/Frameworks/QtXml.framework/QtXml)")
	endif()
	install(CODE "message(STATUS \"two ERRORS here about libmysqlclient and libpq not found are harmless\")")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
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

	# the script we created above is now added as a command to run at
	# install time - so this ensures that subsurface.exe has been
	# built before this is run
	install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -DSTAGING=${WINDOWSSTAGING} -P cmake/Modules/dlllist.cmake)")

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
		if (SMARTTRAK_IMPORT)
			install(TARGETS ${SMTK_IMPORT_TARGET} DESTINATION bin)
		endif()
	endif()
	if(DEFINED LIBMARBLEDEVEL)
		install(
			CODE "file(GLOB SSRFMARBLE_SHLIBS \"${LIBMARBLEDEVEL}/lib/libssrfmarblewidget.so*\")"
			CODE "file(INSTALL \${SSRFMARBLE_SHLIBS} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)"
		)
	endif()
endif()

if (MAKE_TESTS)
	add_subdirectory(tests)
endif()
