marbledir.files = $$MARBLEDIR
doc.files = $$DOC_FILES
theme.files = $$THEME_FILES
translation.files = $$replace(TRANSLATIONS, .ts, .qm)
exists($$[QT_INSTALL_TRANSLATIONS]) {
        qt_translation_dir = $$[QT_INSTALL_TRANSLATIONS]
} else: exists(/usr/share/qt5/translations) {
	# On some cross-compilation environments, the translations are either missing or not
	# where they're expected to be. In such cases, try copying from the system.
	qt_translation_dir = /usr/share/qt5/translations
}

# Prepend the Qt translation dir so we can actually find the files
qttranslation.files =
for(translation, QTTRANSLATIONS): \
    qttranslation.files += $${qt_translation_dir}/$$translation

nltab = $$escape_expand(\\n\\t)

mac {
	# OS X bundling rules
	#  "make mac-deploy" deploys the external libs (Qt, libdivecomputer, libusb, etc.) into the bundle
	#  "make install" installs the bundle to /Applications
	#  "make mac-create-dmg" creates Subsurface.dmg

	mac_bundle.path = /Applications
	mac_bundle.files = Subsurface.app
	mac_bundle.CONFIG += no_check_exist directory
	INSTALLS += mac_bundle
	install.depends += mac-deploy

	datadir = Contents/Resources/share
	marbledir.path = Contents/Resources/data
	theme.path = Contents/Resources
	doc.path = $$datadir/Documentation
	translation.path = Contents/Resources/translations
	qttranslation.path = Contents/Resources/translations
	QMAKE_BUNDLE_DATA += marbledir doc translation qttranslation theme

	mac_deploy.target = mac-deploy
	mac_deploy.commands += $$[QT_INSTALL_BINS]/macdeployqt $${TARGET}.app
	!isEmpty(V):mac_deploy.commands += -verbose=1

	mac_dmg.target = mac-create-dmg
	mac_dmg.commands = $$mac_deploy.commands -dmg
	mac_dmg.commands += $${nltab}$(MOVE) $${TARGET}.dmg $${TARGET}-$${FULL_VERSION}.dmg
	QMAKE_EXTRA_TARGETS += mac_deploy mac_dmg
} else: win32 {
	# Windows bundling rules
	# We don't have a helpful tool like macdeployqt for Windows, so we hardcode
	# which libs we need.
	# The only target is "make install", which copies everything into packaging/windows
	WINDOWSSTAGING = $$OUT_PWD/staging
	WINDOWSPACKAGING = $$PWD/packaging/windows
	NSIFILE = $$WINDOWSSTAGING/subsurface.nsi
	NSIINPUTFILE = $$WINDOWSPACKAGING/subsurface.nsi.in
	MAKENSIS = /usr/bin/makensis

	doc.path = $$WINDOWSSTAGING/Documentation
	CONFIG -= copy_dir_files
	deploy.path = $$WINDOWSSTAGING
	deploy.CONFIG += no_check_exist
	target.path = $$WINDOWSSTAGING
	marbledir.path = $$WINDOWSSTAGING/data
	theme.path = $$WINDOWSSTAGING
	INSTALLS += deploy marbledir target doc theme

	translation.path = $$WINDOWSSTAGING/translations
	qttranslation.path = $$WINDOWSSTAGING/translations
	package.files = $$PWD/gpl-2.0.txt $$WINDOWSPACKAGING/subsurface.ico
	package.path = $$WINDOWSSTAGING
	INSTALLS += translation qttranslation package

	qt_conf.commands = echo \'[Paths]\' > $@
	qt_conf.commands += $${nltab}echo \'Prefix=.\' >> $@
	qt_conf.target = $$WINDOWSSTAGING/qt.conf
	install.depends += qt_conf

	# Plugin code
	defineTest(deployPlugin) {
		plugin = $$1
		plugintype = $$dirname(1)
		CONFIG(debug, debug|release): plugin = $${plugin}d4.dll
		else: plugin = $${plugin}4.dll

		abs_plugin = $$[QT_INSTALL_PLUGINS]/$$plugin
		ABS_DEPLOYMENT_PLUGIN += $$abs_plugin
		export(ABS_DEPLOYMENT_PLUGIN)

		safe_name = $$replace(1, /, _)
		INSTALLS += $$safe_name

		# Work around qmake bug in Qt4 that it can't handle $${xx}.yy properly
		eval(safe_name_files = $${safe_name}.files)
		eval(safe_name_path = $${safe_name}.path)
		$$safe_name_files = $$abs_plugin
		$$safe_name_path = $$WINDOWSSTAGING/plugins/$$plugintype
		export($$safe_name_files)
		export($$safe_name_path)
		export(INSTALLS)
	}
	# Convert plugin names to the relative DLL path
	for(plugin, $$list($$DEPLOYMENT_PLUGIN)) {
		deployPlugin($$plugin)
	}

	!win32-msvc* {
		#!equals($$QMAKE_HOST.os, "Windows"): dlls.commands += OBJDUMP=`$$QMAKE_CC -dumpmachine`-objdump
		dlls.commands += PATH=\$\$PATH:`$$QMAKE_CC -print-search-dirs | sed -nE \'/^libraries: =/{s///;s,/lib/?(:|\$\$),/bin\\1,g;p;q;}\'`
		dlls.commands += perl $$PWD/scripts/win-ldd.pl
		# equals(QMAKE_HOST.os, "Windows"): EXE_SUFFIX = .exe
		EXE_SUFFIX = .exe
		CONFIG(debug, debug|release): dlls.commands += $$OUT_PWD/debug/subsurface$$EXE_SUFFIX
		else: dlls.commands += $$OUT_PWD/release/$$TARGET$$EXE_SUFFIX

		dlls.commands += $$ABS_DEPLOYMENT_PLUGIN $$LIBS
		dlls.commands += | while read name; do $(INSTALL_FILE) \$\$name $$WINDOWSSTAGING; done
		dlls.depends += $(DESTDIR_TARGET)

		nsis.commands += $(CHK_DIR_EXISTS) $$WINDOWSSTAGING;
		win64target {
			nsis.commands += cat $$NSIINPUTFILE | sed -e \'s/VERSIONTOKEN/$$VERSION_STRING/;s/PRODVTOKEN/$${PRODVERSION_STRING}/;s/64BITBUILDTOKEN/1 == 1/\' > $$NSIFILE
		} else {
			nsis.commands += cat $$NSIINPUTFILE | sed -e \'s/VERSIONTOKEN/$$VERSION_STRING/;s/PRODVTOKEN/$${PRODVERSION_STRING}/;s/64BITBUILDTOKEN/1 == 0/\' > $$NSIFILE
		}
		nsis.depends += $$NSIINPUTFILE
		nsis.target = $$NSISFILE
		#
		# FIXME HACK HACK FIXME  -- this is needed to create working daily builds...
		#
		brokenQt532win {
			installer.commands += cp Qt531/*.dll staging;
		}
		installer.commands += $$MAKENSIS $$NSIFILE
		installer.target = installer
		installer.depends = nsis install
		QMAKE_EXTRA_TARGETS = installer nsis
		install.depends += dlls
	}
} else: android {
	# Android install rules
	QMAKE_BUNDLE_DATA += translation qttranslation
	# Android template directory
	ANDROID_PACKAGE_SOURCE_DIR = $$OUT_PWD/android
} else {
	# Linux install rules
	# On Linux, we can count on packagers doing the right thing
	# We just need to drop a few files here and there

	# This is a fake rule just to create some rules in the target file
	dummy.target = dummy-only-for-var-expansion
	dummy.commands	= $$escape_expand(\\n)prefix = /usr

	QMAKE_EXTRA_VARIABLES = BINDIR DATADIR DOCDIR DESKTOPDIR ICONPATH ICONDIR MANDIR
	BINDIR = $(prefix)/bin
	DATADIR = $(prefix)/share
	DOCDIR = $(EXPORT_DATADIR)/subsurface/Documentation
	DESKTOPDIR = $(EXPORT_DATADIR)/applications
	ICONPATH = $(EXPORT_DATADIR)/icons/hicolor
	ICONDIR = $(EXPORT_ICONPATH)/scalable/apps
	MANDIR = $(EXPORT_DATADIR)/man/man1

	QMAKE_EXTRA_TARGETS += dummy

	target.path = /$(EXPORT_BINDIR)
	target.files = $$TARGET

	desktop.path = /$(EXPORT_DESKTOPDIR)
	desktop.files = $$DESKTOP_FILE
	manpage.path = /$(EXPORT_MANDIR)
	manpage.files = $$MANPAGE

	icon.path = /$(EXPORT_ICONDIR)
	icon.files = $$ICON

	marbledir.path = /$(EXPORT_DATADIR)/subsurface/data
	doc.path = /$(EXPORT_DOCDIR)
	theme.path = /$(EXPORT_DATADIR)/subsurface

	doc.CONFIG += no_check_exist

	translation.path = /$(EXPORT_DATADIR)/subsurface/translations
	translation.CONFIG += no_check_exist

	INSTALLS += target desktop manpage doc marbledir translation icon theme
	install.target = install
}
!isEmpty(TRANSLATIONS) {
	isEmpty(QMAKE_LRELEASE) {
		win32: QMAKE_LRELEASE = $$[QT_INSTALL_BINS]\\lrelease.exe
		else: QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
	}
	isEmpty(TS_DIR):TS_DIR = translations
	TSQM.input = TRANSLATIONS
	TSQM.output = $$TS_DIR/${QMAKE_FILE_BASE}.qm
	TSQM.CONFIG += no_link target_predeps
	TSQM.commands = $$QMAKE_LRELEASE ${QMAKE_FILE_IN} -qm $$TS_DIR/${QMAKE_FILE_BASE}.qm
	QMAKE_EXTRA_COMPILERS += TSQM
}
QMAKE_EXTRA_TARGETS += install $$install.depends
