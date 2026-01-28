# SPDX-License-Identifier: GPL-2.0
# CLI test suite integration

# Find Python for running tests
find_package(Python3 COMPONENTS Interpreter REQUIRED)

# Add test target
add_custom_target(test-cli
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_cli.py
        --cli-path $<TARGET_FILE:subsurface-cli>
        --test-data ${CMAKE_SOURCE_DIR}/dives/SampleDivesV2.ssrf
    DEPENDS subsurface-cli
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running CLI test suite"
)

# Also register with CTest
add_test(
    NAME cli-tests
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_cli.py
        --cli-path $<TARGET_FILE:subsurface-cli>
        --test-data ${CMAKE_SOURCE_DIR}/dives/SampleDivesV2.ssrf
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
