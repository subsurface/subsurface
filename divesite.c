/* divesite.c */
#include "divesite.h"
#include "dive.h"

struct dive_site_table dive_site_table;

/* there could be multiple sites of the same name - return the first one */
uint32_t get_dive_site_uuid_by_name(const char *name, struct dive_site **dsp)
{
	int i;
	struct dive_site *ds;
	for_each_dive_site (i, ds) {
		if (same_string(ds->name, name)) {
			if (dsp)
				*dsp = ds;
			return ds->uuid;
		}
	}
	return 0;
}

/* there could be multiple sites at the same GPS fix - return the first one */
uint32_t get_dive_site_uuid_by_gps(degrees_t latitude, degrees_t longitude, struct dive_site **dsp)
{
	int i;
	struct dive_site *ds;
	for_each_dive_site (i, ds) {
		if (ds->latitude.udeg == latitude.udeg && ds->longitude.udeg == longitude.udeg) {
			if (dsp)
				*dsp = ds;
			return ds->uuid;
		}
	}
	return 0;
}

/* try to create a uniqe ID - fingers crossed */
static uint32_t dive_site_getUniqId()
{
	uint32_t id = 0;

	while (id == 0 || get_dive_site_by_uuid(id)) {
		id = rand() & 0xff;
		id |= (rand() & 0xff) << 8;
		id |= (rand() & 0xff) << 16;
		id |= (rand() & 0xff) << 24;
	}

	return id;
}

struct dive_site *alloc_dive_site()
{
	int nr = dive_site_table.nr, allocated = dive_site_table.allocated;
	struct dive_site **sites = dive_site_table.dive_sites;

	if (nr >= allocated) {
		allocated = (nr + 32) * 3 / 2;
		sites = realloc(sites, allocated * sizeof(struct dive_site *));
		if (!sites)
			exit(1);
		dive_site_table.dive_sites = sites;
		dive_site_table.allocated = allocated;
	}
	struct dive_site *ds = calloc(1, sizeof(*ds));
	if (!ds)
		exit(1);
	sites[nr] = ds;
	dive_site_table.nr = nr + 1;
	ds->uuid = dive_site_getUniqId();
	return ds;
}

void delete_dive_site(uint32_t id)
{
	int nr = dive_site_table.nr;
	for (int i = 0; i < nr; i++) {
		struct dive_site *ds = get_dive_site(i);
		if (ds->uuid == id) {
			free(ds->name);
			free(ds->notes);
			free(ds);
			if (nr - 1 > i)
				memmove(&dive_site_table.dive_sites[i],
					&dive_site_table.dive_sites[i+1],
					(nr - 1 - i) * sizeof(dive_site_table.dive_sites[0]));
			dive_site_table.nr = nr - 1;
			break;
		}
	}
}

/* allocate a new site and add it to the table */
uint32_t create_dive_site(const char *name)
{
	struct dive_site *ds = alloc_dive_site();
	ds->name = copy_string(name);

	return ds->uuid;
}

/* same as before, but with GPS data */
uint32_t create_dive_site_with_gps(const char *name, degrees_t latitude, degrees_t longitude)
{
	struct dive_site *ds = alloc_dive_site();
	ds->uuid = dive_site_getUniqId();
	ds->name = copy_string(name);
	ds->latitude = latitude;
	ds->longitude = longitude;

	return ds->uuid;
}

/* a uuid is always present - but if all the other fields are empty, the dive site is pointless */
bool dive_site_is_empty(struct dive_site *ds)
{
	return same_string(ds->name, "") &&
	       same_string(ds->description, "") &&
	       same_string(ds->notes, "") &&
	       ds->latitude.udeg == 0 &&
	       ds->longitude.udeg == 0;
}
