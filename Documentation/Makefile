ASCIIDOC=asciidoc
A2X=a2x
BROWSER=firefox

doc: user-manual.html

show: user-manual.html
	$(BROWSER) user-manual.html

user-manual.html: user-manual.txt
	$(ASCIIDOC) user-manual.txt

# Alternatively::
user-manual.xhtml: user-manual.txt
	$(A2X) --icons -f xhtml user-manual.txt

user-manual.text: user-manual.txt
	$(A2X) -f text user-manual.txt

user-manual.pdf: user-manual.txt
	$(A2X) -f pdf user-manual.txt

clean:
	rm -f user-manual.html user-manual.pdf user-manual.text
