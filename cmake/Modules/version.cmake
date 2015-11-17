if(${APPLE})
	set(VER_OS darwin)
elseif(${WIN32})
	set(VER_OS win)
else()
	set(VER_OS linux)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set(VER_OS win)
endif()
execute_process(
	COMMAND sh scripts/get-version ${VER_OS}
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

configure_file(${SRC} ${DST} @ONLY)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	execute_process(
		COMMAND cat ${CMAKE_SOURCE_DIR}/packaging/windows/subsurface.nsi.in
		COMMAND sed -e \"s/VERSIONTOKEN/\${GIT_VERSION_STRING}/\"
		COMMAND sed -e \"s/PRODVTOKEN/\${CANONICAL_VERSION_STRING}/\"
		OUTPUT_FILE ${CMAKE_BINARY_DIR}/staging/subsurface.nsi
	)
endif()
