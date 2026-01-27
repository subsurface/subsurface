DOCNAMES = user-manual user-manual_es user-manual_fr user-manual_nl user-manual_de mobile-manual-v3 mobile-manual_es mobile-manual_fr mobile-manual_de

OUT ?= output

HTMLDOCS = $(patsubst %,$(OUT)/%.html,$(DOCNAMES))
TEXTDOCS = $(patsubst %,$(OUT)/%.text,$(DOCNAMES))
PDFDOCS = $(patsubst %,$(OUT)/%.pdf,$(DOCNAMES))

ASCIIDOC = asciidoctor
A2X = asciidoctor-pdf
BROWSER = firefox
THEME = compact_subsurface
PWD = $(realpath .)

# Track the last commit hash affecting Documentation/ (not the version string)
# FORCE ensures this rule always runs to check for git changes
.latest_change_git_hash: FORCE
	@NEW_HASH=$$(git log -1 --pretty=format:%H . 2>/dev/null || echo "unknown"); \
	CURRENT=$$(cat $@ 2>/dev/null || echo ""); \
	if [ "$$NEW_HASH" != "$$CURRENT" ]; then \
		echo "$$NEW_HASH" > $@; \
	fi

# Rebuild version file only when Documentation/ commit history changes
.latest_change_version: .latest_change_git_hash
	@git log -1 --pretty=format:%H . 2>/dev/null | xargs ../scripts/get-version-for-changeset.sh > $@ 2>/dev/null || echo "<unknown>" > $@

# Read version from cache (fast; .latest_change_version regenerates when git state changes)
VERSION = $(shell test -f .latest_change_version && cat .latest_change_version || echo "<unknown>")
VERSION_DATE = $(shell git log -1 --pretty=format:%ad --date=short . 2>/dev/null || echo "<unknown>")
DISPLAY_VERSION = $(VERSION)
ifeq ($(DISPLAY_VERSION),<unknown>)
DISPLAY_VERSION = &lt;unknown&gt;
endif

.PHONY: all doc show clean

mkdir = @mkdir -p $(@D)

all: .latest_change_version $(HTMLDOCS) $(TEXTDOCS) $(PDFDOCS)

doc: $(HTMLDOCS)

$(OUT)/images: images mobile-images
	@mkdir -p $(OUT)/
	@cp -r images $(OUT)/
	@cp -r mobile-images $(OUT)/

$(OUT)/%.text: $(OUT)/%.html
	$(mkdir)
	w3m -dump -T text/html $< > $@


$(OUT)/%.pdf: %.txt .latest_change_version
	$(mkdir)
	$(A2X) -a version="$(DISPLAY_VERSION)" -a version-date="$(VERSION_DATE)" -o $(OUT)/$(basename $(notdir $<)).pdf $<

$(OUT)/%.html: %.txt .latest_change_version | $(OUT)/images
	$(mkdir)
	$(ASCIIDOC) -a toc=left -a toc-position=left -a themedir=$(PWD) -a theme=$(THEME) -a version="$(DISPLAY_VERSION)" -a version-date="$(VERSION_DATE)" -o $@ $<

# for the web facing version of the desktop manual, just use the post-processing script
user-manual.html.wp: $(OUT)/user-manual.html
	$(mkdir)
	bash ../scripts/prep-manual.sh $<

# for the web facing version of the mobile manual, re-create the HTML with only one level in
# the table of context and then post process
mobile-manual-v3.html.wp: mobile-manual-v3.txt
	$(mkdir)
	$(ASCIIDOC) -a toc=left -a toclevels=1 -a themedir=$(PWD) -a theme=$(THEME) -a version="$(DISPLAY_VERSION)" -a version-date="$(VERSION_DATE)" -o $(<:%.txt=%.html.tmp) $< && \
		bash ../scripts/prep-manual.sh $(<:%.txt=%.html.tmp) && \
		rm $(<:%.txt=%.html.tmp) && \
		mv $(<:%.txt=%.html.tmp.wp) $(OUT)/$@

show: $(HTMLDOC)
	$(BROWSER) $<

clean:
	rm -f *~ *.xml docbook-xsl.css .latest_change_version .latest_change_git_hash
	rm -rf $(OUT)/

.PHONY: FORCE
FORCE:
