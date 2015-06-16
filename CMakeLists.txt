# cmake based build of Subsurface

project(Subsurface)
cmake_minimum_required(VERSION 2.8.11)

# global settings

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
option(LIBGIT2_FROM_PKGCONFIG "use pkg-config to retrieve libgit2" OFF)
option(LIBDC_FROM_PKGCONFIG "use pkg-config to retrieve libdivecomputer" OFF)
option(LIBGRANTLEE_FROM_PKGCONFIG "use pkg-config to retrieve grantlee" OFF)
option(NO_MARBLE "disable the marble widget" OFF)
option(NO_TESTS "disable the tests" OFF)
option(NO_DOCS "disable the docs" OFF)
option(NO_PRINTING "disable the printing support" ON)
option(USE_LIBGIT23_API "allow building with libgit2 master" OFF)

set(CMAKE_MODULE_PATH
	${CMAKE_MODULE_PATH}
	${${PROJECT_NAME}_SOURCE_DIR}/cmake/Modules
	${${PROJECT_NAME}_SOURCE_SIR}/../install-root/lib/cmake/libssh2
)

include_directories(.
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_BINARY_DIR}
	qt-ui
	qt-models
	qt-ui/profile
)

# compiler specific settings
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUXX)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 ")
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
		pkg_config_library(LIBSSH2 libssh2 REQUIRED)
		set(LIBGIT2_LIBRARIES ${LIBGIT2_LIBRARIES} ${LIBSSH2_LIBRARIES})
	endif()
else()
	find_package(LIBGIT2 REQUIRED)
	include_directories(${LIBGIT2_INCLUDE_DIR})
	if(USE_LIBGIT23_API)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_LIBGIT23_API")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_LIBGIT23_API")
		find_package(libssh2 QUIET)
		if(!LIBSSH2_FOUND OR "${LIBSSH2_FOUND}" STREQUAL "")
			pkg_config_library(LIBSSH2 libssh2 REQUIRED)
		endif()
		find_package(libcurl QUIET)
		if(!LIBCURL_FOUND OR "${LIBCURL_FOUND}" STREQUAL "")
			pkg_config_library(LIBCURL libcurl REQUIRED)
		endif()
		set(LIBGIT2_LIBRARIES ${LIBGIT2_LIBRARIES} ${LIBSSH2_LIBRARIES} ${LIBCURL_LIBRARIES})
	endif()
endif()

if(LIBDC_FROM_PKGCONFIG)
	pkg_config_library(LIBDC libdivecomputer REQUIRED)
	set(LIBDIVECOMPUTER_LIBRARIES "")
else()
	find_package(Libdivecomputer REQUIRED)
	include_directories(${LIBDIVECOMPUTER_INCLUDE_DIR})
endif()

# optional marble
if(NOT NO_MARBLE)
	find_package(Marble QUIET)
	if(MARBLE_FOUND)
		include_directories(${MARBLE_INCLUDE_DIR})
	else()
		set(NO_MARBLE ON)
	endif()
endif()

if(NO_MARBLE)
	message(STATUS "building without marble widget support")
	add_definitions(-DNO_MARBLE)
	set(MARBLE_LIBRARIES "")
endif()

# setup Grantlee

if(NOT NO_PRINTING)
	if(LIBGRANTLEE_FROM_PKGCONFIG)
		pkg_config_library(GRANTLEE libgrantlee REQUIRED)
		set(GRANTLEE_LIBRARIES "")
	else()
		find_package(Grantlee5)
		set(GRANTLEE_LIBRARIES Grantlee5::Templates)
	endif()
	set(SUBSURFACE_PRINTING_SRCS
		printer.cpp
		templatelayout.cpp
	)
endif()

if(NO_PRINTING)
	message(STATUS "building without printing support")
	add_definitions(-DNO_PRINTING)
	set(GRANTLEE_LIBRARIES "")
endif()

set(SUBSURFACE_LINK_LIBRARIES ${SUBSURFACE_LINK_LIBRARIES} ${LIBDIVECOMPUTER_LIBRARIES} ${LIBGIT2_LIBRARIES} ${GRANTLEE_LIBRARIES} -lusb-1.0)

# handle out of tree build correctly
string(COMPARE EQUAL "${${PROJECT_NAME}_SOURCE_DIR}" "${${PROJECT_NAME}_BINARY_DIR}" insource)
get_filename_component(PARENTDIR ${${PROJECT_NAME}_SOURCE_DIR} PATH)
string(COMPARE EQUAL "${${PROJECT_NAME}_SOURCE_DIR}" "${PARENTDIR}" insourcesubdir)
if(NOT (insource OR insourcedir))
	if(NOT NO_MARBLE)
		add_custom_target(link_marble_data ALL COMMAND rm -f marbledata && ln -s ${${PROJECT_NAME}_SOURCE_DIR}/marbledata ${${PROJECT_NAME}_BINARY_DIR}/marbledata)
	endif()
