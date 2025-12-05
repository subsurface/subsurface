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

# Compute version information once for all targets
VERSION := $(shell git log -1 --pretty=format:%H . | xargs ../scripts/get-version-for-changeset.sh)
ifeq ($(VERSION),)
VERSION := pre-6.0.0
endif
VERSION_DATE := $(shell git log -1 --pretty=format:%ad --date=short .)

.PHONY: all doc show clean

.NOTPARALLEL:

mkdir = @mkdir -p $(@D)

all: $(HTMLDOCS) $(TEXTDOCS) $(PDFDOCS)

doc: $(HTMLDOCS)

$(OUT)/images: images mobile-images
	@mkdir -p $(OUT)/
	@cp -r images $(OUT)/
	@cp -r mobile-images $(OUT)/

$(OUT)/%.text: $(OUT)/%.html
	$(mkdir)
	w3m -dump -T text/html $< > $@

$(OUT)/%.pdf: %.txt
	$(mkdir)
	$(A2X) -a version=$(VERSION) -a version-date=$(VERSION_DATE) -o $(OUT)/$(basename $(notdir $<)).pdf $<

$(OUT)/%.html: %.txt | $(OUT)/images
	$(mkdir)
	$(ASCIIDOC) -a toc=left -a toc-position=left -a themedir=$(PWD) -a theme=$(THEME) -a version=$(VERSION) -a version-date=$(VERSION_DATE) -o $@ $<

# for the web facing version of the desktop manual, just use the post-processing script
user-manual.html.wp: $(OUT)/user-manual.html
	$(mkdir)
	bash ../scripts/prep-manual.sh $<

# for the web facing version of the mobile manual, re-create the HTML with only one level in
# the table of context and then post process
mobile-manual-v3.html.wp: mobile-manual-v3.txt
	$(mkdir)
	$(ASCIIDOC) -a toc=left -a toclevels=1 -a themedir=$(PWD) -a theme=$(THEME) -a version="$(VERSION)" -a version-date="$(VERSION_DATE)" -o $(<:%.txt=%.html.tmp) $< && \
		bash ../scripts/prep-manual.sh $(<:%.txt=%.html.tmp) && \
		rm $(<:%.txt=%.html.tmp) && \
		mv $(<:%.txt=%.html.tmp.wp) $(OUT)/$@

show: $(HTMLDOC)
	$(BROWSER) $<

clean:
	rm -f *~ *.xml docbook-xsl.css
	rm -rf $(OUT)/
