#ifndef DIVESITE_H
#define DIVESITE_H

#include "units.h"
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
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

struct dive_site *alloc_dive_site();
void delete_dive_site(uint32_t id);
uint32_t create_dive_site(const char *name);
uint32_t create_dive_site_with_gps(const char *name, degrees_t latitude, degrees_t longitude);
uint32_t get_dive_site_uuid_by_name(const char *name, struct dive_site **dsp);
uint32_t get_dive_site_uuid_by_gps(degrees_t latitude, degrees_t longitude, struct dive_site **dsp);
uint32_t get_dive_site_uuid_by_gps_proximity(degrees_t latitude, degrees_t longitude, int distance, struct dive_site **dsp);
bool dive_site_is_empty(struct dive_site *ds);

#ifdef __cplusplus
}
#endif
#endif // DIVESITE_H
