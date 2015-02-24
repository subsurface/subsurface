project(Subsurface)
cmake_minimum_required(VERSION 2.8.11)

SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
SET(CMAKE_MODULE_PATH ${${PROJECT_NAME}_SOURCE_DIR}/marbledata)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUXX)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 ")
endif()

INCLUDE_DIRECTORIES( . ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_BINARY_DIR} qt-ui qt-ui/profile)
FIND_PACKAGE(PkgConfig)

MACRO(pkg_config_library LIBNAME pcfile)
	pkg_check_modules(${LIBNAME} REQUIRED ${pcfile})
	include_directories(${${LIBNAME}_INCLUDE_DIRS})
	link_directories(${${LIBNAME}_LIBRARY_DIRS})
	add_definitions(${${LIBNAME}_CFLAGS_OTHER})
	set(SUBSURFACE_LINK_LIBRARIES ${SUBSURFACE_LINK_LIBRARIES} ${${LIBNAME}_LIBRARIES})
ENDMACRO()

pkg_config_library(LIBXML libxml-2.0)
pkg_config_library(LIBSQLITE3 sqlite3)
if(NOT DEFINED LIBGIT2DEVEL)
	pkg_config_library(LIBGIT2 libgit2)
ELSE()
	include_directories(${LIBGIT2DEVEL}/include)
	link_directories(${LIBGIT2DEVEL}/build)
	if(LIBGIT2STATIC STREQUAL "")
		set(SUBSURFACE_LINK_LIBRARIES ${SUBSURFACE_LINK_LIBRARIES} libgit2.so libssl.so libcrypto.so)
	ELSE()
		set(SUBSURFACE_LINK_LIBRARIES ${SUBSURFACE_LINK_LIBRARIES} libgit2.a libssl.so libcrypto.so)
	ENDIF()
ENDIF()
pkg_config_library(LIBXSLT libxslt)

SET(LIBDCDEVEL "" CACHE STRING "libraries")
IF(NOT (LIBDCDEVEL STREQUAL ""))
	cmake_policy(SET CMP0015 OLD)
	include_directories(${LIBDCDEVEL}/include )
	link_directories(${LIBDCDEVEL}/src/.libs)
ENDIF()

STRING(COMPARE EQUAL "${${PROJECT_NAME}_SOURCE_DIR}" "${${PROJECT_NAME}_BINARY_DIR}" insource)
GET_FILENAME_COMPONENT(PARENTDIR ${${PROJECT_NAME}_SOURCE_DIR} PATH)
STRING(COMPARE EQUAL "${${PROJECT_NAME}_SOURCE_DIR}" "${PARENTDIR}" insourcesubdir)
IF(NOT (insource OR insourcedir))
	add_custom_target(link_marble_data ALL COMMAND rm -f marbledata && ln -s ${${PROJECT_NAME}_SOURCE_DIR}/marbledata ${${PROJECT_NAME}_BINARY_DIR}/marbledata)
ENDIF()

#configure Qt.
FIND_PACKAGE(Qt5Core REQUIRED)
FIND_PACKAGE(Qt5Concurrent REQUIRED)
FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5Network REQUIRED)
FIND_PACKAGE(Qt5WebKitWidgets REQUIRED)
FIND_PACKAGE(Qt5PrintSupport REQUIRED)
FIND_PACKAGE(Qt5Svg REQUIRED)
FIND_PACKAGE(Qt5Test REQUIRED)
SET(QT_LIBRARIES Qt5::Core Qt5::Concurrent Qt5::Widgets Qt5::Network Qt5::WebKitWidgets Qt5::PrintSupport Qt5::Svg)
SET(QT_TEST_LIBRARIES ${QT_LIBRARIES} Qt5::Test)

IF(NOT DEFINED LIBMARBLEDEVEL)
	FIND_PACKAGE(Marble REQUIRED)
	INCLUDE_DIRECTORIES(${MARBLE_INCLUDE_DIR})
ELSE()
	INCLUDE_DIRECTORIES(${LIBMARBLEDEVEL}/include)
	SET(MARBLE_LIBRARIES ${LIBMARBLEDEVEL}/lib/libssrfmarblewidget.so)
ENDIF()

