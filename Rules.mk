# -*- Makefile -*-
# Rules for building and creating the version file

VERSION_FILE = version.h
# There's only one line in $(VERSION_FILE); use the shell builtin `read'
STORED_VERSION_STRING = \
	$(subst ",,$(shell [ ! -r $(VERSION_FILE) ] || \
			   read ignore ignore v <$(VERSION_FILE) && echo $$v))
#" workaround editor syntax highlighting quirk

GET_VERSION = ./scripts/get-version
VERSION_STRING := $(shell $(GET_VERSION) linux || echo "v$(VERSION)")
# Mac Info.plist style with three numbers 1.2.3
CFBUNDLEVERSION_STRING := $(shell $(GET_VERSION) darwin $(VERSION_STRING) || \
	echo "$(VERSION).0")
# Windows .nsi style with four numbers 1.2.3.4
PRODVERSION_STRING := $(shell $(GET_VERSION) win $(VERSION_STRING) || \
	echo "$(VERSION).0.0")

MSGOBJS=$(addprefix share/locale/,$(MSGLANGS:.po=.UTF-8/LC_MESSAGES/subsurface.mo))

C_SOURCES = $(filter %.c, $(SOURCES))
CXX_SOURCES = $(filter %.cpp, $(SOURCES))
OTHER_SOURCES = $(filter-out %.c %.cpp, $(SOURCES))
OBJS = $(C_SOURCES:.c=.o) $(CXX_SOURCES:.cpp=.o) $(OTHER_SOURCES)

# Add the objects for the header files which define QObject subclasses
HEADERS_NEEDING_MOC += $(shell grep -l -s 'Q_OBJECT' $(HEADERS))
MOC_OBJS = $(HEADERS_NEEDING_MOC:.h=.moc.o)

ALL_OBJS = $(OBJS) $(MOC_OBJS)

DEPS = $(wildcard .dep/*.dep)

all: $(NAME)

$(NAME): gen_version_file $(ALL_OBJS) $(MSGOBJS) $(INFOPLIST)
	$(CXX) $(LDFLAGS) -o $(NAME) $(ALL_OBJS) $(LIBS)

gen_version_file:
ifneq ($(STORED_VERSION_STRING),$(VERSION_STRING))
	$(info updating $(VERSION_FILE) to $(VERSION_STRING))
	@echo \#define VERSION_STRING \"$(VERSION_STRING)\" >$(VERSION_FILE)
endif

install: all
	$(INSTALL) -d -m 755 $(BINDIR)
	$(INSTALL) $(NAME) $(BINDIR)
	$(INSTALL) -d -m 755 $(DESKTOPDIR)
	$(INSTALL) $(DESKTOPFILE) $(DESKTOPDIR)
	$(INSTALL) -d -m 755 $(ICONDIR)
	$(INSTALL) -m 644 $(ICONFILE) $(ICONDIR)
	@-if test -z "$(DESTDIR)"; then \
		$(gtk_update_icon_cache); \
	fi
	$(INSTALL) -d -m 755 $(MANDIR)
	$(INSTALL) -m 644 $(MANFILES) $(MANDIR)
	@-if test ! -z "$(XSLT)"; then \
		$(INSTALL) -d -m 755 $(DATADIR)/subsurface; \
		$(INSTALL) -d -m 755 $(XSLTDIR); \
		$(INSTALL) -m 644 $(XSLTFILES) $(XSLTDIR); \
	fi
	for LOC in $(wildcard share/locale/*/LC_MESSAGES); do \
		$(INSTALL) -d $(prefix)/$$LOC; \
		$(INSTALL) -m 644 $$LOC/subsurface.mo $(prefix)/$$LOC/subsurface.mo; \
	done


install-macosx: all
	$(INSTALL) -d -m 755 $(MACOSXINSTALL)/Contents/Resources
	$(INSTALL) -d -m 755 $(MACOSXINSTALL)/Contents/MacOS
	$(INSTALL) $(NAME) $(MACOSXINSTALL)/Contents/MacOS/$(NAME)-bin
	$(INSTALL) $(MACOSXFILES)/$(NAME).sh $(MACOSXINSTALL)/Contents/MacOS/$(NAME)
	$(INSTALL) $(MACOSXFILES)/PkgInfo $(MACOSXINSTALL)/Contents/
	$(INSTALL) $(MACOSXFILES)/Info.plist $(MACOSXINSTALL)/Contents/
	$(INSTALL) $(ICONFILE) $(MACOSXINSTALL)/Contents/Resources/
	$(INSTALL) $(MACOSXFILES)/Subsurface.icns $(MACOSXINSTALL)/Contents/Resources/
	for LOC in $(wildcard share/locale/*/LC_MESSAGES); do \
		$(INSTALL) -d -m 755 $(MACOSXINSTALL)/Contents/Resources/$$LOC; \
		$(INSTALL) $$LOC/subsurface.mo $(MACOSXINSTALL)/Contents/Resources/$$LOC/subsurface.mo; \
	done
	@-if test ! -z "$(XSLT)"; then \
		$(INSTALL) -d -m 755 $(MACOSXINSTALL)/Contents/Resources/xslt; \
		$(INSTALL) -m 644 $(XSLTFILES) $(MACOSXINSTALL)/Contents/Resources/xslt/; \
	fi


create-macosx-bundle: all
	$(INSTALL) -d -m 755 $(MACOSXSTAGING)/Contents/Resources
	$(INSTALL) -d -m 755 $(MACOSXSTAGING)/Contents/MacOS
	$(INSTALL) $(NAME) $(MACOSXSTAGING)/Contents/MacOS/
	$(INSTALL) $(MACOSXFILES)/PkgInfo $(MACOSXSTAGING)/Contents/
	$(INSTALL) $(MACOSXFILES)/Info.plist $(MACOSXSTAGING)/Contents/
	$(INSTALL) $(ICONFILE) $(MACOSXSTAGING)/Contents/Resources/
	$(INSTALL) $(MACOSXFILES)/Subsurface.icns $(MACOSXSTAGING)/Contents/Resources/
	for LOC in $(wildcard share/locale/*/LC_MESSAGES); do \
		$(INSTALL) -d -m 755 $(MACOSXSTAGING)/Contents/Resources/$$LOC; \
		$(INSTALL) $$LOC/subsurface.mo $(MACOSXSTAGING)/Contents/Resources/$$LOC/subsurface.mo; \
	done
	@-if test ! -z "$(XSLT)"; then \
		$(INSTALL) -d -m 755 $(MACOSXSTAGING)/Contents/Resources/xslt; \
		$(INSTALL) -m 644 $(XSLTFILES) $(MACOSXSTAGING)/Contents/Resources/xslt/; \
	fi
	$(GTK_MAC_BUNDLER) packaging/macosx/subsurface.bundle

sign-macosx-bundle: all
	codesign -s "3A8CE62A483083EDEA5581A61E770EC1FA8BECE8" /Applications/Subsurface.app/Contents/MacOS/subsurface-bin

install-cross-windows: all
	$(INSTALL) -d -m 755 $(WINDOWSSTAGING)/share/locale
	for MSG in $(WINMSGDIRS); do\
		$(INSTALL) -d -m 755 $(WINDOWSSTAGING)/$$MSG;\
		$(INSTALL) $(CROSS_PATH)/$$MSG/* $(WINDOWSSTAGING)/$$MSG;\
	done
	for LOC in $(wildcard share/locale/*/LC_MESSAGES); do \
		$(INSTALL) -d -m 755 $(WINDOWSSTAGING)/$$LOC; \
		$(INSTALL) $$LOC/subsurface.mo $(WINDOWSSTAGING)/$$LOC/subsurface.mo; \
	done

