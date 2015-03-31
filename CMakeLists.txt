# cmake based build of Subsurface

project(Subsurface)
cmake_minimum_required(VERSION 2.8.11)

# global settings

SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
SET(CMAKE_MODULE_PATH ${${PROJECT_NAME}_SOURCE_DIR}/marbledata)
INCLUDE_DIRECTORIES( . ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_BINARY_DIR} qt-ui qt-ui/profile)

# compiler specific settings

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUXX)
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 ")
endif()

# pkgconfig for required libraries

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
pkg_config_library(LIBXSLT libxslt)
pkg_config_library(LIBZIP libzip)

# more libraries with special handling in case we build them ourselves

if(NOT DEFINED LIBGIT2DEVEL)
	pkg_config_library(LIBGIT2 libgit2)
ELSE()
	include_directories(${LIBGIT2DEVEL}/include)
	link_directories(${LIBGIT2DEVEL}/build)
	if(NOT DEFINED LIBGIT2STATIC)
		set(SUBSURFACE_LINK_LIBRARIES ${SUBSURFACE_LINK_LIBRARIES} -lgit2 -lssl -lcrypto)
	ELSE()
		set(SUBSURFACE_LINK_LIBRARIES ${SUBSURFACE_LINK_LIBRARIES} libgit2.a -lssl -lcrypto)
	ENDIF()
ENDIF()

SET(LIBDCDEVEL "" CACHE STRING "libraries")
IF(DEFINED LIBDCDEVEL)
	cmake_policy(SET CMP0015 OLD)
	include_directories(${LIBDCDEVEL}/include )
	link_directories(${LIBDCDEVEL}/src/.libs)
	if(NOT DEFINED LIBDCSTATIC)
		set(SUBSURFACE_LINK_LIBRARIES ${SUBSURFACE_LINK_LIBRARIES} -ldivecomputer -lusb-1.0)
	ELSE()
		set(SUBSURFACE_LINK_LIBRARIES ${SUBSURFACE_LINK_LIBRARIES} libdivecomputer.a -lusb-1.0)
	ENDIF()
ELSE()
	set(SUBSURFACE_LINK_LIBRARIES ${SUBSURFACE_LINK_LIBRARIES} -ldivecomputer)
ENDIF()

IF(NOT DEFINED LIBMARBLEDEVEL)
	FIND_PACKAGE(Marble REQUIRED)
	include_directories(${MARBLE_INCLUDE_DIR})
	link_directories(${MARBLE_LIB_DIR})
ELSE()
	include_directories(${LIBMARBLEDEVEL}/include)
	link_directories(${LIBMARBLEDEVEL}/lib)
	SET(MARBLE_LIBRARIES -L${LIBMARBLEDEVEL}/lib -lssrfmarblewidget)
ENDIF()

# handle out of tree build correctly

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

# set up the different target platforms

SET(PLATFORM_SRC unknown_platform.c)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	SET(PLATFORM_SRC linux.c)
	if(NOT DEFINED LRELEASE)
		set(LRELEASE lrelease)
	endif()
