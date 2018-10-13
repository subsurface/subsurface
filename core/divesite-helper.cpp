// SPDX-License-Identifier: GPL-2.0
#include "divesite.h"
#include "pref.h"
#include "gettextfromc.h"

QString constructLocationTags(struct taxonomy_data *taxonomy, bool for_maintab)
{
	QString locationTag;

	if (!taxonomy->nr)
		return locationTag;

	/* Check if the user set any of the 3 geocoding categories */
	bool prefs_set = false;
	for (int i = 0; i < 3; i++) {
		if (prefs.geocoding.category[i] != TC_NONE)
			prefs_set = true;
	}

	if (!prefs_set && !for_maintab) {
		locationTag = QString("<small><small>") + gettextFromC::tr("No dive site layout categories set in preferences!") +
			QString("</small></small>");
		return locationTag;
	}
	else if (!prefs_set)
		return locationTag;

	if (for_maintab)
		locationTag = QString("<small><small>(") + gettextFromC::tr("Tags") + QString(": ");
	else 
		locationTag = QString("<small><small>");
	QString connector;
	for (int i = 0; i < 3; i++) {
		if (prefs.geocoding.category[i] == TC_NONE)
			continue;
		for (int j = 0; j < taxonomy->nr; j++) {
			if (taxonomy->category[j].category == prefs.geocoding.category[i]) {
				QString tag = taxonomy->category[j].value;
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
