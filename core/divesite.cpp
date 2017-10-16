// SPDX-License-Identifier: GPL-2.0
#include "divesite.h"
#include "pref.h"

QString constructLocationTags(struct dive_site *ds, bool for_maintab)
{
	QString locationTag;

	if (!ds || !ds->taxonomy.nr)
		return locationTag;

	/* Check if the user set any of the 3 geocoding categories */
	bool prefs_set = false;
	for (int i = 0; i < 3; i++) {
		if (prefs.geocoding.category[i] != TC_NONE)
			prefs_set = true;
	}

	if (!prefs_set && !for_maintab) {
		locationTag = QString("<small><small>") + QObject::tr("No dive site layout categories set in preferences!") +
			QString("</small></small>");
		return locationTag;
	}
	else if (!prefs_set)
		return locationTag;

	if (for_maintab)
		locationTag = QString("<small><small>(") + QObject::tr("Tags") + QString(": ");
	else 
		locationTag = QString("<small><small>");
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

	if (for_maintab)
		locationTag += ")</small></small>";
	else
		locationTag += "</small></small>";
	return locationTag;
}