ENDIF()
IF(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	SET(PLATFORM_SRC macos.c)
	set(SUBSURFACE_LINK_LIBRARIES ${SUBSURFACE_LINK_LIBRARIES} -framework CoreServices)
	if(NOT DEFINED LRELEASE)
		set(LRELEASE lrelease)
	endif()
ENDIF()
IF(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	SET(PLATFORM_SRC windows.c)
	set(SUBSURFACE_LINK_LIBRARIES ${SUBSURFACE_LINK_LIBRARIES} -lwsock32)
	remove_definitions(-DUNICODE)
	if(NOT DEFINED LRELEASE)
		set(LRELEASE lrelease.exe)
	endif()
ENDIF()

# compile the core library, in C.

SET(SUBSURFACE_CORE_LIB_SRCS
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
	divesitehelpers.cpp
	${PLATFORM_SRC}
)

# the interface, in C++

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
	qt-ui/notificationwidget.cpp
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

# the profile widget

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

# the yearly statistics widget.

SET(SUBSURFACE_STATISTICS_LIB_SRCS
	qt-ui/statistics/statisticswidget.cpp
	qt-ui/statistics/yearstatistics.cpp
	qt-ui/statistics/statisticsbar.cpp
	qt-ui/statistics/monthstatistics.cpp
)

# the main app.

SET(SUBSURFACE_APP
	main.cpp
	qt-gui.cpp
	qthelper.cpp
)

# create the libraries

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

# create the executables

ADD_EXECUTABLE(subsurface ${SUBSURFACE_APP} ${SUBSURFACE_RESOURCES})
target_link_libraries( subsurface
  subsurface_generated_ui
  subsurface_interface
  subsurface_profile
  subsurface_statistics
  subsurface_corelib
  ${SUBSURFACE_LINK_LIBRARIES}
)

ADD_DEPENDENCIES(subsurface_statistics subsurface_generated_ui)
ADD_DEPENDENCIES(subsurface_profile subsurface_generated_ui)
ADD_DEPENDENCIES(subsurface_interface subsurface_generated_ui)
ADD_DEPENDENCIES(subsurface_generated_ui version)
ADD_DEPENDENCIES(subsurface_corelib version)

# QTest based tests

MACRO(test NAME FILE)
    ADD_EXECUTABLE(${NAME} tests/${FILE} ${SUBSURFACE_RESOURCES})
    TARGET_LINK_LIBRARIES(${NAME} subsurface_corelib ${QT_TEST_LIBRARIES}  ${SUBSURFACE_LINK_LIBRARIES})
    ADD_TEST(NAME ${NAME} COMMAND ${NAME})
ENDMACRO()

ENABLE_TESTING()
ADD_DEFINITIONS(-DSUBSURFACE_SOURCE="${CMAKE_SOURCE_DIR}")
ADD_DEFINITIONS(-g)
test(TestUnitConversion testunitconversion.cpp)
test(TestProfile testprofile.cpp)
test(TestGpsCoords testgpscoords.cpp)
test(TestParse testparse.cpp)

ADD_CUSTOM_TARGET(documentation ALL mkdir -p ${CMAKE_BINARY_DIR}/Documentation/ \\; make -C ${CMAKE_SOURCE_DIR}/Documentation OUT=${CMAKE_BINARY_DIR}/Documentation/ doc)

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

set(TRANSLATION_SRC
	translations/subsurface_source.ts
	translations/subsurface_bg_BG.ts
	translations/subsurface_cs.ts
	translations/subsurface_da_DK.ts
	translations/subsurface_de_CH.ts
	translations/subsurface_de_DE.ts
	translations/subsurface_en_GB.ts
	translations/subsurface_es_ES.ts
	translations/subsurface_et_EE.ts
	translations/subsurface_fi_FI.ts
	translations/subsurface_fr_FR.ts
	translations/subsurface_it_IT.ts
	translations/subsurface_lv_LV.ts
	translations/subsurface_nb_NO.ts
	translations/subsurface_nl_NL.ts
	translations/subsurface_pl_PL.ts
	translations/subsurface_pt_BR.ts
	translations/subsurface_pt_PT.ts
	translations/subsurface_ru_RU.ts
	translations/subsurface_sk_SK.ts
	translations/subsurface_sv_SE.ts
	translations/subsurface_tr.ts
	translations/subsurface_zh_TW.ts
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

# disabled translations as they are below 50%:
# translations/subsurface_el_GR.ts \
# translations/subsurface_he.ts \
# translations/subsurface_hu.ts \
# translations/subsurface_ro_RO.ts \

# if we apply the REGEX to TRANSLATION_SRC then the list of files turns
# into a single string, so we assemble it file name by file name
foreach(TRANSLATION ${TRANSLATION_SRC})
	string(REGEX REPLACE \\.ts .qm TRANSLATION_QM ${TRANSLATION})
	string(REGEX REPLACE "/" "-" TRANSLATION_TARGET ${TRANSLATION_QM})
	MESSAGE(STATUS ${TRANSLATION} "->" ${CMAKE_BINARY_DIR}/${TRANSLATION_QM})
	ADD_CUSTOM_COMMAND(
		OUTPUT ${CMAKE_BINARY_DIR}/${TRANSLATION_QM}
		COMMAND mkdir -p ${CMAKE_BINARY_DIR}/translations \\; ${LRELEASE} ${CMAKE_SOURCE_DIR}/${TRANSLATION} -qm ${CMAKE_BINARY_DIR}/${TRANSLATION_QM}
		DEPENDS ${TRANSLATION}
		)
	ADD_CUSTOM_TARGET(
		generate_translations_${TRANSLATION_TARGET}
		DEPENDS ${CMAKE_BINARY_DIR}/${TRANSLATION_QM}
		)
	ADD_DEPENDENCIES(subsurface generate_translations_${TRANSLATION_TARGET})
	set(TRANSLATIONS ${TRANSLATIONS} ${CMAKE_BINARY_DIR}/${TRANSLATION_QM})
endforeach()

foreach(QTTRANSLATION ${QTTRANSLATIONS_BASE})
	set(QTTRANSLATIONS ${QTTRANSLATIONS} ${QT_TRANSLATION_DIR}/${QTTRANSLATION})
endforeach()

# now for each platform the install instructions

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
#	# OS X bundling rules
#	#  "make mac-deploy" deploys the external libs (Qt, libdivecomputer, libusb, etc.) into the bundle
#	#  "make install" installs the bundle to /Applications
#	#  "make mac-create-dmg" creates Subsurface.dmg
ENDIF()

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
#	# Windows bundling rules
#	# We don't have a helpful tool like macdeployqt for Windows, so we hardcode
#	# which libs we need.
#	# "make install", copies everything into a staging area
#	# "make installer", uses makensis to create an installer executable
ENDIF()

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
#	# Android template directory
#	SET(ANDROID_PACKAGE_SOURCE_DIR, ${CMAKE_BINARY_DIR}/android)
ENDIF()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	install(DIRECTORY marbledata/maps DESTINATION share/subsurface/data)
	install(DIRECTORY marbledata/bitmaps DESTINATION share/subsurface/data)
	install(FILES subsurface.desktop DESTINATION share/applications)
	install(FILES subsurface-icon.svg DESTINATION share/icons/hicolor/scalable/apps)
	install(DIRECTORY Documentation/images DESTINATION share/subsurface/Documentation)
	install(FILES ${DOCFILES} DESTINATION share/subsurface/Documentation)
	install(DIRECTORY theme DESTINATION share/subsurface)
	install(FILES ${TRANSLATIONS} DESTINATION share/subsurface/translations)
	install(TARGETS subsurface DESTINATION bin)
ENDIF()
