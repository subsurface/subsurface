# QTest based tests
qt5_add_resources(SUBSURFACE_TEST_RESOURCES ../subsurface.qrc)

add_library(RESOURCE_LIBRARY STATIC ${SUBSURFACE_TEST_RESOURCES})

macro(TEST NAME FILE)
	add_executable(${NAME} ${FILE} )
	target_link_libraries(${NAME} subsurface_corelib RESOURCE_LIBRARY ${QT_TEST_LIBRARIES} ${SUBSURFACE_LINK_LIBRARIES} )
	add_test(NAME ${NAME} COMMAND  $<TARGET_FILE:${NAME}>)
endmacro()

enable_testing()
add_definitions(-g)
TEST(TestUnitConversion testunitconversion.cpp)
TEST(TestProfile testprofile.cpp)
TEST(TestGpsCoords testgpscoords.cpp)
TEST(TestParse testparse.cpp)
TEST(TestPlan testplan.cpp)
TEST(TestDiveSiteDuplication testdivesiteduplication.cpp)
TEST(TestRenumber testrenumber.cpp)
TEST(TestGitStorage testgitstorage.cpp)
TEST(TestPreferences testpreferences.cpp)

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
)

add_custom_target(check_travis COMMAND ${CMAKE_CTEST_COMMAND}
	DEPENDS
	TestUnitConversion
	TestProfile
	TestGpsCoords
	TestParse
	TestGitStorage
	TestPlan
	TestDiveSiteDuplication
	TestRenumber
)
