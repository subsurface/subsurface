# HandleFindQLiteHtml.cmake
# Find QLiteHtml library for Qt6 user manual rendering

# QLiteHtml doesn't provide pkg-config, so we manually search for it
# Search in CMAKE_PREFIX_PATH directories as well as standard locations
find_path(QLITEHTML_INCLUDE_DIR
	NAMES qlitehtmlwidget.h
	PATH_SUFFIXES qlitehtml include/qlitehtml
	HINTS ${CMAKE_PREFIX_PATH}
	PATHS
		${CMAKE_INSTALL_PREFIX}/include/qlitehtml
		/usr/local/include/qlitehtml
		/usr/include/qlitehtml
	DOC "QLiteHtml include directory"
)

find_path(QLITEHTML_LITEHTML_INCLUDE_DIR
	NAMES litehtml/master_css.h
	# qlitehtml's own "make install" places litehtml/master_css.h directly
	# next to qlitehtmlwidget.h, so look there first before falling back to
	# other layouts (e.g. a system package installing litehtml separately).
	HINTS
		${QLITEHTML_INCLUDE_DIR}
		${CMAKE_PREFIX_PATH}
		${CMAKE_SOURCE_DIR}/qlitehtml
	PATH_SUFFIXES include include/qlitehtml src/3rdparty/litehtml/include
	PATHS
		${CMAKE_INSTALL_PREFIX}/include
		/usr/local/include
		/usr/include
	DOC "litehtml include directory used by QLiteHtml"
)

find_library(QLITEHTML_LIBRARY
	NAMES qlitehtml qlitehtml1 libqlitehtml
	# PATH_SUFFIXES (unlike HINTS/PATHS alone) is applied to every search
	# directory, so this also covers a CMAKE_PREFIX_PATH entry that installed
	# into <prefix>/lib64 rather than <prefix>/lib.
	HINTS ${CMAKE_PREFIX_PATH}
	PATH_SUFFIXES lib lib64
	PATHS
		${CMAKE_INSTALL_PREFIX}
		/usr/local
		/usr
	DOC "QLiteHtml library"
)

if(QLITEHTML_INCLUDE_DIR AND QLITEHTML_LITEHTML_INCLUDE_DIR AND QLITEHTML_LIBRARY)
	set(QLITEHTML_FOUND TRUE)
	set(QLITEHTML_LIBRARIES ${QLITEHTML_LIBRARY})
	message(STATUS "QLiteHtml found: ${QLITEHTML_LIBRARY}")
	message(STATUS "litehtml headers found: ${QLITEHTML_LITEHTML_INCLUDE_DIR}")
	include_directories(${QLITEHTML_INCLUDE_DIR} ${QLITEHTML_LITEHTML_INCLUDE_DIR})
else()
	set(QLITEHTML_FOUND FALSE)
endif()
