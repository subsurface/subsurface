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
	@echo "if asciidoc isn't found no HTML documentation is produced but Subsurface is still functional"
	$(ASCIIDOC) -a toc -a toclevels=3 -a themedir=$(PWD) -a theme=$(THEME) -a stylesdir=$(PWD) $< || true

# Alternatively::
$(OUT)$(DOCNAME).xhtml: $(DOCSOURCE)
	$(A2X) --icons -f xhtml $<

show: $(HTMLDOC)
	$(BROWSER) $<

clean:
	rm -f *~ docbook-xsl.css $(HTMLDOC) $(addprefix $(DOCNAME).,pdf text)