# Generate the ssrf-config.h every 'make'
FILE(WRITE ${CMAKE_BINARY_DIR}/version.h.in "
    #define VERSION_STRING \"@VERSION_STRING@\"
    #define GIT_VERSION_STRING \"@GIT_VERSION_STRING@\"
    #define CANONICAL_VERSION_STRING \"@CANONICAL_VERSION_STRING@\"
")
FILE(WRITE ${CMAKE_BINARY_DIR}/version.cmake "
    IF (\${APPLE})
        SET(VER_OS darwin)
    ELSEIF (\${WIN32})
        SET(VER_OS win)
    ELSE ()
        SET(VER_OS linux)
    ENDIF ()
    EXECUTE_PROCESS(
        COMMAND sh scripts/get-version \${VER_OS}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE VERSION_STRING
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    EXECUTE_PROCESS(
        COMMAND sh scripts/get-version linux
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_VERSION_STRING
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    EXECUTE_PROCESS(
        COMMAND sh scripts/get-version full
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE CANONICAL_VERSION_STRING
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    CONFIGURE_FILE(\${SRC} \${DST} @ONLY)
")
ADD_CUSTOM_TARGET(version ALL COMMAND
    ${CMAKE_COMMAND} -D SRC=${CMAKE_BINARY_DIR}/version.h.in
                     -D DST=${CMAKE_BINARY_DIR}/ssrf-version.h
                     -P ${CMAKE_BINARY_DIR}/version.cmake
)

# compile the core library, in C.
SET(SUBSURFACE_CORE_LIB_SRCS
	cochran.c
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
	linux.c
	#gettextfrommoc should be added because we are using it on the c-code.
	gettextfromc.cpp
	#dirk ported some core functionality to c++.
	qthelper.cpp
	divecomputer.cpp
	exif.cpp
	subsurfacesysinfo.cpp
	devicedetails.cpp
	configuredivecomputer.cpp
	configuredivecomputerthreads.cpp
)

#the interface, in C++
SET(SUBSURFACE_INTERFACE
	qt-ui/updatemanager.cpp
	qt-ui/about.cpp
	qt-ui/completionmodels.cpp
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
	qt-ui/models.cpp
	qt-ui/metrics.cpp
	qt-ui/preferences.cpp
	qt-ui/printdialog.cpp
	qt-ui/printlayout.cpp
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
	qt-ui/filtermodels.cpp
	qt-ui/undocommands.cpp
)

#the profile widget
SET(SUBSURFACE_PROFILE_LIB_SRCS
	qt-ui/profile/profilewidget2.cpp
	qt-ui/profile/diverectitem.cpp
	qt-ui/profile/divepixmapitem.cpp
	qt-ui/profile/divelineitem.cpp
	qt-ui/profile/divetextitem.cpp
	qt-ui/profile/animationfunctions.cpp
	qt-ui/profile/divecartesianaxis.cpp
	qt-ui/profile/diveplotdatamodel.cpp
	qt-ui/profile/diveprofileitem.cpp
	qt-ui/profile/diveeventitem.cpp
	qt-ui/profile/divetooltipitem.cpp
	qt-ui/profile/ruleritem.cpp
	qt-ui/profile/tankitem.cpp
)

#the yearly statistics widget.
SET(SUBSURFACE_STATISTICS_LIB_SRCS
	qt-ui/statistics/statisticswidget.cpp
	qt-ui/statistics/yearstatistics.cpp
	qt-ui/statistics/statisticsbar.cpp
	qt-ui/statistics/monthstatistics.cpp
)

#the main app.
SET(SUBSURFACE_APP
	main.cpp
	qt-gui.cpp
	qthelper.cpp
)

FILE(GLOB SUBSURFACE_UI qt-ui/*.ui)
QT5_WRAP_UI(SUBSURFACE_UI_HDRS ${SUBSURFACE_UI})
QT5_ADD_RESOURCES(SUBSURFACE_RESOURCES subsurface.qrc)

ADD_LIBRARY(subsurface_corelib STATIC ${SUBSURFACE_CORE_LIB_SRCS} )
TARGET_LINK_LIBRARIES(subsurface_corelib ${QT_LIBRARIES})
ADD_LIBRARY(subsurface_profile STATIC ${SUBSURFACE_PROFILE_LIB_SRCS})
TARGET_LINK_LIBRARIES(subsurface_profile ${QT_LIBRARIES})
ADD_LIBRARY(subsurface_statistics STATIC ${SUBSURFACE_STATISTICS_LIB_SRCS})
TARGET_LINK_LIBRARIES(subsurface_statistics ${QT_LIBRARIES})
ADD_LIBRARY(subsurface_generated_ui STATIC ${SUBSURFACE_UI_HDRS})
TARGET_LINK_LIBRARIES(subsurface_generated_ui ${QT_LIBRARIES})
ADD_LIBRARY(subsurface_interface STATIC ${SUBSURFACE_INTERFACE})
TARGET_LINK_LIBRARIES(subsurface_interface ${QT_LIBRARIES} ${MARBLE_LIBRARIES})

ADD_EXECUTABLE(subsurface ${SUBSURFACE_APP} ${SUBSURFACE_RESOURCES})
target_link_libraries( subsurface
  subsurface_generated_ui
  subsurface_interface
  subsurface_profile
  subsurface_statistics
  subsurface_corelib
  ${SUBSURFACE_LINK_LIBRARIES}
  -ldivecomputer
  -lzip
)

ADD_DEPENDENCIES(subsurface_statistics subsurface_generated_ui)
ADD_DEPENDENCIES(subsurface_profile subsurface_generated_ui)
ADD_DEPENDENCIES(subsurface_interface subsurface_generated_ui)
ADD_DEPENDENCIES(subsurface_generated_ui version)
ADD_DEPENDENCIES(subsurface_corelib version)

MACRO(test NAME FILE)
    ADD_EXECUTABLE(${NAME} tests/${FILE})
    TARGET_LINK_LIBRARIES(${NAME} subsurface_corelib ${QT_TEST_LIBRARIES}  ${SUBSURFACE_LINK_LIBRARIES} -lzip -ldivecomputer)
    ADD_TEST(NAME ${NAME} COMMAND ${NAME})
ENDMACRO()

ENABLE_TESTING()
test(TestUnitConversion testunitconversion.cpp)
test(TestProfile testprofile.cpp)
test(TestGpsCoords testgpscoords.cpp)
