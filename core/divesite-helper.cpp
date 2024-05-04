// SPDX-License-Identifier: GPL-2.0
#include "divesite.h"
#include "pref.h"
#include "gettextfromc.h"

QString constructLocationTags(taxonomy_data &taxonomy, bool for_maintab)
{
	QString locationTag;

	if (taxonomy.empty())
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
		for (auto const &t: taxonomy) {
			if (t.category == prefs.geocoding.category[i]) {
				if (!t.value.empty()) {
					locationTag += connector + QString::fromStdString(t.value);
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
