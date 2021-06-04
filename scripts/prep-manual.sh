#!/bin/bash
cat $1 | \
	sed -e '1,/title/d;/<\/head>/,/<body/d;/<\/body/,/<\/html/d' | \
	sed -e '/^@media/,/^}/d' | \
	sed -e 's/src="images/src="\/images/g' | \
	sed -e 's/src="\.\/images/src="\/images/g' | \
	sed -e 's/src="mobile-images/src="\/mobile-images/g' | \
	html-minifier --collapse-whitespace --keep-closing-slash --minify-js --minify-css > $1.wp
