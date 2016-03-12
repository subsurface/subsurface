#!/bin/bash
#
# Author(s): Guillaume GARDET <guillaume.gardet@free.fr>
#
# History:
# 	- 2016-03-12: Generate 2 PO files: one for mobile-manual and another for user-manual
# 	- 2015-01-14:	Initial release
#
# Known bugs:
# 	- Some list item indexes are lost during PO to ASCII conversion (see asciidoc warning messages)
#
# Package deps:	- po4a (for po4a-translate)
# 		- git (for git sync)
# 		- gettext-tools (for msginit and msgmerge)

# Some vars
Files_to_translate="mobile-manual.txt user-manual.txt"
POT_files_folder="./50-pot/"
langs="fr" 	# Language list which uses POT/PO files for translation
PO_filename_root="subsurface-manual"
translation_limit="0"	# Minimal translation percentage. Under this limit, no translated file is output.
dosync=1 # whether to pull new translations from git

for i in "$@"; do
	if [ "$i" = "--nosync" ]; then
		dosync=0
	fi
done

for lang in $langs; do
	PO_folder="$lang/po"

# Sync PO files from GIT server and update it using latest generated POT file (in case PO file not merged with latest POT)
	if [ "$dosync" = 1 ]; then
		echo "* Syncing PO file for '$lang'"
		git pull

		for file in $(ls $POT_files_folder/*.pot); do
			filename=$(basename $file ".pot").$lang.po
			if [ ! -f $PO_folder/$filename ]; then
				echo "** Initializing PO file for $lang"
				mkdir -p $PO_folder
				msginit -l $lang --input=$file --output-file=$PO_folder/$filename
			fi;
			echo "** Updating PO file for '$lang' from POT file"
			msgmerge --previous --lang=$lang --update $PO_folder/$filename $file
		done
	fi

# Generate translated ASCIIDOC files
	echo "* Generating ASCIIDOC files for '$lang'"

	for file in $Files_to_translate; do
		Translated_file=$(basename $file ".txt")_$lang.txt
		PO_name=subsurface-$(basename $file ".txt").$lang.po
		cmd="po4a-translate --keep $translation_limit -f asciidoc -M UTF-8 -m $file -p $PO_folder/$PO_name -l $Translated_file"
		echo $cmd
		$cmd
	done

done
