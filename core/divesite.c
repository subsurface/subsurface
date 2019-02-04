// SPDX-License-Identifier: GPL-2.0
/* divesite.c */
#include "divesite.h"
#include "dive.h"
#include "subsurface-string.h"
#include "divelist.h"
#include "membuffer.h"

#include <math.h>

struct dive_site_table dive_site_table;

/* there could be multiple sites of the same name - return the first one */
struct dive_site *get_dive_site_by_name(const char *name)
{
	int i;
	struct dive_site *ds;
	for_each_dive_site (i, ds) {
		if (same_string(ds->name, name))
			return ds;
	}
	return NULL;
}

/* there could be multiple sites at the same GPS fix - return the first one */
struct dive_site *get_dive_site_by_gps(const location_t *loc)
{
	int i;
	struct dive_site *ds;
	for_each_dive_site (i, ds) {
		if (same_location(loc, &ds->location))
			return ds;
	}
	return NULL;
}

/* to avoid a bug where we have two dive sites with different name and the same GPS coordinates
 * and first get the gps coordinates (reading a V2 file) and happen to get back "the other" name,
 * this function allows us to verify if a very specific name/GPS combination already exists */
struct dive_site *get_dive_site_by_gps_and_name(char *name, const location_t *loc)
{
	int i;
	struct dive_site *ds;
	for_each_dive_site (i, ds) {
		if (same_location(loc, &ds->location) && same_string(ds->name, name))
			return ds;
	}
	return NULL;
}

// Calculate the distance in meters between two coordinates.
unsigned int get_distance(const location_t *loc1, const location_t *loc2)
{
	double lat2_r = udeg_to_radians(loc2->lat.udeg);
	double lat_d_r = udeg_to_radians(loc2->lat.udeg - loc1->lat.udeg);
	double lon_d_r = udeg_to_radians(loc2->lon.udeg - loc1->lon.udeg);

	double a = sin(lat_d_r/2) * sin(lat_d_r/2) +
		cos(lat2_r) * cos(lat2_r) * sin(lon_d_r/2) * sin(lon_d_r/2);
	double c = 2 * atan2(sqrt(a), sqrt(1.0 - a));

	// Earth radious in metres
	return lrint(6371000 * c);
}

/* find the closest one, no more than distance meters away - if more than one at same distance, pick the first */
struct dive_site *get_dive_site_by_gps_proximity(const location_t *loc, int distance)
{
	int i;
	struct dive_site *ds, *res = NULL;
	unsigned int cur_distance, min_distance = distance;
	for_each_dive_site (i, ds) {
		if (dive_site_has_gps_location(ds) &&
		    (cur_distance = get_distance(&ds->location, loc)) < min_distance) {
			min_distance = cur_distance;
			res = ds;
		}
	}
	return res;
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

	if (uuid && (ds = get_dive_site_by_uuid(uuid)) != NULL)
		return ds;

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

int nr_of_dives_at_dive_site(struct dive_site *ds, bool select_only)
{
	int j;
	int nr = 0;
	struct dive *d;
	if (!ds)
		return 0;
	for_each_dive(j, d) {
		if (d->dive_site == ds && (!select_only || d->selected)) {
			nr++;
		}
	}
	return nr;
}

bool is_dive_site_used(struct dive_site *ds, bool select_only)
{
	int j;
	bool found = false;
	struct dive *d;
	if (!ds)
		return false;
	for_each_dive(j, d) {
		if (d->dive_site == ds && (!select_only || d->selected)) {
			found = true;
			break;
		}
	}
	return found;
}

void free_dive_site(struct dive_site *ds)
{
	if (ds) {
		free(ds->name);
		free(ds->notes);
		free(ds->description);
		free_taxonomy(&ds->taxonomy);
		free(ds);
	}
}

void delete_dive_site(struct dive_site *ds)
{
	int nr = dive_site_table.nr;
	for (int i = 0; i < nr; i++) {
		if (ds == get_dive_site(i)) {
			free_dive_site(ds);
			if (nr - 1 > i)
				memmove(&dive_site_table.dive_sites[i],
					&dive_site_table.dive_sites[i+1],
					(nr - 1 - i) * sizeof(dive_site_table.dive_sites[0]));
			dive_site_table.nr = nr - 1;
			break;
		}
	}
}

static uint32_t create_divesite_uuid(const char *name, timestamp_t divetime)
{
	if (name == NULL)
		name ="";
	union {
		unsigned char hash[20];
		uint32_t i;
	} u;
	SHA_CTX ctx;
	SHA1_Init(&ctx);
	SHA1_Update(&ctx, &divetime, sizeof(timestamp_t));
	SHA1_Update(&ctx, name, strlen(name));
	SHA1_Final(u.hash, &ctx);
	// now return the first 32 of the 160 bit hash
	return u.i;
}

/* allocate a new site and add it to the table */
struct dive_site *create_dive_site(const char *name, timestamp_t divetime)
{
	uint32_t uuid = create_divesite_uuid(name, divetime);
	struct dive_site *ds = alloc_or_get_dive_site(uuid);
	ds->name = copy_string(name);
	return ds;
}

/* same as before, but with current time if no current_dive is present */
struct dive_site *create_dive_site_from_current_dive(const char *name)
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
struct dive_site *create_dive_site_with_gps(const char *name, const location_t *loc, timestamp_t divetime)
{
	uint32_t uuid = create_divesite_uuid(name, divetime);
	struct dive_site *ds = alloc_or_get_dive_site(uuid);
	ds->name = copy_string(name);
	ds->location = *loc;

