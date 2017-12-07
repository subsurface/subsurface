#
# in order for "normal" developers not to have to install asciidoc (which
# brings in a couple hundred MB of dependencies) we now include both the
# user-manual.txt and a copy of the generated .html file in git
#
# in order to avoid unnecessary thrash with minor version differences, when
# submitting patches to the user manual, only submit the changes to the .txt
# file - the maintainer will recreate the .html.git file

DOCNAMES = ReleaseNotes

HTMLDOCS = $(patsubst %,$(OUT)%.html,$(DOCNAMES))
TEXTDOCS = $(patsubst %,$(OUT)%.text,$(DOCNAMES))
PDFDOCS = $(patsubst %,$(OUT)%.pdf,$(DOCNAMES))

ASCIIDOC = asciidoc
A2X = a2x
BROWSER = firefox
THEME = ../Documentation/compact_subsurface
PWD = $(realpath .)

all: $(HTMLEDOCS) $(TEXTDOCS) $(PDFDOCS)

doc: $(HTMLDOCS)

$(OUT)%.text: %.txt
	$(A2X) -f text $<

$(OUT)%.pdf: %.txt
	-$(A2X) --dblatex-opts "-P latex.output.revhistory=0" -f pdf $<

$(OUT)%.html: %.txt
	@echo "if asciidoc isn't found the html file included in the sources is copied"
	$(ASCIIDOC) -a themedir=$(PWD) -a theme=$(THEME) -a stylesdir=$(PWD) \
		-o $@ $< || cp $(<:%.txt=%.html.git) $@

# Alternatively::
$(OUT)$(DOCNAME).xhtml: $(DOCSOURCE)
	$(A2X) --icons -f xhtml $<

show: $(HTMLDOC)
	$(BROWSER) $<

clean:
	rm -f *~ *.xml docbook-xsl.css $(HTMLDOCS) $(TEXTDOCS) $(PDFDOCS)
