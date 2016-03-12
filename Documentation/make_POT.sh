#!/bin/bash
#
# Author(s): Guillaume GARDET <guillaume.gardet@free.fr>
#
# History:
# 	- 2016-03-12: Generate 2 POT files: one for mobile-manual and another for user-manual
#	- 2015-01-14:	Initial release
#
# Package deps:	- po4a (for po4a-gettextize)
# 		- perl-Unicode-LineBreak

# Some vars
Files_to_translate="mobile-manual.txt user-manual.txt"
POT_files_folder="./50-pot"

for File_to_translate in $Files_to_translate; do
	POT_name=subsurface-$(basename $File_to_translate ".txt").pot
	#Generate a POT file *.txt file in current folder
	cmd="po4a-gettextize --msgid-bugs-address subsurface@subsurface-divelog.org \
		--package-name subsurface-manual -o porefs=full,nowrap \
		-f asciidoc -M UTF-8 -m $File_to_translate -p $POT_files_folder/$POT_name"
	echo "Generating POT file:"
	echo $cmd
	$cmd
done