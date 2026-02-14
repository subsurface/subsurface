# Generate the ssrf-config.h every 'make'
file(WRITE ${CMAKE_BINARY_DIR}/version.h.in
"#define CANONICAL_VERSION_STRING \"@CANONICAL_VERSION_STRING@\"
#define CANONICAL_VERSION_STRING_4 \"@CANONICAL_VERSION_STRING_4@\"
")

file(COPY cmake/Modules/version.cmake
    DESTINATION ${CMAKE_BINARY_DIR})

if(MSVC)
	set(USING_MSVC TRUE)
else()
	set(USING_MSVC FALSE)
endif()

add_custom_target(
	version ALL COMMAND ${CMAKE_COMMAND}
	-D SRC=${CMAKE_BINARY_DIR}/version.h.in
	-D DST=${CMAKE_BINARY_DIR}/ssrf-version.h
	-D CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}
	-D CMAKE_TOP_SRC_DIR=${CMAKE_SOURCE_DIR}
	-D CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}
	-D USING_MSVC=${USING_MSVC}
	-P ${CMAKE_BINARY_DIR}/version.cmake
)