create-windows-installer: all $(NSIFILE) install-cross-windows
	$(MAKENSIS) $(NSIFILE)

$(NSIFILE): $(NSIINPUTFILE)
	$(shell cat $(NSIINPUTFILE) | sed -e 's/VERSIONTOKEN/$(VERSION_STRING)/;s/PRODVTOKEN/$(PRODVERSION_STRING)/' > $(NSIFILE))

$(INFOPLIST): $(INFOPLISTINPUT)
	$(shell cat $(INFOPLISTINPUT) | sed -e 's/CFBUNDLEVERSION_TOKEN/$(CFBUNDLEVERSION_STRING)/' > $(INFOPLIST))

# Transifex merge the translations
update-po-files:
	xgettext -o po/subsurface-new.pot -s -k_ -kN_ -ktr --keyword=C_:1c,2  --add-comments="++GETTEXT" *.c qt-ui/*.cpp
	tx push -s
	tx pull -af

MOCFLAGS = $(filter -I%, $(CXXFLAGS) $(EXTRA_FLAGS)) $(filter -D%, $(CXXFLAGS) $(EXTRA_FLAGS))

%.o: %.c
	@echo '    CC' $<
	@mkdir -p .dep .dep/qt-ui
	@$(CC) $(CFLAGS) $(EXTRA_FLAGS) -MD -MF .dep/$@.dep -c -o $@ $<

%.o: %.cpp
	@echo '    CXX' $<
	@mkdir -p .dep .dep/qt-ui
	@$(CXX) $(CXXFLAGS) $(EXTRA_FLAGS) -MD -MF .dep/$@.dep -c -o $@ $<

# Detect which files require the moc or uic tools to be run
CPP_NEEDING_MOC = $(shell grep -l -s '^\#include \".*\.moc\"' $(OBJS:.o=.cpp))
OBJS_NEEDING_MOC += $(CPP_NEEDING_MOC:.cpp=.o)

CPP_NEEDING_UIC = $(shell grep -l -s '^\#include \"ui_.*\.h\"' $(OBJS:.o=.cpp))
OBJS_NEEDING_UIC += $(CPP_NEEDING_UIC:.cpp=.o)

# This rule is for running the moc on QObject subclasses defined in the .h
# files.
%.moc.cpp: %.h
	@echo '    MOC' $<
	@$(MOC) $(MOCFLAGS) $< -o $@

# This rule is for running the moc on QObject subclasses defined in the .cpp
# files; remember to #include "<file>.moc" at the end of the .cpp file, or
# you'll get linker errors ("undefined vtable for...")
%.moc: %.cpp
	@echo '    MOC' $<
	@$(MOC) -i $(MOCFLAGS) $< -o $@

# This creates the ui headers.
ui_%.h: %.ui
	@echo '    UIC' $<
	@$(UIC) $< -o $@

$(OBJS_NEEDING_MOC): %.o: %.moc
$(OBJS_NEEDING_UIC): qt-ui/%.o: qt-ui/ui_%.h

share/locale/%.UTF-8/LC_MESSAGES/subsurface.mo: po/%.po po/%.aliases
	mkdir -p $(dir $@)
	msgfmt -c -o $@ po/$*.po
	@-if test -s po/$*.aliases; then \
		for ALIAS in `cat po/$*.aliases`; do \
			mkdir -p share/locale/$$ALIAS/LC_MESSAGES; \
			cp $@ share/locale/$$ALIAS/LC_MESSAGES; \
		done; \
	fi

satellite.png: satellite.svg
	convert -transparent white -resize 11x16 -depth 8 $< $@

# This should work, but it doesn't get the colors quite right - so I manually converted with Gimp
#	convert -colorspace RGB -transparent white -resize 256x256 subsurface-icon.svg subsurface-icon.png
#
# The following creates the pixbuf data in .h files with the basename followed by '_pixmap'
# as name of the data structure
%.h: %.png
	@echo '    gdk-pixbuf-csource' $<
	@gdk-pixbuf-csource --struct --name `echo $* | sed 's/-/_/g'`_pixbuf $< > $@

doc:
	$(MAKE) -C Documentation doc

clean:
	rm -f $(ALL_OBJS) *~ $(NAME) $(NAME).exe po/*~ po/subsurface-new.pot \
		$(VERSION_FILE) qt-ui/*.moc qt-ui/ui_*.h
	rm -rf share .dep

confclean: clean
	rm -f $(CONFIGFILE)

-include $(DEPS)
