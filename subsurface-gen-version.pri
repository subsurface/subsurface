# Generate the ssrf-version.h file
VERSION_FILE = ssrf-version.h
macx: VER_OS = darwin
unix: !macx: VER_OS = linux
win32: VER_OS = win
exists(.git/HEAD): {
	win32: SET_GIT_DIR = set GIT_DIR
	else: SET_GIT_DIR = GIT_DIR
	GIT_HEAD = .git/HEAD
	VERSION_SCRIPT = $$PWD/scripts/get-version
	# always use linux here -------------------vvv	so we get the true full version
	FULL_VERSION = "`$$VERSION_SCRIPT linux`"
	VERSION = $$system("sh scripts/get-version full || echo $${VERSION}")
	PRODVERSION_STRING = $$system("sh scripts/get-version win $$FULL_VERSION || echo $${VERSION}.0.0-git")
	VERSION_STRING = $$system("sh scripts/get-version linux $$FULL_VERSION || echo $${VERSION}-git")
	version_h.depends = $$VERSION_SCRIPT $$PWD/.git/$$system("$$SET_GIT_DIR=$$PWD/.git git rev-parse --symbolic-full-name HEAD")
	version_h.commands = echo \\$${LITERAL_HASH}define VERSION_STRING \\\"`GIT_DIR=$$PWD/.git $$VERSION_SCRIPT $$VER_OS || echo $$VERSION-git`\\\" > ${QMAKE_FILE_OUT}
	version_h.input = GIT_HEAD
	version_h.output = $$VERSION_FILE
	version_h.variable_out = GENERATED_FILES
	version_h.CONFIG = ignore_no_exist
	QMAKE_EXTRA_COMPILERS += version_h
} else {
	# This is probably a package
	FULL_VERSION = $$VERSION
	system(echo \\$${LITERAL_HASH}define VERSION_STRING \\\"$$VERSION\\\" > $$VERSION_FILE)
	QMAKE_CLEAN += $$VERSION_FILE
}
