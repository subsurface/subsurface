// SPDX-License-Identifier: GPL-2.0
/* divesite.c */
#include "divesite.h"
#include "dive.h"
#include "subsurface-string.h"
#include "divelist.h"
#include "membuffer.h"

#include <math.h>

struct dive_site_table dive_site_table;

// TODO: keep table sorted by UUID and do a binary search?
int get_divesite_idx(const struct dive_site *ds, struct dive_site_table *table)
{
	int i;
	const struct dive_site *d;
	// tempting as it may be, don't die when called with ds=NULL
	if (ds)
		for_each_dive_site(i, d, table) {
			if (d == ds)
				return i;
		}
	return -1;
}

struct dive_site *get_dive_site_by_uuid(uint32_t uuid, struct dive_site_table *table)
{
	int i;
	struct dive_site *ds;
	for_each_dive_site (i, ds, table)
		if (ds->uuid == uuid)
			return get_dive_site(i, table);
	return NULL;
}

/* there could be multiple sites of the same name - return the first one */
struct dive_site *get_dive_site_by_name(const char *name, struct dive_site_table *table)
{
	int i;
	struct dive_site *ds;
	for_each_dive_site (i, ds, table) {
		if (same_string(ds->name, name))
			return ds;
	}
	return NULL;
}

/* there could be multiple sites at the same GPS fix - return the first one */
struct dive_site *get_dive_site_by_gps(const location_t *loc, struct dive_site_table *table)
{
	int i;
	struct dive_site *ds;
	for_each_dive_site (i, ds, table) {
		if (same_location(loc, &ds->location))
			return ds;
	}
	return NULL;
}

/* to avoid a bug where we have two dive sites with different name and the same GPS coordinates
 * and first get the gps coordinates (reading a V2 file) and happen to get back "the other" name,
 * this function allows us to verify if a very specific name/GPS combination already exists */
