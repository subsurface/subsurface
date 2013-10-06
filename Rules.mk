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

MSGOBJS=$(addprefix share/locale/,$(MSGLANGS:.po=.UTF-8/LC_MESSAGES/$(NAME).mo))

ifeq ($(V),1)
	PRETTYECHO=true
	COMPILE_PREFIX=
else
	PRETTYECHO=echo
	COMPILE_PREFIX=@
endif

C_SOURCES = $(filter %.c, $(SOURCES))
CXX_SOURCES = $(filter %.cpp, $(SOURCES)) $(RESOURCES:.qrc=.qrc.cpp)
OTHER_SOURCES = $(filter-out %.c %.cpp, $(SOURCES))
OBJS = $(C_SOURCES:.c=.o) $(CXX_SOURCES:.cpp=.o) $(OTHER_SOURCES)

# Add the objects for the header files which define QObject subclasses
HEADERS_NEEDING_MOC += $(shell grep -l -s 'Q_OBJECT' $(HEADERS))
MOC_OBJS = $(HEADERS_NEEDING_MOC:.h=.moc.o)

ALL_OBJS = $(OBJS) $(MOC_OBJS)

# handling of uic
UIC_HEADERS = $(patsubst %.ui, ui_%.h, $(subst qt-ui/,,$(FORMS)))
vpath %.ui qt-ui
vpath ui_%.h .uic

# Files for using Qt Creator
CREATOR_FILES = $(NAME).config $(NAME).creator $(NAME).files $(NAME).includes

all: $(TARGET) doc

$(TARGET): gen_version_file $(ALL_OBJS) $(INFOPLIST)
	@$(PRETTYECHO) '    LINK' $(TARGET)
	$(COMPILE_PREFIX)$(CXX) $(LDFLAGS) -o $(TARGET) $(ALL_OBJS) $(LIBS)

uicables: $(UIC_HEADERS)

