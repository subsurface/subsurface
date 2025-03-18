DOCNAMES = user-manual user-manual_es user-manual_fr user-manual_nl mobile-manual-v3 mobile-manual_es mobile-manual_fr

OUT ?= output
OUT := $(OUT)/

HTMLDOCS = $(patsubst %,$(OUT)%.html,$(DOCNAMES))
TEXTDOCS = $(patsubst %,$(OUT)%.text,$(DOCNAMES))
PDFDOCS = $(patsubst %,$(OUT)%.pdf,$(DOCNAMES))

ASCIIDOC = asciidoc
A2X = a2x
BROWSER = firefox
THEME = compact_subsurface
PWD = $(realpath .)

.PHONY: all doc images show clean

mkdir = @mkdir -p $(@D)

all: $(HTMLDOCS) $(TEXTDOCS) $(PDFDOCS) images

doc: $(HTMLDOCS) images

images:
	@cp -r images $(OUT)
	@cp -r mobile-images $(OUT)

$(OUT)%.text: %.txt
	$(mkdir)
	$(A2X) -f text --destination-dir=$(OUT) $<

$(OUT)%.pdf: %.txt
	$(mkdir)
	-$(A2X) --dblatex-opts "-P latex.output.revhistory=0" -f pdf --destination-dir=$(OUT) $<

$(OUT)%.html: %.txt
	$(mkdir)
	$(ASCIIDOC) -a toc -a toclevels=3 -a themedir=$(PWD) -a theme=$(THEME) -o $@ $<

# for the web facing version of the desktop manual, just use the post-processing script
user-manual.html.wp: $(OUT)user-manual.html
	$(mkdir)
	bash ../scripts/prep-manual.sh $<

# for the web facing version of the mobile manual, re-create the HTML with only one level in
# the table of context and then post process
mobile-manual-v3.html.wp: mobile-manual-v3.txt
	$(mkdir)
	$(ASCIIDOC) -a toc -a toclevels=1 -a themedir=$(PWD) -a theme=$(THEME) -o $(<:%.txt=%.html.tmp) $< && \
		bash ../scripts/prep-manual.sh $(<:%.txt=%.html.tmp) && \
		rm $(<:%.txt=%.html.tmp) && \
		mv $(<:%.txt=%.html.tmp.wp) $(OUT)$@

show: $(HTMLDOC)
	$(BROWSER) $<

clean:
	rm -f *~ *.xml docbook-xsl.css
	rm -rf $(OUT)
