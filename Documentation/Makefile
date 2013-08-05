DOCNAME = user-manual
DOCSOURCE = $(DOCNAME).txt
HTMLDOC = $(DOCNAME).html
ASCIIDOC = asciidoc
A2X = a2x
BROWSER = firefox

all: doc $(addprefix $(DOCNAME).,pdf text)

doc: $(HTMLDOC)

$(DOCNAME).text: $(DOCSOURCE)
	$(A2X) -f text $<

$(DOCNAME).pdf: $(DOCSOURCE)
	$(A2X) -f pdf $<

$(HTMLDOC): $(DOCSOURCE)
	@echo "if asciidoc isn't found no HTML documentation is produced but Subsurface is still functional"
	$(ASCIIDOC) $< || true

# Alternatively::
$(DOCNAME).xhtml: $(DOCSOURCE)
	$(A2X) --icons -f xhtml $<

show: $(HTMLDOC)
	$(BROWSER) $<

clean:
	rm -f *~ docbook-xsl.css $(HTMLDOC) $(addprefix $(DOCNAME).,pdf text)
