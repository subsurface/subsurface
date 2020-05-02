#!/bin/bash
#
# simply create some stats
if [ ! -d translations ] && [ ! -f translations/subsurface_source.ts ] ; then
	echo "please start from the Subsurface source directory"
	exit 1
fi

cd translations || exit 1

STRINGS=$(grep -c source subsurface_source.ts)

for tr in subsurface_*.ts
do
	[ "$tr" = "subsurface_source.ts" ] && continue
	[ "$tr" = "subsurface_en_US.ts" ] && continue
	MISSING=$(grep -c "translation.*unfinished" "$tr")
	PERCENT=$(( (STRINGS - MISSING) * 100 / STRINGS ))
	printf "%3d %s\n" "$PERCENT" "$tr"
done | sort -n -r

