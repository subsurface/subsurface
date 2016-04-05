/* divesite.c */
#include "divesite.h"
#include "dive.h"
#include "divelist.h"

#include <math.h>

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


/* to avoid a bug where we have two dive sites with different name and the same GPS coordinates
 * and first get the gps coordinates (reading a V2 file) and happen to get back "the other" name,
 * this function allows us to verify if a very specific name/GPS combination already exists */
uint32_t get_dive_site_uuid_by_gps_and_name(char *name, degrees_t latitude, degrees_t longitude)
{
	int i;
	struct dive_site *ds;
	for_each_dive_site (i, ds) {
		if (ds->latitude.udeg == latitude.udeg && ds->longitude.udeg == longitude.udeg && same_string(ds->name, name))
			return ds->uuid;
	}
	return 0;
}

// Calculate the distance in meters between two coordinates.
unsigned int get_distance(degrees_t lat1, degrees_t lon1, degrees_t lat2, degrees_t lon2)
{
	double lat2_r = udeg_to_radians(lat2.udeg);
	double lat_d_r = udeg_to_radians(lat2.udeg-lat1.udeg);
	double lon_d_r = udeg_to_radians(lon2.udeg-lon1.udeg);

	double a = sin(lat_d_r/2) * sin(lat_d_r/2) +
		cos(lat2_r) * cos(lat2_r) * sin(lon_d_r/2) * sin(lon_d_r/2);
	double c = 2 * atan2(sqrt(a), sqrt(1.0 - a));

	// Earth radious in metres
	return 6371000 * c;
}

