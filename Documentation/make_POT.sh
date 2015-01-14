#!/bin/bash
#
# Author(s): Guillaume GARDET <guillaume.gardet@free.fr>
#
# History:
#	- 2015-01-14:	Initial release
#
# Package deps:	- po4a (for po4a-gettextize)
# 		- perl-Unicode-LineBreak

# Some vars
File_to_translate="./user-manual.txt"
POT_files_folder="./50-pot"
POT_name="subsurface-manual.pot"

#Generate a POT file from user-manual.txt file in current folder
cmd="po4a-gettextize --msgid-bugs-address subsurface@subsurface-divelog.org \
	--package-name subsurface-manual -o porefs=full,nowrap \
	-f asciidoc -M UTF-8 -m $File_to_translate -p $POT_files_folder/$POT_name"
echo "Generating POT file:"
echo $cmd
$cmd
