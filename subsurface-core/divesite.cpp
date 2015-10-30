#include "divesite.h"
#include "pref.h"

QString constructLocationTags(uint32_t ds_uuid)
{
	QString locationTag;
	struct dive_site *ds = get_dive_site_by_uuid(ds_uuid);

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
