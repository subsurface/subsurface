# QTest based tests
qt5_add_resources(SUBSURFACE_TEST_RESOURCES ../subsurface.qrc)

# Access test data (dive folder) from SUBSURFACE_SOURCE by default.
# In cross compilation cases or when test will not be executed at build time
# a differnt value can be set via cmake -DSUBSURFACE_TEST_DATA.
if(NOT SUBSURFACE_TEST_DATA)
  set(SUBSURFACE_TEST_DATA ${SUBSURFACE_SOURCE})
endif()

add_library(RESOURCE_LIBRARY STATIC ${SUBSURFACE_TEST_RESOURCES})

macro(TEST NAME FILE)
	add_executable(${NAME} ${FILE} )
	target_link_libraries(${NAME} subsurface_corelib RESOURCE_LIBRARY ${QT_TEST_LIBRARIES} ${SUBSURFACE_LINK_LIBRARIES} )
	add_test(NAME ${NAME} COMMAND  $<TARGET_FILE:${NAME}>)
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