struct dive_site *get_dive_site_by_gps_and_name(char *name, const location_t *loc, struct dive_site_table *table)
{
	int i;
	struct dive_site *ds;
	for_each_dive_site (i, ds, table) {
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
struct dive_site *get_dive_site_by_gps_proximity(const location_t *loc, int distance, struct dive_site_table *table)
{
	int i;
	struct dive_site *ds, *res = NULL;
	unsigned int cur_distance, min_distance = distance;
	for_each_dive_site (i, ds, table) {
		if (dive_site_has_gps_location(ds) &&
		    (cur_distance = get_distance(&ds->location, loc)) < min_distance) {
			min_distance = cur_distance;
			res = ds;
		}
	}
	return res;
}

void register_dive_site(struct dive_site *ds)
{
	add_dive_site_to_table(ds, &dive_site_table);
}

void add_dive_site_to_table(struct dive_site *ds, struct dive_site_table *table)
{
	int nr = table->nr;
	int allocated = table->allocated;
	struct dive_site **sites = table->dive_sites;

	/* Take care to never have the same uuid twice. This could happen on
	 * reimport of a log where the dive sites have diverged */
	while (ds->uuid == 0 || get_dive_site_by_uuid(ds->uuid, table) != NULL) {
		ds->uuid = rand() & 0xff;
		ds->uuid |= (rand() & 0xff) << 8;
		ds->uuid |= (rand() & 0xff) << 16;
		ds->uuid |= (rand() & 0xff) << 24;
	}

	if (nr >= allocated) {
		allocated = (nr + 32) * 3 / 2;
		sites = realloc(sites, allocated * sizeof(struct dive_site *));
		if (!sites)
			exit(1);
		table->dive_sites = sites;
		table->allocated = allocated;
	}
	sites[nr] = ds;
	table->nr = nr + 1;
}

struct dive_site *alloc_dive_site()
{
	struct dive_site *ds;
	ds = calloc(1, sizeof(*ds));
	if (!ds)
		exit(1);
	return ds;
}

/* when parsing, dive sites are identified by uuid */
struct dive_site *alloc_or_get_dive_site(uint32_t uuid, struct dive_site_table *table)
{
	struct dive_site *ds;

	if (uuid && (ds = get_dive_site_by_uuid(uuid, table)) != NULL)
		return ds;

	ds = alloc_dive_site();
	ds->uuid = uuid;

	add_dive_site_to_table(ds, table);

	return ds;
}

int nr_of_dives_at_dive_site(struct dive_site *ds)
{
	return ds->dives.nr;
}

bool is_dive_site_used(struct dive_site *ds, bool select_only)
{
	int i;

	if (!select_only)
		return ds->dives.nr > 0;

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
		free_taxonomy(&ds->taxonomy);
		free(ds);
	}
}

void unregister_dive_site(struct dive_site *ds)
{
	remove_dive_site_from_table(ds, &dive_site_table);
}

void remove_dive_site_from_table(struct dive_site *ds, struct dive_site_table *table)
{
	int nr = table->nr;
	for (int i = 0; i < nr; i++) {
		if (ds == get_dive_site(i, table)) {
			if (nr - 1 > i)
				memmove(&table->dive_sites[i],
					&table->dive_sites[i+1],
					(nr - 1 - i) * sizeof(table->dive_sites[0]));
			table->nr = nr - 1;
			break;
		}
	}
}

void delete_dive_site(struct dive_site *ds, struct dive_site_table *table)
{
	if (!ds)
		return;
	remove_dive_site_from_table(ds, table);
	free_dive_site(ds);
}

/* allocate a new site and add it to the table */
struct dive_site *create_dive_site(const char *name, struct dive_site_table *table)
{
	struct dive_site *ds = alloc_dive_site();
	ds->name = copy_string(name);
	add_dive_site_to_table(ds, table);
	return ds;
}

/* same as before, but with GPS data */
struct dive_site *create_dive_site_with_gps(const char *name, const location_t *loc, struct dive_site_table *table)
{
	struct dive_site *ds = alloc_dive_site();
	ds->name = copy_string(name);
	ds->location = *loc;
	add_dive_site_to_table(ds, table);

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
 * Taxonomy is not compared, as not taxonomy is generated on
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
	for_each_dive_site (i, ds, &dive_site_table)
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
			unregister_dive_from_dive_site(d);
			add_dive_to_dive_site(d, ref);
			invalidate_dive_cache(d);
		}
	}

	for(i = 0; i < count; i++) {
		if (dive_sites[i] == ref)
			continue;
		delete_dive_site(dive_sites[i], &dive_site_table);
	}
	mark_divelist_changed(true);
}

struct dive_site *find_or_create_dive_site_with_name(const char *name, struct dive_site_table *table)
{
	int i;
	struct dive_site *ds;
	for_each_dive_site(i,ds, table) {
		if (same_string(name, ds->name))
			break;
	}
	if (ds)
		return ds;
	return create_dive_site(name, table);
}

void purge_empty_dive_sites(struct dive_site_table *table)
{
	int i, j;
	struct dive *d;
	struct dive_site *ds;

	for (i = 0; i < table->nr; i++) {
		ds = get_dive_site(i, table);
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
	if (d->dive_site == ds)
		return;
	if (d->dive_site) {
		fprintf(stderr, "Warning: adding dive that has dive site set to dive site\n");
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

/* Assign arbitrary UUIDs to dive sites. This is called by before writing the dive log to XML or git. */
static int compare_sites(const void *_a, const void *_b)
{
	const struct dive_site *a = (const struct dive_site *)*(void **)_a;
	const struct dive_site *b = (const struct dive_site *)*(void **)_b;
	return a->uuid > b->uuid ? 1 : a->uuid == b->uuid ? 0 : -1;
}

void dive_site_table_sort(struct dive_site_table *table)
{
	qsort(table->dive_sites, table->nr, sizeof(struct dive_site *), compare_sites);
}