endif()

# configure Qt.
find_package(Qt5 REQUIRED COMPONENTS Core Concurrent Widgets Network WebKitWidgets PrintSupport Svg Test LinguistTools)
set(QT_LIBRARIES Qt5::Core Qt5::Concurrent Qt5::Widgets Qt5::Network Qt5::WebKitWidgets Qt5::PrintSupport Qt5::Svg)
set(QT_TEST_LIBRARIES ${QT_LIBRARIES} Qt5::Test)

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
set(PLATFORM_SRC unknown_platform.c)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(FBSUPPORT 1)
	set(SUBSURFACE_TARGET subsurface)
	set(PLATFORM_SRC linux.c)
	# in some builds we appear to be missing libz for some strange reason...
	set(SUBSURFACE_LINK_LIBRARIES ${SUBSURFACE_LINK_LIBRARIES} -lz)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	set(FBSUPPORT 1)
	set(SUBSURFACE_TARGET Subsurface)
	set(PLATFORM_SRC macos.c)
	find_library(APP_SERVICES_LIBRARY ApplicationServices)
	set(EXTRA_LIBS ${APP_SERVICES_LIBRARY})
	set(ICON_FILE ${CMAKE_SOURCE_DIR}/packaging/macosx/Subsurface.icns)
	set(MACOSX_BUNDLE_INFO_STRING "Subsurface")
	set(MACOSX_BUNDLE_ICON_FILE Subsurface.icns)
	set(MACOSX_BUNDLE_GUI_IDENTIFIER "org.subsurface-divelog")
	set(MACOSX_BUNDLE_BUNDLE_NAME "Subsurface")
	set(MACOSX_BUNDLE_BUNDLE_VERSION "4.4.1")
	set(MACOSX_BUNDLE_SHORT_VERSION_STRING "4.4.1")
	set(MACOSX_BUNDLE_LONG_VERSION_STRING "4.4.1")
	set(MACOSX_BUNDLE_COPYRIGHT "Linus Torvalds, Dirk Hohndel, Tomaz Canabrava, and others")
	set_source_files_properties(${ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
	set(SUBSURFACE_PKG MACOSX_BUNDLE ${ICON_FILE})
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set(FBSUPPORT 1)
	set(SUBSURFACE_TARGET subsurface)
	set(PLATFORM_SRC windows.c)
	set(SUBSURFACE_LINK_LIBRARIES ${SUBSURFACE_LINK_LIBRARIES} -lwsock32)
	remove_definitions(-DUNICODE)
	add_definitions(-mwindows)
endif()

# include translations
add_subdirectory(translations)

# compile the core library, in C.
set(SUBSURFACE_CORE_LIB_SRCS
	cochran.c
	datatrak.c
	deco.c
	device.c
	dive.c
	divesite.c
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
	checkcloudconnection.cpp
	windowtitleupdate.cpp
	divelogexportlogic.cpp
	qt-init.cpp
	${SUBSURFACE_PRINTING_SRCS}
	${PLATFORM_SRC}
)
source_group("Subsurface Core" FILES ${SUBSURFACE_CORE_LIB_SRCS})

if(FBSUPPORT)
	add_definitions(-DFBSUPPORT)
	set(SOCIALNETWORKS qt-ui/socialnetworks.cpp)
endif()

# the data models that will interface
# with the views.
set(SUBSURFACE_MODELS_LIB_SRCS
	qt-models/cleanertablemodel.cpp
	qt-models/cylindermodel.cpp
	qt-models/diveplannermodel.cpp
	qt-models/models.cpp
	qt-models/filtermodels.cpp
	qt-models/tankinfomodel.cpp
	qt-models/weigthsysteminfomodel.cpp
	qt-models/weightmodel.cpp
	qt-models/divecomputermodel.cpp
	qt-models/treemodel.cpp
	qt-models/tableprintmodel.cpp
	qt-models/yearlystatisticsmodel.cpp
	qt-models/divetripmodel.cpp
	qt-models/divecomputerextradatamodel.cpp
	qt-models/completionmodels.cpp
	qt-models/profileprintmodel.cpp
	qt-models/divepicturemodel.cpp
	qt-models/diveplotdatamodel.cpp
	qt-models/divelocationmodel.cpp
)
source_group("Subsurface Models" FILES ${SUBSURFACE_MODELS})

# the interface, in C++
set(SUBSURFACE_INTERFACE
	qt-ui/updatemanager.cpp
	qt-ui/about.cpp
	qt-ui/divecomputermanagementdialog.cpp
	qt-ui/divelistview.cpp
	qt-ui/diveplanner.cpp
	qt-ui/diveshareexportdialog.cpp
	qt-ui/downloadfromdivecomputer.cpp
	qt-ui/globe.cpp
	qt-ui/graphicsview-common.cpp
	qt-ui/kmessagewidget.cpp
	qt-ui/maintab.cpp
	qt-ui/mainwindow.cpp
	qt-ui/modeldelegates.cpp
	qt-ui/metrics.cpp
	qt-ui/notificationwidget.cpp
	qt-ui/preferences.cpp
	qt-ui/printdialog.cpp
	qt-ui/printoptions.cpp
	qt-ui/simplewidgets.cpp
	qt-ui/starwidget.cpp
	qt-ui/subsurfacewebservices.cpp
	qt-ui/tableview.cpp
	qt-ui/divelogimportdialog.cpp
	qt-ui/tagwidget.cpp
	qt-ui/groupedlineedit.cpp
	qt-ui/usermanual.cpp
	qt-ui/divelogexportdialog.cpp
	qt-ui/divepicturewidget.cpp
	qt-ui/usersurvey.cpp
	qt-ui/configuredivecomputerdialog.cpp
	qt-ui/undocommands.cpp
	qt-ui/locationinformation.cpp
	qt-ui/qtwaitingspinner.cpp
	${SOCIALNETWORKS}
)
source_group("Subsurface Interface" FILES ${SUBSURFACE_INTERFACE})

# the profile widget
set(SUBSURFACE_PROFILE_LIB_SRCS
	qt-ui/profile/profilewidget2.cpp
	qt-ui/profile/diverectitem.cpp
	qt-ui/profile/divepixmapitem.cpp
	qt-ui/profile/divelineitem.cpp
	qt-ui/profile/divetextitem.cpp
	qt-ui/profile/animationfunctions.cpp
	qt-ui/profile/divecartesianaxis.cpp
	qt-ui/profile/diveprofileitem.cpp
	qt-ui/profile/diveeventitem.cpp
	qt-ui/profile/divetooltipitem.cpp
	qt-ui/profile/ruleritem.cpp
	qt-ui/profile/tankitem.cpp
)
source_group("Subsurface Profile" FILES ${SUBSURFACE_PROFILE_LIB_SRCS})

# the yearly statistics widget.
set(SUBSURFACE_STATISTICS_LIB_SRCS
	qt-ui/statistics/statisticswidget.cpp
	qt-ui/statistics/yearstatistics.cpp
	qt-ui/statistics/statisticsbar.cpp
	qt-ui/statistics/monthstatistics.cpp
)
source_group("Subsurface Statistics" FILES ${SUBSURFACE_STATISTICS_LIB_SRCS})

# the main app.
set(SUBSURFACE_APP
	main.cpp
	qt-gui.cpp
	qthelper.cpp
)
source_group("Subsurface App" FILES ${SUBSURFACE_APP})

# create the libraries
file(GLOB SUBSURFACE_UI qt-ui/*.ui)
qt5_wrap_ui(SUBSURFACE_UI_HDRS ${SUBSURFACE_UI})
qt5_add_resources(SUBSURFACE_RESOURCES subsurface.qrc)
source_group("Subsurface Interface Files" FILES ${SUBSURFACE_UI})

add_library(subsurface_corelib STATIC ${SUBSURFACE_CORE_LIB_SRCS} )
target_link_libraries(subsurface_corelib ${QT_LIBRARIES})
add_library(subsurface_models STATIC ${SUBSURFACE_MODELS_LIB_SRCS})
target_link_libraries(subsurface_models ${QT_LIBRARIES})
add_library(subsurface_profile STATIC ${SUBSURFACE_PROFILE_LIB_SRCS})
target_link_libraries(subsurface_profile ${QT_LIBRARIES})
add_library(subsurface_statistics STATIC ${SUBSURFACE_STATISTICS_LIB_SRCS})
target_link_libraries(subsurface_statistics ${QT_LIBRARIES})
add_library(subsurface_generated_ui STATIC ${SUBSURFACE_UI_HDRS})
target_link_libraries(subsurface_generated_ui ${QT_LIBRARIES})
add_library(subsurface_interface STATIC ${SUBSURFACE_INTERFACE})
target_link_libraries(subsurface_interface ${QT_LIBRARIES} ${MARBLE_LIBRARIES})

# add pthread to the end of the library list on Linux
# this is only needed on Ubuntu (why do these idiots break everything?)
# but shouldn't hurt on other Linux versions
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(SUBSURFACE_LINK_LIBRARIES ${SUBSURFACE_LINK_LIBRARIES} -lpthread)
endif()

# create the executables
add_executable(${SUBSURFACE_TARGET} MACOSX_BUNDLE WIN32 ${SUBSURFACE_PKG} ${SUBSURFACE_APP} ${SUBSURFACE_RESOURCES})
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
add_dependencies(subsurface_profile subsurface_generated_ui)
add_dependencies(subsurface_interface subsurface_generated_ui)
add_dependencies(subsurface_generated_ui version)
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
add_executable(export-html EXCLUDE_FROM_ALL export-html.cpp qt-init.cpp qthelper.cpp ${SUBSURFACE_RESOURCES})
target_link_libraries(export-html subsurface_corelib ${SUBSURFACE_LINK_LIBRARIES})

# QTest based tests
macro(TEST NAME FILE)
	add_executable(${NAME} EXCLUDE_FROM_ALL tests/${FILE} ${SUBSURFACE_RESOURCES})
	target_link_libraries(${NAME} subsurface_corelib ${QT_TEST_LIBRARIES} ${SUBSURFACE_LINK_LIBRARIES})
	add_test(NAME ${NAME}_build COMMAND "${CMAKE_COMMAND}" --build ${CMAKE_BINARY_DIR} --target ${NAME})
	add_test(NAME ${NAME}_run COMMAND ${NAME})
	set_tests_properties(${NAME}_run PROPERTIES DEPENDS ${NAME}_build)
endmacro()

add_definitions(-DSUBSURFACE_SOURCE="${CMAKE_SOURCE_DIR}")
add_definitions(-g)
if(NOT NO_TESTS)
	enable_testing()
	TEST(TestUnitConversion testunitconversion.cpp)
	TEST(TestProfile testprofile.cpp)
	TEST(TestGpsCoords testgpscoords.cpp)
	TEST(TestParse testparse.cpp)
endif()

if(NOT NO_DOCS)
	add_custom_target(
		documentation ALL
		mkdir -p ${CMAKE_BINARY_DIR}/Documentation/ \\;
		cp -a ${CMAKE_SOURCE_DIR}/Documentation/images ${CMAKE_BINARY_DIR}/Documentation/ \\;
		make -C ${CMAKE_SOURCE_DIR}/Documentation OUT=${CMAKE_BINARY_DIR}/Documentation/ doc
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
	install(DIRECTORY marbledata/maps DESTINATION ${RESOURCEDIR}/data)
	install(DIRECTORY marbledata/bitmaps DESTINATION ${RESOURCEDIR}/data)
	install(DIRECTORY Documentation/images DESTINATION ${RESOURCEDIR}/share/Documentation)
	install(FILES ${DOCFILES} DESTINATION ${RESOURCEDIR}/share/Documentation)
	install(DIRECTORY theme DESTINATION ${RESOURCEDIR})
	install(DIRECTORY printing_templates DESTINATION ${RESOURCEDIR})
	install(FILES ${TRANSLATIONS} DESTINATION ${RESOURCEDIR}/translations)
	install(FILES ${QTTRANSLATIONS} DESTINATION ${RESOURCEDIR}/translations)
	install(FILES ${CMAKE_SOURCE_DIR}/gpl-2.0.txt DESTINATION ${RESOURCEDIR})
	# this is a hack - but I don't know how else to find the macdeployqt program if it's not in the PATH
	string(REPLACE moc macdeployqt MACDEPLOYQT ${QT_MOC_EXECUTABLE})
	install(CODE "execute_process(COMMAND ${MACDEPLOYQT} Subsurface.app)")
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

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
	# Android template directory
	set(ANDROID_PACKAGE_SOURCE_DIR, ${CMAKE_BINARY_DIR}/android)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	install(DIRECTORY marbledata/maps DESTINATION share/subsurface/data)
	install(DIRECTORY marbledata/bitmaps DESTINATION share/subsurface/data)
	install(FILES subsurface.desktop DESTINATION share/applications)
	install(FILES subsurface-icon.svg DESTINATION share/icons/hicolor/scalable/apps)
	install(DIRECTORY Documentation/images DESTINATION share/subsurface/Documentation)
	install(FILES ${DOCFILES} DESTINATION share/subsurface/Documentation)
	install(DIRECTORY theme DESTINATION share/subsurface)
	install(DIRECTORY printing_templates DESTINATION share/subsurface)
	install(FILES ${TRANSLATIONS} DESTINATION share/subsurface/translations)
	install(TARGETS ${SUBSURFACE_TARGET} DESTINATION bin)
	if(DEFINED LIBMARBLEDEVEL)
		install(
			CODE "file(GLOB SSRFMARBLE_SHLIBS \"${LIBMARBLEDEVEL}/lib/libssrfmarblewidget.so*\")"
			CODE "file(INSTALL \${SSRFMARBLE_SHLIBS} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)"
		)
	endif()
endif()
