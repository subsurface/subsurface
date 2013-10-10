marbledir.files = $$MARBLEDIR
xslt.files = $$XSLT_FILES
doc.files = $$DOC_FILES
translation.files = $$replace(TRANSLATIONS, .ts, .qm)

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
    xslt.path = $$datadir
    doc.path = $$datadir/doc
    translation.path = Contents/Resources/translations
    doc.files = $$files($$doc.files)
    QMAKE_BUNDLE_DATA += marbledir xslt doc translation

    mac_deploy.target = mac-deploy
    mac_deploy.commands += $$[QT_INSTALL_BINS]/macdeployqt $${TARGET}.app
    !isEmpty(V):mac_deploy.commands += -verbose=1

    mac_dmg.target = mac-create-dmg
    mac_dmg.commands = $$mac_deploy.commands -dmg
    mac_dmg.commands += $${nltab}$(MOVE) $${TARGET}.dmg $${TARGET}-$${VERSION}.dmg
    QMAKE_EXTRA_TARGETS += mac_deploy mac_dmg
} else: win32 {
    # Windows bundling rules
    # We don't have a helpful tool like macdeployqt for Windows, so we hardcode
    # which libs we need.
    # The only target is "make install", which copies everything into packaging/windows
    WINDOWSSTAGING = packaging/windows

    deploy.path = $$WINDOWSSTAGING
    deploy.files += $$xslt.files $$doc.files
    target.path = $$WINDOWSSTAGING
    marbledir.path = $$WINDOWSSTAGING/data
    INSTALLS += deploy marbledir target

    qt_conf.commands = echo \'[Paths]\' > $@
    qt_conf.commands += $${nltab}echo \'Prefix=.\' >> $@
    qt_conf.commands += $${nltab}echo \'Plugins=.\' >> $@
    qt_conf.target = $$PWD/packaging/windows/qt.conf
    install.depends += qt_conf

    !win32-msvc* {
        #!equals($$QMAKE_HOST.os, "Windows"): dlls.commands += OBJDUMP=`$(CC) -dumpmachine`-objdump
        dlls.commands += PATH=\$\$PATH:`$(CC) -print-search-dirs | $(SED) -nE \'/^libraries: =/{s///;s,/lib/?(:|\\\$\$),/bin\\1,g;p;q;}\'`
        dlls.commands += perl $$PWD/scripts/win-ldd.pl $(DESTDIR_TARGET)

        for(plugin, $$list($$DEPLOYMENT_PLUGIN)) {
            CONFIG(debug, debug|release): dlls.commands += $$[QT_INSTALL_PLUGINS]/$${plugin}d4.dll
            else: dlls.commands += $$[QT_INSTALL_PLUGINS]/$${plugin}4.dll
        }

        dlls.commands += $$LIBS
        dlls.commands += | while read name; do $(INSTALL_FILE) \$\$name $$PWD/$$WINDOWSSTAGING; done
        dlls.depends = $(DESTDIR_TARGET)
        install.depends += dlls
    }
} else {
    # Linux install rules
    # On Linux, we can count on packagers doing the right thing
    # We just need to drop a few files here and there

    # This is a fake rule just to create some rules in the target file
    nl = $$escape_expand(\\n)
    dummy.target = dummy-only-for-var-expansion
    dummy.commands  = $${nl}prefix = /usr$${nl}\
BINDIR = $(prefix)/bin$${nl}\
DATADIR = $(prefix)/share$${nl}\
DOCDIR = $(DATADIR)/doc/subsurface$${nl}\
DESKTOPDIR = $(DATADIR)/applications$${nl}\
ICONPATH = $(DATADIR)/icons/hicolor$${nl}\
ICONDIR = $(ICONPATH)/scalable/apps$${nl}\
MANDIR = $(DATADIR)/man/man1$${nl}\
XSLTDIR = $(DATADIR)/subsurface
    QMAKE_EXTRA_TARGETS += dummy

    WINDOWSSTAGING = ./packaging/windows

    target.path = /$(BINDIR)
    target.files = $$TARGET

    desktop.path = /$(DESKTOPDIR)
    desktop.files = $$DESKTOP_FILE
    manpage.path = /$(MANDIR)
    manpage.files = $$MANPAGE

    icon.path = /$(ICONDIR)
    icon.files = $$ICON

    xslt.path = /$(XSLTDIR)
    marbledir.path = /$(DATADIR)/subsurface
    doc.path = /$(DOCDIR)

    # FIXME: Linguist translations
    #l10n_install.commands = for LOC in $$files(share/locale/*/LC_MESSAGES); do \
    #                $(INSTALL_PROGRAM) -d $(INSTALL_ROOT)/$(prefix)/$$LOC; \
    #                $(INSTALL_FILE) $$LOC/subsurface.mo $(INSTALL_ROOT)/$(prefix)/$$LOC/subsurface.mo; \
    #        done
    #install.depends += l10n_install

    INSTALLS += target desktop icon manpage xslt doc marbledir
    install.target = install
}
!isEmpty(TRANSLATIONS) {
	isEmpty(QMAKE_LRELEASE) {
		win32: QMAKE_LRELEASE = $$[QT_INSTALL_BINS]\lrelease.exe
		else: QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
	}
	isEmpty(TS_DIR):TS_DIR = translations
	TSQM.target = .translations
	TSQM.name = lrelease subsurface.pro
	TSQM.depends = $$TRANSLATIONS
	TSQM.output = $$TS_DIR/${QMAKE_FILE_BASE}.qm
	TSQM.commands = $$QMAKE_LRELEASE subsurface.pro && echo "OK" > .translations
	QMAKE_EXTRA_TARGETS += TSQM
	PRE_TARGETDEPS += .translations
}
QMAKE_EXTRA_TARGETS += install $$install.depends
