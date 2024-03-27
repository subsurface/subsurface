// SPDX-License-Identifier: GPL-2.0
/* divesite.c */
#include "divesite.h"
#include "dive.h"
#include "divelist.h"
#include "divelog.h"
#include "errorhelper.h"
#include "membuffer.h"
#include "subsurface-string.h"
#include "table.h"
#include "sha1.h"

#include <math.h>

int get_divesite_idx(const struct dive_site *ds, struct dive_site_table *ds_table)
{
	int i;
	const struct dive_site *d;
	// tempting as it may be, don't die when called with ds=NULL
	if (ds)
		for_each_dive_site(i, d, ds_table) {
			if (d == ds)
				return i;
		}
	return -1;
}

// TODO: keep table sorted by UUID and do a binary search?
struct dive_site *get_dive_site_by_uuid(uint32_t uuid, struct dive_site_table *ds_table)
{
	int i;
	struct dive_site *ds;
	for_each_dive_site (i, ds, ds_table)
		if (ds->uuid == uuid)
			return get_dive_site(i, ds_table);
	return NULL;
}

/* there could be multiple sites of the same name - return the first one */
struct dive_site *get_dive_site_by_name(const char *name, struct dive_site_table *ds_table)
{
	int i;
	struct dive_site *ds;
	for_each_dive_site (i, ds, ds_table) {
		if (same_string(ds->name, name))
			return ds;
	}
	return NULL;
}

/* there could be multiple sites at the same GPS fix - return the first one */
struct dive_site *get_dive_site_by_gps(const location_t *loc, struct dive_site_table *ds_table)
{
	int i;
	struct dive_site *ds;
	for_each_dive_site (i, ds, ds_table) {
		if (same_location(loc, &ds->location))
			return ds;
	}
	return NULL;
}

/* to avoid a bug where we have two dive sites with different name and the same GPS coordinates
 * and first get the gps coordinates (reading a V2 file) and happen to get back "the other" name,
 * this function allows us to verify if a very specific name/GPS combination already exists */
struct dive_site *get_dive_site_by_gps_and_name(const char *name, const location_t *loc, struct dive_site_table *ds_table)
{
	int i;
	struct dive_site *ds;
	for_each_dive_site (i, ds, ds_table) {
		if (same_location(loc, &ds->location) && same_string(ds->name, name))
			return ds;
	}
	return NULL;
}

// Calculate the distance in meters between two coordinates.
unsigned int get_distance(const location_t *loc1, const location_t *loc2)
{
	double lat1_r = udeg_to_radians(loc1->lat.udeg);
	double lat2_r = udeg_to_radians(loc2->lat.udeg);
	double lat_d_r = udeg_to_radians(loc2->lat.udeg - loc1->lat.udeg);
	double lon_d_r = udeg_to_radians(loc2->lon.udeg - loc1->lon.udeg);

	double a = sin(lat_d_r/2) * sin(lat_d_r/2) +
		cos(lat1_r) * cos(lat2_r) * sin(lon_d_r/2) * sin(lon_d_r/2);
	if (a < 0.0) a = 0.0;
	if (a > 1.0) a = 1.0;
	double c = 2 * atan2(sqrt(a), sqrt(1.0 - a));

	// Earth radius in metres
	return lrint(6371000 * c);
}

/* find the closest one, no more than distance meters away - if more than one at same distance, pick the first */
struct dive_site *get_dive_site_by_gps_proximity(const location_t *loc, int distance, struct dive_site_table *ds_table)
{
	int i;
	struct dive_site *ds, *res = NULL;
	unsigned int cur_distance, min_distance = distance;
	for_each_dive_site (i, ds, ds_table) {
		if (dive_site_has_gps_location(ds) &&
		    (cur_distance = get_distance(&ds->location, loc)) < min_distance) {
			min_distance = cur_distance;
			res = ds;
		}
	}
	return res;
}

int register_dive_site(struct dive_site *ds)
{
	return add_dive_site_to_table(ds, divelog.sites);
}

static int compare_sites(const struct dive_site *a, const struct dive_site *b)
{
	return a->uuid > b->uuid ? 1 : a->uuid == b->uuid ? 0 : -1;
}

static int site_less_than(const struct dive_site *a, const struct dive_site *b)
{
	return compare_sites(a, b) < 0;
}

static MAKE_GROW_TABLE(dive_site_table, struct dive_site *, dive_sites)
static MAKE_GET_INSERTION_INDEX(dive_site_table, struct dive_site *, dive_sites, site_less_than)
static MAKE_ADD_TO(dive_site_table, struct dive_site *, dive_sites)
static MAKE_REMOVE_FROM(dive_site_table, dive_sites)
static MAKE_GET_IDX(dive_site_table, struct dive_site *, dive_sites)
MAKE_SORT(dive_site_table, struct dive_site *, dive_sites, compare_sites)
static MAKE_REMOVE(dive_site_table, struct dive_site *, dive_site)
MAKE_CLEAR_TABLE(dive_site_table, dive_sites, dive_site)
MAKE_MOVE_TABLE(dive_site_table, dive_sites)

