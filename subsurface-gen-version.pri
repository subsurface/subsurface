# Generate the version.h file
VERSION_FILE = version.h
macx: VER_OS = darwin
unix: !macx: VER_OS = linux
win32: VER_OS = win
exists(.git/HEAD): {
    GIT_HEAD = .git/HEAD
    VERSION_SCRIPT = $$PWD/scripts/get-version
    version_h.depends = $$VERSION_SCRIPT
    version_h.commands = echo \\$${LITERAL_HASH}define VERSION_STRING \\\"`$$VERSION_SCRIPT $$VER_OS`\\\" > ${QMAKE_FILE_OUT}
    version_h.input = GIT_HEAD
    version_h.output = $$VERSION_FILE
    version_h.variable_out = GENERATED_FILES
    version_h.CONFIG = ignore_no_exist
    QMAKE_EXTRA_COMPILERS += version_h
} else {
    # This is probably a package
    system(echo \\$${LITERAL_HASH}define VERSION_STRING \\\"$$VERSION\\\" > $$VERSION_FILE)
}
