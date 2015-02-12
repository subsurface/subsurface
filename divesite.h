#ifndef DIVESITE_H
#define DIVESITE_H

#include "units.h"
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

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

/* iterate over each dive site */
#define for_each_dive_site(_i, _x) \
	for ((_i) = 0; ((_x) = get_dive_site(_i)) != NULL; (_i)++)

static inline struct dive_site *get_dive_site_by_uuid(uint32_t uuid)
{
	int i;
	struct dive_site *ds;
	for_each_dive_site (i, ds)
		if (ds->uuid == uuid)
			return get_dive_site(i);
	return NULL;
}

/* there could be multiple sites of the same name - return the first one */
static inline uint32_t get_dive_site_uuid_by_name(const char *name)
{
	int i;
	struct dive_site *ds;
	for_each_dive_site (i, ds)
		if (ds->name == name)
			return ds->uuid;
	return 0;
}

struct dive_site *alloc_dive_site();
uint32_t create_dive_site(const char *name);
uint32_t create_dive_site_with_gps(const char *name, degrees_t latitude, degrees_t longitude);
uint32_t dive_site_uuid_by_name(const char *name);
struct dive_site *get_or_create_dive_site_by_uuid(uint32_t uuid);

#ifdef __cplusplus
}
#endif
#endif // DIVESITE_H
