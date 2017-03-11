# QTest based tests
qt5_add_resources(SUBSURFACE_TEST_RESOURCES ../subsurface.qrc)

# Access test data (dive folder) from SUBSURFACE_SOURCE by default.
# In cross compilation cases or when test will not be executed at build time
# a differnt value can be set via cmake -DSUBSURFACE_TEST_DATA.
if(NOT SUBSURFACE_TEST_DATA)
	if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
		# For windows case we expect tests to be executed
		# with WORKING_DIRECTORY pointing to folder where test data can be found
		set(SUBSURFACE_TEST_DATA .)
	else()
		set(SUBSURFACE_TEST_DATA ${SUBSURFACE_SOURCE})
	endif()
endif()

add_library(RESOURCE_LIBRARY STATIC ${SUBSURFACE_TEST_RESOURCES})

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")

	# Prepare a staging_tests folder
	# Test can run accessing data and dependecies for build time testing
	# or can be deployed for target testing
	# It inludes:
	#  - test data
	#  - test binaries (see TEST macro)
	#  - test binaries dependencies (see TEST macro)
	set(WINDOWS_STAGING_TESTS ${CMAKE_BINARY_DIR}/staging_tests)
	install(DIRECTORY ${SUBSURFACE_SOURCE}/dives DESTINATION ${WINDOWS_STAGING_TESTS})
	install(FILES ${SUBSURFACE_SOURCE}/wreck.jpg DESTINATION ${WINDOWS_STAGING_TESTS})

	# Check if we can run tests locally using wine
	# Add a fake test used to ensure data is deployed to WINDOWS_STAGING_TESTS before running
	find_program(WINE_PROGRAM wine)
	if(WINE_PROGRAM)
		add_test(
			NAME InstallTestsDataAndDependencies
			COMMAND "${CMAKE_COMMAND}" --build ${CMAKE_CURRENT_BINARY_DIR} --target install
			)
	endif()
endif()

# Helper macro TEST used to created rules to build, link, install and run tests
macro(TEST NAME FILE)
	add_executable(${NAME} ${FILE} )
	target_link_libraries(
		${NAME}
		subsurface_corelib
		RESOURCE_LIBRARY
		${QT_TEST_LIBRARIES}
		${SUBSURFACE_LINK_LIBRARIES}
		)

	if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
		# Re-install dependencies in WINDOWS_STAGING_TESTS (and not in WINDOWSSTAGING)
		# to avoid packing testing related dlls in the installer
		install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} -DSUBSURFACE_TARGET=${NAME} -DSUBSURFACE_SOURCE=${SUBSURFACE_SOURCE} -DSTAGING=${WINDOWS_STAGING_TESTS} -P ${CMAKE_SOURCE_DIR}/cmake/Modules/dlllist.cmake WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})")

		# Run test using wine
		if(WINE_PROGRAM)
			add_test(
				NAME ${NAME}
				COMMAND "$<TARGET_FILE:${NAME}>"
				WORKING_DIRECTORY ${WINDOWS_STAGING_TESTS}
				)
			# Set WINEPATH (%PATH%) to WINDOWS_STAGING_TESTS allowing wine to find dlls
			# WINEDEBUG=-all is used to avoid anoying winde debug outputs
			set_tests_properties(${NAME} PROPERTIES ENVIRONMENT "WINEPATH=${WINDOWS_STAGING_TESTS};WINEDEBUG=-all")
			set_tests_properties(${NAME} PROPERTIES DEPENDS PrepareTests)
		endif()
	else()
		add_test(NAME ${NAME} COMMAND  $<TARGET_FILE:${NAME}>)
	endif()
endmacro()

enable_testing()
add_definitions(-g)
add_definitions(-DSUBSURFACE_TEST_DATA="${SUBSURFACE_TEST_DATA}")

TEST(TestUnitConversion testunitconversion.cpp)
TEST(TestProfile testprofile.cpp)
TEST(TestGpsCoords testgpscoords.cpp)
TEST(TestParse testparse.cpp)
TEST(TestPlan testplan.cpp)
TEST(TestDiveSiteDuplication testdivesiteduplication.cpp)
TEST(TestRenumber testrenumber.cpp)
TEST(TestGitStorage testgitstorage.cpp)
TEST(TestPreferences testpreferences.cpp)
TEST(TestPicture testpicture.cpp)
TEST(TestMerge testmerge.cpp)


add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
	DEPENDS
	TestUnitConversion
	TestProfile
	TestGpsCoords
	TestParse
	TestGitStorage
	TestPlan
	TestDiveSiteDuplication
	TestPreferences
	TestRenumber
	TestPicture
	TestMerge
)

# useful for debugging CMake issues
# print_all_variables()
