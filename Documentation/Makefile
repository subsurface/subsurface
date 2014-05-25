#
# in order for "normal" developers not to have to install asciidoc (which
# brings in a couple hundred MB of dependencies) we now include both the
# user-manual.txt and a copy of the generated .html file in git
#
# in order to avoid unnecessary thrash with minor version differences, when
# submitting patches to the user manual, only submit the changes to the .txt
# file - the maintainer will recreate the .html.git file

DOCNAME = user-manual
DOCSOURCE = $(DOCNAME).txt
HTMLDOC = $(DOCNAME).html
ASCIIDOC = asciidoc
A2X = a2x
BROWSER = firefox
THEME = compact_subsurface
PWD = $(realpath .)

all: doc $(addprefix $(DOCNAME).,pdf text)

doc: $(OUT)$(HTMLDOC)

$(OUT)$(DOCNAME).text: $(DOCSOURCE)
	$(A2X) -f text $<

$(OUT)$(DOCNAME).pdf: $(DOCSOURCE)
	$(A2X) -f pdf $<

$(OUT)$(HTMLDOC): $(DOCSOURCE)
	@echo "if asciidoc isn't found the html file included in the sources is copied"
	$(ASCIIDOC) -a toc -a toclevels=3 -a themedir=$(PWD) -a theme=$(THEME) -a stylesdir=$(PWD) \
		-o $(OUT)$(HTMLDOC) $< || cp $(HTMLDOC).git $(OUT)$(HTMLDOC)

# Alternatively::
$(OUT)$(DOCNAME).xhtml: $(DOCSOURCE)
	$(A2X) --icons -f xhtml $<

show: $(HTMLDOC)
	$(BROWSER) $<

clean:
	rm -f *~ docbook-xsl.css $(HTMLDOC) $(addprefix $(DOCNAME).,pdf text)
