message(STATUS "processing version.cmake")

# AI-generated (Claude)
# On GitHub Actions a failing get-version must fail the build instead of
# carrying on with an empty version string, which is what happens when the
# guard in get-version.sh is enforced and trips. Elsewhere (local builds,
# packaging environments) the behaviour is unchanged.
function(check_get_version_result RESULT)
	if(NOT "${RESULT}" STREQUAL "0" AND "$ENV{GITHUB_ACTIONS}" STREQUAL "true")
		message(FATAL_ERROR "get-version failed (${RESULT}) in a GitHub Actions build, see the output above")
	endif()
endfunction()

if(DEFINED ENV{CANONICALVERSION})
       set(CANONICAL_VERSION_STRING $ENV{CANONICALVERSION})
else()
	if(USING_MSVC)
		# Use PowerShell on Windows with MSVC
		execute_process(
			COMMAND powershell -ExecutionPolicy Bypass -File ${CMAKE_TOP_SRC_DIR}/../scripts/get-version.ps1
			WORKING_DIRECTORY ${CMAKE_TOP_SRC_DIR}
			OUTPUT_VARIABLE CANONICAL_VERSION_STRING
			RESULT_VARIABLE GET_VERSION_RESULT
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)
		check_get_version_result("${GET_VERSION_RESULT}")
	else()
		execute_process(
			COMMAND bash ${CMAKE_TOP_SRC_DIR}/../scripts/get-version.sh
			WORKING_DIRECTORY ${CMAKE_TOP_SRC_DIR}
			OUTPUT_VARIABLE CANONICAL_VERSION_STRING
			RESULT_VARIABLE GET_VERSION_RESULT
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)
		check_get_version_result("${GET_VERSION_RESULT}")
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
			RESULT_VARIABLE GET_VERSION_RESULT
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)
		check_get_version_result("${GET_VERSION_RESULT}")
	else()
		execute_process(
			COMMAND bash ${CMAKE_TOP_SRC_DIR}/../scripts/get-version.sh 4
			WORKING_DIRECTORY ${CMAKE_TOP_SRC_DIR}
			OUTPUT_VARIABLE CANONICAL_VERSION_STRING_4
			RESULT_VARIABLE GET_VERSION_RESULT
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)
		check_get_version_result("${GET_VERSION_RESULT}")
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