	return ds;
}

/* a uuid is always present - but if all the other fields are empty, the dive site is pointless */
bool dive_site_is_empty(struct dive_site *ds)
{
	return !ds ||
	       (empty_string(ds->name) &&
	       empty_string(ds->description) &&
	       empty_string(ds->notes) &&
	       !has_location(&ds->location));
}

void copy_dive_site(struct dive_site *orig, struct dive_site *copy)
{
	free(copy->name);
	free(copy->notes);
	free(copy->description);

	copy->location = orig->location;
	copy->name = copy_string(orig->name);
	copy->notes = copy_string(orig->notes);
	copy->description = copy_string(orig->description);
	copy->uuid = orig->uuid;
	copy_taxonomy(&orig->taxonomy, &copy->taxonomy);
}

static void merge_string(char **a, char **b)
{
	char *s1 = *a, *s2 = *b;

	if (!s2)
		return;

	if (same_string(s1, s2))
		return;

	if (!s1) {
		*a = strdup(s2);
		return;
	}

	*a = format_string("(%s) or (%s)", s1, s2);
	free(s1);
}

void merge_dive_site(struct dive_site *a, struct dive_site *b)
{
	if (!has_location(&a->location)) a->location = b->location;
	merge_string(&a->name, &b->name);
	merge_string(&a->notes, &b->notes);
	merge_string(&a->description, &b->description);

	if (!a->taxonomy.category) {
		a->taxonomy = b->taxonomy;
		memset(&b->taxonomy, 0, sizeof(b->taxonomy));
	}
}

void clear_dive_site(struct dive_site *ds)
{
	free(ds->name);
	free(ds->notes);
	free(ds->description);
	ds->name = NULL;
	ds->notes = NULL;
	ds->description = NULL;
	ds->location.lat.udeg = 0;
	ds->location.lon.udeg = 0;
	ds->uuid = 0;
	ds->taxonomy.nr = 0;
	free_taxonomy(&ds->taxonomy);
}

void merge_dive_sites(struct dive_site *ref, struct dive_site *dive_sites[], int count)
{
	int curr_dive, i;
	struct dive *d;
	for(i = 0; i < count; i++){
		if (dive_sites[i] == ref)
			continue;

		for_each_dive(curr_dive, d) {
			if (d->dive_site != dive_sites[i] )
				continue;
			d->dive_site = ref;
			invalidate_dive_cache(d);
		}
	}

	for(i = 0; i < count; i++) {
		if (dive_sites[i] == ref)
			continue;
		delete_dive_site(dive_sites[i]);
	}
	mark_divelist_changed(true);
}

struct dive_site *find_or_create_dive_site_with_name(const char *name, timestamp_t divetime)
{
	int i;
	struct dive_site *ds;
	for_each_dive_site(i,ds) {
		if (same_string(name, ds->name))
			break;
	}
	if (ds)
		return ds;
	return create_dive_site(name, divetime);
}

void purge_empty_dive_sites()
{
	int i, j;
	struct dive *d;
	struct dive_site *ds;

	for (i = 0; i < dive_site_table.nr; i++) {
		ds = get_dive_site(i);
		if (!dive_site_is_empty(ds))
			continue;
		for_each_dive(j, d) {
			if (d->dive_site == ds)
				d->dive_site = NULL;
		}
	}
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
