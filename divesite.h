#ifndef DIVESITE_H
#define DIVESITE_H

#include "units.h"
#include "taxonomy.h"
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
	struct taxonomy_data taxonomy;
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

void dive_site_table_sort();
struct dive_site *alloc_or_get_dive_site(uint32_t uuid);
int nr_of_dives_at_dive_site(uint32_t uuid, bool select_only);
bool is_dive_site_used(uint32_t uuid, bool select_only);
void delete_dive_site(uint32_t id);
uint32_t create_dive_site(const char *name, timestamp_t divetime);
uint32_t create_dive_site_from_current_dive(const char *name);
uint32_t create_dive_site_with_gps(const char *name, degrees_t latitude, degrees_t longitude, timestamp_t divetime);
uint32_t get_dive_site_uuid_by_name(const char *name, struct dive_site **dsp);
uint32_t get_dive_site_uuid_by_gps(degrees_t latitude, degrees_t longitude, struct dive_site **dsp);
uint32_t get_dive_site_uuid_by_gps_and_name(char *name, degrees_t latitude, degrees_t longitude);
uint32_t get_dive_site_uuid_by_gps_proximity(degrees_t latitude, degrees_t longitude, int distance, struct dive_site **dsp);
bool dive_site_is_empty(struct dive_site *ds);
void copy_dive_site(struct dive_site *orig, struct dive_site *copy);
void clear_dive_site(struct dive_site *ds);
unsigned int get_distance(degrees_t lat1, degrees_t lon1, degrees_t lat2, degrees_t lon2);
uint32_t find_or_create_dive_site_with_name(const char *name, timestamp_t divetime);
void merge_dive_sites(uint32_t ref, uint32_t *uuids, int count);

#define INVALID_DIVE_SITE_NAME "development use only - not a valid dive site name"

#ifdef __cplusplus
}
#endif

#endif // DIVESITE_H
