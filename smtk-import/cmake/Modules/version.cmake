message(STATUS "processing version.cmake")

if(DEFINED ENV{CANONICALVERSION})
       set(CANONICAL_VERSION_STRING $ENV{CANONICALVERSION})
else()
	if(USING_MSVC)
		# Use PowerShell on Windows with MSVC
		execute_process(
			COMMAND powershell -ExecutionPolicy Bypass -File ${CMAKE_TOP_SRC_DIR}/../scripts/get-version.ps1
			WORKING_DIRECTORY ${CMAKE_TOP_SRC_DIR}
			OUTPUT_VARIABLE CANONICAL_VERSION_STRING
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)
	else()
		execute_process(
			COMMAND bash ${CMAKE_TOP_SRC_DIR}/../scripts/get-version.sh
			WORKING_DIRECTORY ${CMAKE_TOP_SRC_DIR}
			OUTPUT_VARIABLE CANONICAL_VERSION_STRING
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)
	endif()
endif()

if(DEFINED ENV{CANONICALVERSION_4})
       set(CANONICAL_VERSION_STRING_4 $ENV{CANONICALVERSION_4})
else()
	if(USING_MSVC)
		# Use PowerShell on Windows with MSVC
		execute_process(
			COMMAND powershell -ExecutionPolicy Bypass -File ${CMAKE_TOP_SRC_DIR}/../scripts/get-version.ps1 4
			WORKING_DIRECTORY ${CMAKE_TOP_SRC_DIR}
			OUTPUT_VARIABLE CANONICAL_VERSION_STRING_4
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)
	else()
		execute_process(
			COMMAND bash ${CMAKE_TOP_SRC_DIR}/../scripts/get-version.sh 4
			WORKING_DIRECTORY ${CMAKE_TOP_SRC_DIR}
			OUTPUT_VARIABLE CANONICAL_VERSION_STRING_4
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)
	endif()
endif()

configure_file(${SRC} ${DST} @ONLY)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# Read the template file
	file(READ ${CMAKE_TOP_SRC_DIR}/../packaging/windows/smtk-import.nsi.in NSI_CONTENT)
	# Replace version tokens
	string(REPLACE "VERSIONTOKEN" "${CANONICAL_VERSION_STRING}" NSI_CONTENT "${NSI_CONTENT}")
	string(REPLACE "PRODVTOKEN" "${CANONICAL_VERSION_STRING_4}" NSI_CONTENT "${NSI_CONTENT}")
	# Write the output file
	file(WRITE ${CMAKE_BINARY_DIR}/staging/smtk-import.nsi "${NSI_CONTENT}")
endif()
