# - Try to find the LibGit2 Library
# Once done this will define
#
#  LIBGIT2_FOUND - system has LibGit2
#  LIBGIT2_INCLUDE_DIR - the LibGit2 include directory
#  LIBGIT2_LIBRARIES
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

# in cache already
IF ( LIBGIT2_INCLUDE_DIR AND LIBGIT2_LIBRARIES )
   SET( LIBGIT2_FIND_QUIETLY TRUE )
ENDIF ()

FIND_PATH( LIBGIT2_INCLUDE_DIR
NAMES git2.h
HINTS
    ${CMAKE_CURRENT_SOURCE_DIR}/../install-root/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../libgit2/include
    /usr/local/include
    /usr/include
)

FIND_LIBRARY( LIBGIT2_LIBRARIES
NAMES
    libgit2.a
    git2
HINTS
    ${CMAKE_CURRENT_SOURCE_DIR}/../install-root/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/../libgit2/build
    /usr/local/include
    /usr/include
)
SET(LIBGIT2_LIBRARIES ${LIBGIT2_LIBRARIES} -lssl -lcrypto)

INCLUDE( FindPackageHandleStandardArgs )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( git2 DEFAULT_MSG LIBGIT2_INCLUDE_DIR LIBGIT2_LIBRARIES )
include_directories(${LIBGIT2_INCLUDE_DIR}})