// SPDX-License-Identifier: GPL-2.0
#include "divesite.h"
#include "pref.h"

QString constructLocationTags(struct dive_site *ds)
{
	QString locationTag;

	if (!ds || !ds->taxonomy.nr)
		return locationTag;

	locationTag = "<small><small>(tags: ";
	QString connector;
	for (int i = 0; i < 3; i++) {
		if (prefs.geocoding.category[i] == TC_NONE)
			continue;
		for (int j = 0; j < TC_NR_CATEGORIES; j++) {
			if (ds->taxonomy.category[j].category == prefs.geocoding.category[i]) {
				QString tag = ds->taxonomy.category[j].value;
				if (!tag.isEmpty()) {
					locationTag += connector + tag;
					connector = " / ";
				}
				break;
			}
		}
	}

	locationTag += ")</small></small>";
	return locationTag;
}
