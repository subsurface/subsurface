# HandleFindQLiteHtml.cmake
# Find QLiteHtml library for Qt6 user manual rendering

# QLiteHtml doesn't provide pkg-config, so we manually search for it
find_path(QLITEHTML_INCLUDE_DIR
	NAMES qlitehtmlwidget.h
	PATHS
		${CMAKE_INSTALL_PREFIX}/include/qlitehtml
		/usr/local/include/qlitehtml
		/usr/include/qlitehtml
	DOC "QLiteHtml include directory"
)

find_library(QLITEHTML_LIBRARY
	NAMES qlitehtml libqlitehtml
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
