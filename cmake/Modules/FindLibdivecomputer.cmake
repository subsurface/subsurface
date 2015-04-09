# - Try to find the LIBDIVECOMPUTER Library
# Once done this will define
#
#  LIBDIVECOMPUTER_FOUND - system has LIBDIVECOMPUTER
#  LIBDIVECOMPUTER_INCLUDE_DIR - the LIBDIVECOMPUTER include directory
#  LIBDIVECOMPUTER_LIBRARIES
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

# in cache already
IF ( LIBDIVECOMPUTER_INCLUDE_DIR AND LIBDIVECOMPUTER_LIBRARIES )
   SET( LIBDIVECOMPUTER_FIND_QUIETLY TRUE )
ENDIF ( LIBDIVECOMPUTER_INCLUDE_DIR AND LIBDIVECOMPUTER_LIBRARIES )

FIND_PATH( LIBDIVECOMPUTER_INCLUDE_DIR
NAMES libdivecomputer/hw.h
HINTS
    ${CMAKE_CURRENT_SOURCE_DIR}/../install-root/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../libdivecomputer/include/
    /usr/local/include
    /usr/include
)

FIND_LIBRARY( LIBDIVECOMPUTER_LIBRARIES
NAMES
    libdivecomputer.a
    divecomputer
HINTS
    ${CMAKE_CURRENT_SOURCE_DIR}/../install-root/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/../libdivecomputer/src/.libs/
    /usr/local/include
    /usr/include
)

INCLUDE( FindPackageHandleStandardArgs )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( Libdivecomputer DEFAULT_MSG LIBDIVECOMPUTER_INCLUDE_DIR LIBDIVECOMPUTER_LIBRARIES )
