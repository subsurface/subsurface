#ifndef DIVESITE_H
#define DIVESITE_H

#include "units.h"
#include <stdlib.h>

struct dive_site
{
	uint32_t uuid;
	char *name;
	degrees_t latitude, longitude;
	char *description;
	char *notes;
};

struct dive_site_table {
	int nr, allocated;
	struct dive_site **dive_sites;
};

extern struct dive_site_table dive_site_table;

static inline struct dive_site *get_dive_site(int nr)
{
	if (nr >= dive_site_table.nr || nr < 0)
		return NULL;
	return dive_site_table.dive_sites[nr];
}

static inline struct dive_site *get_dive_site_by_uuid(uint32_t uuid)
{
	for (int i = 0; i < dive_site_table.nr; i++)
		if (get_dive_site(i)->uuid == uuid)
			return get_dive_site(i);
	return NULL;
}

/* there could be multiple sites of the same name - return the first one */
static inline struct dive_site *get_dive_site_by_name(const char *name)
{
	for (int i = 0; i < dive_site_table.nr; i++)
		if (get_dive_site(i)->name == name)
			return get_dive_site(i);
	return NULL;
}

#endif // DIVESITE_H
