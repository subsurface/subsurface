DOCNAME = user-manual
DOCSOURCE = $(DOCNAME).txt
HTMLDOC = $(DOCNAME).html
ASCIIDOC = asciidoc
A2X = a2x
BROWSER = firefox

all: $(HTMLDOC) $(addprefix $(DOCNAME).,pdf text)

doc: $(HTMLDOC)

show: $(HTMLDOC)
	$(BROWSER) $<

$(HTMLDOC): $(DOCSOURCE)
	$(ASCIIDOC) $<

# Alternatively::
$(DOCNAME).xhtml: $(DOCSOURCE)
	$(A2X) --icons -f xhtml $<

$(DOCNAME).text: $(DOCSOURCE)
	$(A2X) -f text $<

$(DOCNAME).pdf: $(DOCSOURCE)
	$(A2X) -f pdf $<

clean:
	rm -f *~ docbook-xsl.css $(HTMLDOC) $(addprefix $(DOCNAME).,pdf text)
	rm -fr images
