/* divesite.c */
#include "divesite.h"
#include "dive.h"

struct dive_site_table dive_site_table;

/* try to create a uniqe ID - fingers crossed */
static uint32_t dive_site_getUniqId()
{
	uint32_t id = 0;

	while (id == 0 || get_dive_site_by_uuid(id))
		id = random() + random();

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
	return ds;
}

/* allocate a new site and add it to the table */
uint32_t create_dive_site(const char *name, degrees_t latitude, degrees_t longitude)
{
	struct dive_site *ds = alloc_dive_site();
	ds->uuid = dive_site_getUniqId();
	ds->name = copy_string(name);
	ds->latitude = latitude;
	ds->longitude = longitude;

	return ds->uuid;
}

/* this either returns the uuid for a site with that name or creates an entry */
uint32_t dive_site_uuid_by_name(const char *name)
{
	uint32_t id = get_dive_site_uuid_by_name(name);
	if (id == 0)
		id = create_dive_site(name, (degrees_t){0}, (degrees_t){0});

	return id;
}