gen_version_file $(VERSION_FILE):
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
		$(INSTALL) -d -m 755 $(DATADIR)/$(NAME); \
		$(INSTALL) -d -m 755 $(XSLTDIR); \
		$(INSTALL) -m 644 $(XSLTFILES) $(XSLTDIR); \
	fi
	@-if test ! -z "$(MARBLEDIR)"; then \
		$(INSTALL) -d -m 755 $(DATADIR)/$(NAME)/$(MARBLEDIR); \
		$(TAR) cf - $(MARBLEDIR) | ( cd $(DATADIR)/$(NAME); $(TAR) xf - ); \
	fi
	for LOC in $(wildcard share/locale/*/LC_MESSAGES); do \
		$(INSTALL) -d $(prefix)/$$LOC; \
		$(INSTALL) -m 644 $$LOC/$(NAME).mo $(prefix)/$$LOC/$(NAME).mo; \
	done
	$(INSTALL) -d -m 755 $(DOCDIR)
	$(INSTALL) -m 644 Documentation/user-manual.html $(DOCDIR)
	for IMG in $(wildcard Documentation/images/*); do \
		$(INSTALL) -m 644 $$IMG $(DOCDIR)/images; \
	done


install-macosx: all
	$(INSTALL) -d -m 755 $(MACOSXINSTALL)/Contents/Resources
	$(INSTALL) -d -m 755 $(MACOSXINSTALL)/Contents/MacOS
	$(INSTALL) $(NAME) $(MACOSXINSTALL)/Contents/MacOS/$(NAME)-bin
	$(INSTALL) $(MACOSXFILES)/$(NAME).sh $(MACOSXINSTALL)/Contents/MacOS/$(NAME)
	$(INSTALL) $(MACOSXFILES)/PkgInfo $(MACOSXINSTALL)/Contents/
	$(INSTALL) $(MACOSXFILES)/Info.plist $(MACOSXINSTALL)/Contents/
	$(INSTALL) $(ICONFILE) $(MACOSXINSTALL)/Contents/Resources/
	$(INSTALL) $(MACOSXFILES)/$(CAPITALIZED_NAME).icns $(MACOSXINSTALL)/Contents/Resources/
	@-if test ! -z "$(MARBLEDIR)"; then \
		$(INSTALL) -d -m 755 $(MACOSXINSTALL)/Contents/Resources/share/$(MARBLEDIR); \
		$(TAR) cf - $(MARBLEDIR) | ( cd $(MACOSXINSTALL)/Contents/Resources/share; $(TAR) xf - ); \
	fi
	for LOC in $(wildcard share/locale/*/LC_MESSAGES); do \
		$(INSTALL) -d -m 755 $(MACOSXINSTALL)/Contents/Resources/$$LOC; \
		$(INSTALL) $$LOC/$(NAME).mo $(MACOSXINSTALL)/Contents/Resources/$$LOC/$(NAME).mo; \
	done
	@-if test ! -z "$(XSLT)"; then \
		$(INSTALL) -d -m 755 $(MACOSXINSTALL)/Contents/Resources/share/xslt; \
		$(INSTALL) -m 644 $(XSLTFILES) $(MACOSXINSTALL)/Contents/Resources/share/xslt/; \
	fi
	$(INSTALL) -d -m 755 $(MACOSXINSTALL)/Contents/resources/share/doc/$(NAME)
	$(INSTALL) -m 644 Documentation/user-manual.html $(MACOSXINSTALL)/Contents/Resources/share/doc/$(NAME)
	for IMG in $(wildcard Documentation/images/*); do \
		$(INSTALL) -m 644 $$IMG $(MACOSXINSTALL)/Contents/Resources/share/doc/$(NAME)/images; \
	done


create-macosx-bundle: all
	$(INSTALL) -d -m 755 $(MACOSXSTAGING)/Contents/Resources
	$(INSTALL) -d -m 755 $(MACOSXSTAGING)/Contents/MacOS
	$(INSTALL) $(NAME) $(MACOSXSTAGING)/Contents/MacOS/
	$(INSTALL) $(MACOSXFILES)/PkgInfo $(MACOSXSTAGING)/Contents/
	$(INSTALL) $(MACOSXFILES)/Info.plist $(MACOSXSTAGING)/Contents/
	$(INSTALL) $(ICONFILE) $(MACOSXSTAGING)/Contents/Resources/
	$(INSTALL) $(MACOSXFILES)/$(CAPITALIZED_NAME).icns $(MACOSXSTAGING)/Contents/Resources/
	for LOC in $(wildcard share/locale/*/LC_MESSAGES); do \
		$(INSTALL) -d -m 755 $(MACOSXSTAGING)/Contents/Resources/$$LOC; \
		$(INSTALL) $$LOC/$(NAME).mo $(MACOSXSTAGING)/Contents/Resources/$$LOC/$(NAME).mo; \
	done
	@-if test ! -z "$(XSLT)"; then \
		$(INSTALL) -d -m 755 $(MACOSXSTAGING)/Contents/Resources/xslt; \
		$(INSTALL) -m 644 $(XSLTFILES) $(MACOSXSTAGING)/Contents/Resources/xslt/; \
	fi
	$(INSTALL) -d -m 755 $(MACOSXSTAGING)/Contents/resources/share/doc/$(NAME)
	$(INSTALL) -m 644 Documentation/user-manual.html $(MACOSXSTAGING)/Contents/Resources/share/doc/$(NAME)
	for IMG in $(wildcard Documentation/images/*); do \
		$(INSTALL) -m 644 $$IMG $(MACOSXSTAGING)/Contents/Resources/share/doc/$(NAME)/images; \
	done
	$(GTK_MAC_BUNDLER) packaging/macosx/$(NAME).bundle

sign-macosx-bundle: all
	codesign -s "3A8CE62A483083EDEA5581A61E770EC1FA8BECE8" /Applications/$(CAPITALIZED_NAME).app/Contents/MacOS/$(NAME)-bin

install-cross-windows: all
	$(INSTALL) -d -m 755 $(WINDOWSSTAGING)/share/locale
	for MSG in $(WINMSGDIRS); do\
		$(INSTALL) -d -m 755 $(WINDOWSSTAGING)/$$MSG;\
		$(INSTALL) $(CROSS_PATH)/$$MSG/* $(WINDOWSSTAGING)/$$MSG;\
	done
	for LOC in $(wildcard share/locale/*/LC_MESSAGES); do \
		$(INSTALL) -d -m 755 $(WINDOWSSTAGING)/$$LOC; \
		$(INSTALL) $$LOC/$(NAME).mo $(WINDOWSSTAGING)/$$LOC/$(NAME).mo; \
	done
	$(INSTALL) -d -m 755 $(WINDOWSSTAGING)/share/doc/$(NAME)
	$(INSTALL) -m 644 Documentation/user-manual.html $(WINDOWSSTAGING)/share/doc/$(NAME)
	for IMG in $(wildcard Documentation/images/*); do \
		$(INSTALL) -m 644 $$IMG $(WINDOWSSTAGING)/share/doc/$(NAME)/images; \
	done


create-windows-installer: all $(NSIFILE) install-cross-windows
	$(MAKENSIS) $(NSIFILE)

$(NSIFILE): $(NSIINPUTFILE)
	$(shell cat $(NSIINPUTFILE) | sed -e 's/VERSIONTOKEN/$(VERSION_STRING)/;s/PRODVTOKEN/$(PRODVERSION_STRING)/' > $(NSIFILE))

$(INFOPLIST): $(INFOPLISTINPUT)
	$(shell cat $(INFOPLISTINPUT) | sed -e 's/CFBUNDLEVERSION_TOKEN/$(CFBUNDLEVERSION_STRING)/' > $(INFOPLIST))

# Transifex merge the translations
update-po-files:
	xgettext -o po/$(NAME)-new.pot -s -k_ -kN_ -ktr --keyword=C_:1c,2  --add-comments="++GETTEXT" *.c qt-ui/*.cpp
	tx push -s
	tx pull -af

MOCFLAGS = $(filter -I%, $(CXXFLAGS) $(EXTRA_FLAGS)) $(filter -D%, $(CXXFLAGS) $(EXTRA_FLAGS))

%.o: %.c
	@$(PRETTYECHO) '    CC' $<
	@mkdir -p .dep/$(@D)
	$(COMPILE_PREFIX)$(CC) $(CFLAGS) $(EXTRA_FLAGS) -MD -MF .dep/$@.dep -c -o $@ $<

%.o: %.cpp $(UIC_HEADERS)
	@$(PRETTYECHO) '    CXX' $<
	@mkdir -p .dep/$(@D)
	$(COMPILE_PREFIX)$(CXX) $(CXXFLAGS) $(EXTRA_FLAGS) -I.uic -Iqt-ui -MD -MF .dep/$@.dep -c -o $@ $<

# This rule is for running the moc on QObject subclasses defined in the .h
# files.
%.moc.cpp: %.h
	@$(PRETTYECHO) '    MOC' $<
	$(COMPILE_PREFIX)$(MOC) $(MOCFLAGS) $< -o $@

# This rule is for running the moc on QObject subclasses defined in the .cpp
# files; remember to #include "<file>.moc" at the end of the .cpp file, or
# you'll get linker errors ("undefined vtable for...")
%.moc: %.cpp
	@$(PRETTYECHO) '    MOC' $<
	$(COMPILE_PREFIX)$(MOC) -i $(MOCFLAGS) $< -o $@

# This creates the Qt resource sources.
%.qrc.cpp: %.qrc
	@$(PRETTYECHO) '    RCC' $<
	$(COMPILE_PREFIX)$(RCC) $< -o $@
%.qrc:

# This creates the ui headers.
ui_%.h: %.ui .uic
	@$(PRETTYECHO) '    UIC' $<
	@mkdir -p .uic/qt-ui
	$(COMPILE_PREFIX)$(UIC) $< -o .uic/$@

.uic:
	$(COMPILE_PREFIX)mkdir $@

share/locale/%.UTF-8/LC_MESSAGES/$(NAME).mo: po/%.po po/%.aliases
	@$(PRETTYECHO) '    MSGFMT' $*.po
	@mkdir -p $(dir $@)
	$(COMPILE_PREFIX)msgfmt -c -o $@ po/$*.po
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
	rm -f $(ALL_OBJS) *~ $(NAME) $(NAME).exe po/*~ po/$(NAME)-new.pot \
		$(VERSION_FILE) $(HEADERS_NEEDING_MOC:.h=.moc) *.moc qt-ui/*.moc .uic/*.h
	rm -f $(RESOURCES:.qrc=.qrc.cpp)
	rm -rf share

confclean: clean
	rm -f $(CONFIGFILE)
	rm -rf .dep .uic

distclean: confclean
	rm -f $(CREATOR_FILES)

release:
	@scripts/check-version -cr $(VERSION_STRING)
	git archive --prefix $(CAPITALIZED_NAME)-$(VERSION_STRING)/ \
		    --output $(CAPITALIZED_NAME)-$(VERSION_STRING).tgz \
		    v$(VERSION_STRING)

.PHONY: creator-files
creator-files: $(CREATOR_FILES)
$(NAME).files: Makefile $(CONFIGFILE)
	echo $(wildcard *.h qt-ui/*.h qt-ui/*.ui) $(HEADERS) $(SOURCES) | tr ' ' '\n' | sort | uniq > $(NAME).files
	{ echo Makefile; echo Rules.mk; echo Configure.mk; } >> $(NAME).files
$(NAME).config: Makefile $(CONFIGFILE)
	echo $(patsubst -D%,%,$(filter -D%, $(CXXFLAGS) $(CFLAGS) $(EXTRA_FLAGS))) | tr ' ' '\n' | sort | uniq > $(NAME).config
$(NAME).includes: Makefile $(CONFIGFILE)
	echo $$PWD > $(NAME).includes
	echo $(patsubst -I%,%,$(filter -I%, $(CXXFLAGS) $(CFLAGS) $(EXTRA_FLAGS))) | tr ' ' '\n' | sort | uniq >> $(NAME).includes
$(NAME).creator:
	echo '[General]' > $(NAME).creator

ifneq ($(CONFIGURED)$(CONFIGURING),)
.dep/%.o.dep: %.cpp
	@mkdir -p $(@D)
	@$(CXX) $(CXXFLAGS) $(EXTRA_FLAGS) -MM -MG -MF $@ -MT $(<:.cpp=.o) -c $<
endif

DEPS = $(addprefix .dep/,$(C_SOURCES:.c=.o.dep) $(CXX_SOURCES:.cpp=.o.dep))
-include $(DEPS)
