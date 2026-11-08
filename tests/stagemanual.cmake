# Make the staged user manual visible next to TestUserManual, see tests/CMakeLists.txt.
#
# cmake -DTEST_DIR=<directory of the executable> -DDOC_DIR=<staged Documentation> -P stagemanual.cmake
#
# Nothing is done if the executable is in the directory that holds the staged manual already,
# and an existing Documentation that is not a link is left alone, so nothing but an older
# link is ever replaced. Staging only makes the packaged manual test findable, so a link that
# cannot be made is a warning and does not fail the build.
get_filename_component(test_dir "${TEST_DIR}" REALPATH)
get_filename_component(doc_parent "${DOC_DIR}" DIRECTORY)
get_filename_component(doc_parent "${doc_parent}" REALPATH)
if(test_dir STREQUAL doc_parent)
	return()
endif()

set(link "${test_dir}/Documentation")
if(IS_SYMLINK "${link}")
	file(REMOVE "${link}")
elseif(EXISTS "${link}")
	message(WARNING "${link} exists and is not a link, so the manual is not staged there")
	return()
endif()
file(CREATE_LINK "${DOC_DIR}" "${link}" SYMBOLIC RESULT link_result)
if(NOT link_result STREQUAL "0")
	message(WARNING "Could not link the staged manual to ${link} (${link_result}), "
		"so the packaged manual test will skip")
endif()
