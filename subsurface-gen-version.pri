# Generate the ssrf-version.h file
macx: VER_OS = darwin
unix: !macx: VER_OS = linux
win32: VER_OS = win

# use a compiler target that has a phony input and is forced on each `make` invocation
# the resulted file is not a link target and is cleared with `make clean`
PHONY_DEPS = .
version_h.input = PHONY_DEPS
version_h.depends = FORCE
version_h.output = $$VERSION_FILE
version_h.commands = cd $$PWD && sh $$PWD/scripts/write-version $$VERSION_FILE $$VERSION $$VER_OS
silent: version_h.commands = @echo Checking $$VERSION_FILE && $$version_h.commads
version_h.CONFIG += no_link
QMAKE_EXTRA_COMPILERS += version_h
QMAKE_CLEAN += $$VERSION_FILE