/* find the closest one, no more than distance meters away - if more than one at same distance, pick the first */
uint32_t get_dive_site_uuid_by_gps_proximity(degrees_t latitude, degrees_t longitude, int distance, struct dive_site **dsp)
{
	int i;
	int uuid = 0;
	struct dive_site *ds;
	unsigned int cur_distance, min_distance = distance;
	for_each_dive_site (i, ds) {
		if (dive_site_has_gps_location(ds) &&
		    (cur_distance = get_distance(ds->latitude, ds->longitude, latitude, longitude)) < min_distance) {
			min_distance = cur_distance;
			uuid = ds->uuid;
			if (dsp)
				*dsp = ds;
		}
	}
	return uuid;
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

/* we never allow a second dive site with the same uuid */
struct dive_site *alloc_or_get_dive_site(uint32_t uuid)
{
	struct dive_site *ds;
	if (uuid) {
		if ((ds = get_dive_site_by_uuid(uuid)) != NULL) {
			fprintf(stderr, "PROBLEM: refusing to create dive site with the same uuid %08x\n", uuid);
			return ds;
		}
	}
	int nr = dive_site_table.nr;
	int allocated = dive_site_table.allocated;
	struct dive_site **sites = dive_site_table.dive_sites;

	if (nr >= allocated) {
		allocated = (nr + 32) * 3 / 2;
		sites = realloc(sites, allocated * sizeof(struct dive_site *));
		if (!sites)
			exit(1);
		dive_site_table.dive_sites = sites;
		dive_site_table.allocated = allocated;
	}
	ds = calloc(1, sizeof(*ds));
	if (!ds)
		exit(1);
	sites[nr] = ds;
	dive_site_table.nr = nr + 1;
	// we should always be called with a valid uuid except in the special
	// case where we want to copy a dive site into the memory we allocated
	// here - then we need to pass in 0 and create a temporary uuid here
	// (just so things are always consistent)
	if (uuid)
		ds->uuid = uuid;
	else
		ds->uuid = dive_site_getUniqId();
	return ds;
}

int nr_of_dives_at_dive_site(uint32_t uuid, bool select_only)
{
	int j;
	int nr = 0;
	struct dive *d;
	for_each_dive(j, d) {
		if (d->dive_site_uuid == uuid && (!select_only || d->selected)) {
			nr++;
		}
	}
	return nr;
}

bool is_dive_site_used(uint32_t uuid, bool select_only)
{
	int j;
	bool found = false;
	struct dive *d;
	for_each_dive(j, d) {
		if (d->dive_site_uuid == uuid && (!select_only || d->selected)) {
			found = true;
			break;
		}
	}
	return found;
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

uint32_t create_divesite_uuid(const char *name, timestamp_t divetime)
{
	if (name == NULL)
		name ="";
	unsigned char hash[20];
	SHA_CTX ctx;
	SHA1_Init(&ctx);
	SHA1_Update(&ctx, &divetime, sizeof(timestamp_t));
	SHA1_Update(&ctx, name, strlen(name));
	SHA1_Final(hash, &ctx);
	// now return the first 32 of the 160 bit hash
	return *(uint32_t *)hash;
}

/* allocate a new site and add it to the table */
uint32_t create_dive_site(const char *name, timestamp_t divetime)
{
	uint32_t uuid = create_divesite_uuid(name, divetime);
	struct dive_site *ds = alloc_or_get_dive_site(uuid);
	ds->name = copy_string(name);

	return uuid;
}

/* same as before, but with current time if no current_dive is present */
uint32_t create_dive_site_from_current_dive(const char *name)
{
	if (current_dive != NULL) {
		return create_dive_site(name, current_dive->when);
	} else {
		timestamp_t when;
		time_t now = time(0);
		when = utc_mktime(localtime(&now));
		return create_dive_site(name, when);
	}
}

/* same as before, but with GPS data */
uint32_t create_dive_site_with_gps(const char *name, degrees_t latitude, degrees_t longitude, timestamp_t divetime)
{
	uint32_t uuid = create_divesite_uuid(name, divetime);
	struct dive_site *ds = alloc_or_get_dive_site(uuid);
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

void copy_dive_site(struct dive_site *orig, struct dive_site *copy)
{
	free(copy->name);
	free(copy->notes);
	free(copy->description);

	copy->latitude = orig->latitude;
	copy->longitude = orig->longitude;
	copy->name = copy_string(orig->name);
	copy->notes = copy_string(orig->notes);
	copy->description = copy_string(orig->description);
	copy->uuid = orig->uuid;
	if (orig->taxonomy.category == NULL) {
		free_taxonomy(&copy->taxonomy);
	} else {
		if (copy->taxonomy.category == NULL)
			copy->taxonomy.category = alloc_taxonomy();
		for (int i = 0; i < TC_NR_CATEGORIES; i++) {
			if (i < copy->taxonomy.nr)
				free((void *)copy->taxonomy.category[i].value);
			if (i < orig->taxonomy.nr) {
				copy->taxonomy.category[i] = orig->taxonomy.category[i];
				copy->taxonomy.category[i].value = copy_string(orig->taxonomy.category[i].value);
			}
		}
		copy->taxonomy.nr = orig->taxonomy.nr;
	}
}

void clear_dive_site(struct dive_site *ds)
{
	free(ds->name);
	free(ds->notes);
	free(ds->description);
	ds->name = 0;
	ds->notes = 0;
	ds->description = 0;
	ds->latitude.udeg = 0;
	ds->longitude.udeg = 0;
	ds->uuid = 0;
	ds->taxonomy.nr = 0;
	free_taxonomy(&ds->taxonomy);
}

void merge_dive_sites(uint32_t ref, uint32_t* uuids, int count)
{
	int curr_dive, i;
	struct dive *d;
	for(i = 0; i < count; i++){
		if (uuids[i] == ref)
			continue;

		for_each_dive(curr_dive, d) {
			if (d->dive_site_uuid != uuids[i] )
				continue;
			d->dive_site_uuid = ref;
		}
	}

	for(i = 0; i < count; i++) {
		if (uuids[i] == ref)
			continue;
		delete_dive_site(uuids[i]);
	}
	mark_divelist_changed(true);
}

uint32_t find_or_create_dive_site_with_name(const char *name, timestamp_t divetime)
{
	int i;
	struct dive_site *ds;
	for_each_dive_site(i,ds) {
		if (same_string(name, ds->name))
			break;
	}
	if (ds)
		return ds->uuid;
	return create_dive_site(name, divetime);
}

static int compare_sites(const void *_a, const void *_b)
{
	const struct dive_site *a = (const struct dive_site *)*(void **)_a;
	const struct dive_site *b = (const struct dive_site *)*(void **)_b;
	return a->uuid > b->uuid ? 1 : a->uuid == b->uuid ? 0 : -1;
}

void dive_site_table_sort()
{
	qsort(dive_site_table.dive_sites, dive_site_table.nr, sizeof(struct dive_site *), compare_sites);
}
