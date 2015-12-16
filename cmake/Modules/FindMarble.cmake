# - Try to find the Marble Library
# Once done this will define
#
#  MARBLE_FOUND - system has Marble
#  MARBLE_INCLUDE_DIR - the Marble include directory
#  MARBLE_LIBRARIES
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

# in cache already
IF ( MARBLE_INCLUDE_DIR AND MARBLE_LIBRARIES )
   SET( MARBLE_FIND_QUIETLY TRUE )
ENDIF ( MARBLE_INCLUDE_DIR AND MARBLE_LIBRARIES )

FIND_PATH( MARBLE_INCLUDE_DIR
NAMES marble/MarbleModel.h
HINTS
    ${CMAKE_CURRENT_SOURCE_DIR}/../install-root/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../marble/src/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/../marble-source/src/lib
    /usr/local/include
    /usr/include
)

FIND_LIBRARY( MARBLE_LIBRARIES
NAMES
    ssrfmarblewidget
HINTS
    ${CMAKE_CURRENT_SOURCE_DIR}/../install-root/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/../marble
    ${CMAKE_CURRENT_SOURCE_DIR}/../marble-source
    /usr/local/include
    /usr/include
)

INCLUDE( FindPackageHandleStandardArgs )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( marble DEFAULT_MSG MARBLE_INCLUDE_DIR MARBLE_LIBRARIES )
