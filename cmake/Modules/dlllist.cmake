message(STATUS "processing dlllist.cmake")

# figure out which command to use for objdump
execute_process(
	COMMAND ${CMAKE_C_COMPILER} -dumpmachine
	OUTPUT_VARIABLE OBJDUMP
	OUTPUT_STRIP_TRAILING_WHITESPACE
)
# figure out where we should search for libraries
execute_process(
	COMMAND ${CMAKE_C_COMPILER} -print-search-dirs
	COMMAND sed -nE "/^libraries: =/{s///;s,/lib/?\(:|\$\$\),/bin\\1,g;p;q;}"
	OUTPUT_VARIABLE ADDPATH
	OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "addpath is ${ADDPATH}")
# since cmake doesn't appear to give us a variable with
# all libraries we link against, grab the link.txt script
# instead and drop the command name from it (before the
# first space) -- this will fail if the full path for the
# linker used contains a space...
execute_process(
	COMMAND tail -1 CMakeFiles/subsurface.dir/link.txt
	COMMAND cut -d\  -f 2-
	OUTPUT_VARIABLE LINKER_LINE
	OUTPUT_STRIP_TRAILING_WHITESPACE
)
# finally run our win-ldd.pl script against that to
# collect all the required dlls
execute_process(
	COMMAND sh -c "OBJDUMP=${OBJDUMP}-objdump PATH=$ENV{PATH}:${ADDPATH} perl ${SUBSURFACE_SOURCE}/scripts/win-ldd.pl ${SUBSURFACE_TARGET}.exe ${LINKER_LINE}"
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	OUTPUT_VARIABLE DLLS
	OUTPUT_STRIP_TRAILING_WHITESPACE
)
# replace newlines with semicolons so this is a cmake list
string(REPLACE "\n" ";" DLLLIST ${DLLS})
# executing 'install' as a command seems hacky, but you
# can't use the install() cmake function in a script
foreach(DLL ${DLLLIST})
	execute_process(COMMAND install ${DLL} ${STAGING})
endforeach()
