#
# in order for "normal" developers not to have to install asciidoc (which
# brings in a couple hundred MB of dependencies) we now include both the
# user-manual.txt and a copy of the generated .html file in git
#
# in order to avoid unnecessary thrash with minor version differences, when
# submitting patches to the user manual, only submit the changes to the .txt
# file - the maintainer will recreate the .html.git file

DOCNAMES = user-manual user-manual_es user-manual_fr user-manual_ru user-manual_nl mobile-manual-v2 mobile-manual-v3

HTMLDOCS = $(patsubst %,$(OUT)%.html,$(DOCNAMES))
TEXTDOCS = $(patsubst %,$(OUT)%.text,$(DOCNAMES))
PDFDOCS = $(patsubst %,$(OUT)%.pdf,$(DOCNAMES))

ASCIIDOC = asciidoc
A2X = a2x
BROWSER = firefox
THEME = compact_subsurface
PWD = $(realpath .)

all: $(HTMLEDOCS) $(TEXTDOCS) $(PDFDOCS)

doc: $(HTMLDOCS)

$(OUT)%.text: %.txt
	$(A2X) -f text $<

$(OUT)%.pdf: %.txt
	-$(A2X) --dblatex-opts "-P latex.output.revhistory=0" -f pdf $<

$(OUT)%.html: %.txt
	@echo "if asciidoc isn't found the html file included in the sources is copied"
	$(ASCIIDOC) -a toc -a toclevels=3 -a themedir=$(PWD) -a theme=$(THEME) -o $@ $< || \
		cp $(<:%.txt=%.html.git) $@

# for the web facing version of the desktop manual, just use the post-processing script
user-manual.html.wp: user-manual.html
	bash ../scripts/prep-manual.sh $<

# for the web facing version of the mobile manual, re-create the HTML with only one level in
# the table of context and then post process
mobile-manual-v3.html.wp: mobile-manual-v3.txt
	$(ASCIIDOC) -a toc -a toclevels=1 -a themedir=$(PWD) -a theme=$(THEME) -o $(<:%.txt=%.html.tmp) $< && \
		bash ../scripts/prep-manual.sh $(<:%.txt=%.html.tmp) && \
		mv $(<:%.txt=%.html.tmp.wp) $@

# Alternatively::
$(OUT)$(DOCNAME).xhtml: $(DOCSOURCE)
	$(A2X) --icons -f xhtml $<

show: $(HTMLDOC)
	$(BROWSER) $<

clean:
	rm -f *~ *.xml docbook-xsl.css $(HTMLDOCS) $(TEXTDOCS) $(PDFDOCS)