int add_dive_site_to_table(struct dive_site *ds, struct dive_site_table *ds_table)
{
	/* If the site doesn't yet have an UUID, create a new one.
	 * Make this deterministic for testing. */
	if (!ds->uuid) {
		SHA_CTX ctx;
		uint32_t csum[5];

		SHA1_Init(&ctx);
		if (ds->name)
			SHA1_Update(&ctx, ds->name, strlen(ds->name));
		if (ds->description)
			SHA1_Update(&ctx, ds->description, strlen(ds->description));
		if (ds->notes)
			SHA1_Update(&ctx, ds->notes, strlen(ds->notes));
		SHA1_Final((unsigned char *)csum, &ctx);
		ds->uuid = csum[0];
	}

	/* Take care to never have the same uuid twice. This could happen on
	 * reimport of a log where the dive sites have diverged */
	while (ds->uuid == 0 || get_dive_site_by_uuid(ds->uuid, ds_table) != NULL)
		++ds->uuid;

	int idx = dive_site_table_get_insertion_index(ds_table, ds);
	add_to_dive_site_table(ds_table, idx, ds);
	return idx;
}

struct dive_site *alloc_dive_site()
{
	struct dive_site *ds;
	ds = calloc(1, sizeof(*ds));
	if (!ds)
		exit(1);
	return ds;
}

struct dive_site *alloc_dive_site_with_name(const char *name)
{
	struct dive_site *ds = alloc_dive_site();
	ds->name = copy_string(name);
	return ds;
}

struct dive_site *alloc_dive_site_with_gps(const char *name, const location_t *loc)
{
	struct dive_site *ds = alloc_dive_site_with_name(name);
	ds->location = *loc;

	return ds;
}

/* when parsing, dive sites are identified by uuid */
struct dive_site *alloc_or_get_dive_site(uint32_t uuid, struct dive_site_table *ds_table)
{
	struct dive_site *ds;

	if (uuid && (ds = get_dive_site_by_uuid(uuid, ds_table)) != NULL)
		return ds;

	ds = alloc_dive_site();
	ds->uuid = uuid;

	add_dive_site_to_table(ds, ds_table);

	return ds;
}

int nr_of_dives_at_dive_site(struct dive_site *ds)
{
	return ds->dives.nr;
}

bool is_dive_site_selected(struct dive_site *ds)
{
	int i;

	for (i = 0; i < ds->dives.nr; i++) {
		if (ds->dives.dives[i]->selected)
			return true;
	}
	return false;
}

void free_dive_site(struct dive_site *ds)
{
	if (ds) {
		free(ds->name);
		free(ds->notes);
		free(ds->description);
		free(ds->dives.dives);
		free_taxonomy(&ds->taxonomy);
		free(ds);
	}
}

int unregister_dive_site(struct dive_site *ds)
{
	return remove_dive_site(ds, divelog.sites);
}

void delete_dive_site(struct dive_site *ds, struct dive_site_table *ds_table)
{
	if (!ds)
		return;
	remove_dive_site(ds, ds_table);
	free_dive_site(ds);
}

/* allocate a new site and add it to the table */
struct dive_site *create_dive_site(const char *name, struct dive_site_table *ds_table)
{
	struct dive_site *ds = alloc_dive_site_with_name(name);
	add_dive_site_to_table(ds, ds_table);
	return ds;
}

/* same as before, but with GPS data */
struct dive_site *create_dive_site_with_gps(const char *name, const location_t *loc, struct dive_site_table *ds_table)
{
	struct dive_site *ds = alloc_dive_site_with_gps(name, loc);
	add_dive_site_to_table(ds, ds_table);
	return ds;
}

/* if all fields are empty, the dive site is pointless */
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

/* Used to check on import if two dive sites are equivalent.
 * Since currently no merging is performed, be very conservative
 * and only consider equal dive sites that are exactly the same.
 * Taxonomy is not compared, as no taxonomy is generated on
 * import.
 */
static bool same_dive_site(const struct dive_site *a, const struct dive_site *b)
{
	return same_string(a->name, b->name)
	    && same_location(&a->location, &b->location)
	    && same_string(a->description, b->description)
	    && same_string(a->notes, b->notes);
}

struct dive_site *get_same_dive_site(const struct dive_site *site)
{
	int i;
	struct dive_site *ds;
	for_each_dive_site (i, ds, divelog.sites)
		if (same_dive_site(ds, site))
			return ds;
	return NULL;
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

struct dive_site *find_or_create_dive_site_with_name(const char *name, struct dive_site_table *ds_table)
{
	int i;
	struct dive_site *ds;
	for_each_dive_site(i,ds, ds_table) {
		if (same_string(name, ds->name))
			break;
	}
	if (ds)
		return ds;
	return create_dive_site(name, ds_table);
}

void purge_empty_dive_sites(struct dive_site_table *ds_table)
{
	int i, j;
	struct dive *d;
	struct dive_site *ds;

	for (i = 0; i < ds_table->nr; i++) {
		ds = get_dive_site(i, ds_table);
		if (!dive_site_is_empty(ds))
			continue;
		for_each_dive(j, d) {
			if (d->dive_site == ds)
				unregister_dive_from_dive_site(d);
		}
	}
}

void add_dive_to_dive_site(struct dive *d, struct dive_site *ds)
{
	int idx;
	if (!d) {
		report_info("Warning: add_dive_to_dive_site called with NULL dive");
		return;
	}
	if (!ds) {
		report_info("Warning: add_dive_to_dive_site called with NULL dive site");
		return;
	}
	if (d->dive_site == ds)
		return;
	if (d->dive_site) {
		report_info("Warning: adding dive that already belongs to a dive site to a different site");
		unregister_dive_from_dive_site(d);
	}
	idx = dive_table_get_insertion_index(&ds->dives, d);
	add_to_dive_table(&ds->dives, idx, d);
	d->dive_site = ds;
}

struct dive_site *unregister_dive_from_dive_site(struct dive *d)
{
	struct dive_site *ds = d->dive_site;
	if (!ds)
		return NULL;
	remove_dive(d, &ds->dives);
	d->dive_site = NULL;
	return ds;
}
