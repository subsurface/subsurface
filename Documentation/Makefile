DOCNAMES = user-manual user-manual_es user-manual_fr user-manual_nl user-manual_de mobile-manual-v3 mobile-manual_es mobile-manual_fr mobile-manual_de

OUT ?= output

HTMLDOCS = $(patsubst %,$(OUT)/%.html,$(DOCNAMES))
TEXTDOCS = $(patsubst %,$(OUT)/%.text,$(DOCNAMES))
PDFDOCS = $(patsubst %,$(OUT)/%.pdf,$(DOCNAMES))

ASCIIDOC = asciidoctor
A2X = a2x
BROWSER = firefox
THEME = compact_subsurface
PWD = $(realpath .)

.PHONY: all doc show clean

.NOTPARALLEL:

mkdir = @mkdir -p $(@D)

all: $(HTMLDOCS) $(TEXTDOCS) $(PDFDOCS)

doc: $(HTMLDOCS)

$(OUT)/images: images mobile-images
	@mkdir -p $(OUT)/
	@cp -r images $(OUT)/
	@cp -r mobile-images $(OUT)/

$(OUT)/%.text: %.txt
	$(mkdir)
	@VERSION=$(shell git log -1 --pretty=format:%H $< | xargs ../scripts/get-version-for-changeset.sh); \
	if [ -z "$$VERSION" ]; then \
		VERSION="pre-6.0.0"; \
	fi; \
	VERSION_DATE=$(shell git log -1 --pretty=format:%ad --date=short $<); \
	$(A2X) -f text -a version=$$VERSION -a version-date=$$VERSION_DATE --destination-dir=$(OUT)/ $<

$(OUT)/%.pdf: %.txt
	$(mkdir)
	@VERSION=$(shell git log -1 --pretty=format:%H $< | xargs ../scripts/get-version-for-changeset.sh); \
	if [ -z "$$VERSION" ]; then \
		VERSION="pre-6.0.0"; \
	fi; \
	VERSION_DATE=$(shell git log -1 --pretty=format:%ad --date=short $<); \
	$(A2X) --dblatex-opts "-P latex.output.revhistory=0" -f pdf -a version=$$VERSION -a version-date=$$VERSION_DATE --destination-dir=$(OUT)/ $<

$(OUT)/%.html: %.txt | $(OUT)/images
	$(mkdir)
	@VERSION=$(shell git log -1 --pretty=format:%H $< | xargs ../scripts/get-version-for-changeset.sh); \
	if [ -z "$$VERSION" ]; then \
		VERSION="pre-6.0.0"; \
	fi; \
	VERSION_DATE=$(shell git log -1 --pretty=format:%ad --date=short $<); \
	$(ASCIIDOC) -a toc=left -a toc-position=left -a themedir=$(PWD) -a theme=$(THEME) -a version=$$VERSION -a version-date=$$VERSION_DATE -o $@ $<

# for the web facing version of the desktop manual, just use the post-processing script
user-manual.html.wp: $(OUT)/user-manual.html
	$(mkdir)
	bash ../scripts/prep-manual.sh $<

# for the web facing version of the mobile manual, re-create the HTML with only one level in
# the table of context and then post process
mobile-manual-v3.html.wp: mobile-manual-v3.txt
	$(mkdir)
	@VERSION=$(shell git log -1 --pretty=format:%H $< | xargs ../scripts/get-version-for-changeset.sh); \
	if [ -z "$$VERSION" ]; then \
		VERSION="pre-6.0.0"; \
	fi; \
	VERSION_DATE=$(shell git log -1 --pretty=format:%ad --date=short $<); \
	$(ASCIIDOC) -a toc=left -a toclevels=1 -a themedir=$(PWD) -a theme=$(THEME) -a version="$$VERSION" -a version-date="$$VERSION_DATE" -o $(<:%.txt=%.html.tmp) $< && \
		bash ../scripts/prep-manual.sh $(<:%.txt=%.html.tmp) && \
		rm $(<:%.txt=%.html.tmp) && \
		mv $(<:%.txt=%.html.tmp.wp) $(OUT)/$@

show: $(HTMLDOC)
	$(BROWSER) $<

clean:
	rm -f *~ *.xml docbook-xsl.css
	rm -rf $(OUT)/
