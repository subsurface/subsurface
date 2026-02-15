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

find_library(QLITEHTML_LIBRARY
	NAMES qlitehtml qlitehtml1 libqlitehtml
	HINTS ${CMAKE_PREFIX_PATH}
	PATHS
		${CMAKE_INSTALL_PREFIX}/lib
		/usr/local/lib
		/usr/lib
	DOC "QLiteHtml library"
)

if(QLITEHTML_INCLUDE_DIR AND QLITEHTML_LIBRARY)
	set(QLITEHTML_FOUND TRUE)
	set(QLITEHTML_LIBRARIES ${QLITEHTML_LIBRARY})
	message(STATUS "QLiteHtml found: ${QLITEHTML_LIBRARY}")
	include_directories(${QLITEHTML_INCLUDE_DIR})
else()
	set(QLITEHTML_FOUND FALSE)
endif()
