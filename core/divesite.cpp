// SPDX-License-Identifier: GPL-2.0
/* divesite.c */
#include "divesite.h"
#include "dive.h"
#include "divelist.h"
#include "divelog.h"
#include "errorhelper.h"
#include "format.h"
#include "gettextfromc.h"
#include "membuffer.h"
#include "pref.h"
#include "subsurface-string.h"
#include "sha1.h"

#include <math.h>

int get_divesite_idx(const struct dive_site *ds, dive_site_table &ds_table)
{
	auto it = std::find_if(ds_table.begin(), ds_table.end(), [ds] (const auto &ds2) { return ds2.get() == ds; });
	return it != ds_table.end() ? it - ds_table.begin() : -1;
}

template <typename PRED>
struct dive_site *get_dive_site_by_predicate(dive_site_table &ds_table, PRED pred)
{
	auto it = std::find_if(ds_table.begin(), ds_table.end(), pred);
	return it != ds_table.end() ? it->get() : NULL;
}

struct dive_site *get_dive_site_by_uuid(uint32_t uuid, dive_site_table &ds_table)
{
	// The table is sorted by uuid
	auto it = std::lower_bound(ds_table.begin(), ds_table.end(), uuid,
				   [] (const auto &ds, auto uuid) { return ds->uuid < uuid; });
	return it != ds_table.end() && (*it)->uuid == uuid ? it->get() : NULL;
}

/* there could be multiple sites of the same name - return the first one */
struct dive_site *get_dive_site_by_name(const std::string &name, dive_site_table &ds_table)
{
	return get_dive_site_by_predicate(ds_table,
			       [&name](const auto &ds) { return ds->name == name; });
}

/* there could be multiple sites at the same GPS fix - return the first one */
struct dive_site *get_dive_site_by_gps(const location_t *loc, dive_site_table &ds_table)
{
	return get_dive_site_by_predicate(ds_table,
			       [loc](const auto &ds) { return ds->location == *loc; });
}

/* to avoid a bug where we have two dive sites with different name and the same GPS coordinates
 * and first get the gps coordinates (reading a V2 file) and happen to get back "the other" name,
 * this function allows us to verify if a very specific name/GPS combination already exists */
struct dive_site *get_dive_site_by_gps_and_name(const std::string &name, const location_t *loc, dive_site_table &ds_table)
{
	return get_dive_site_by_predicate(ds_table,
			       [&name, loc](const auto &ds) { return ds->location == *loc &&
			       					     ds->name == name; });
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
struct dive_site *get_dive_site_by_gps_proximity(const location_t *loc, int distance, dive_site_table &ds_table)
{
	struct dive_site *res = nullptr;
	unsigned int cur_distance, min_distance = distance;
	for (const auto &ds: ds_table) {
		if (dive_site_has_gps_location(ds.get()) &&
		    (cur_distance = get_distance(&ds->location, loc)) < min_distance) {
			min_distance = cur_distance;
			res = ds.get();
		}
	}
	return res;
}

dive_site_table::put_result dive_site_table::register_site(std::unique_ptr<dive_site> ds)
{
	/* If the site doesn't yet have an UUID, create a new one.
	 * Make this deterministic for testing. */
	if (!ds->uuid) {
		SHA1 sha;
		if (!ds->name.empty())
			sha.update(ds->name);
		if (!ds->description.empty())
			sha.update(ds->description);
		if (!ds->notes.empty())
			sha.update(ds->notes);
		ds->uuid = sha.hash_uint32();
	}

	/* Take care to never have the same uuid twice. This could happen on
	 * reimport of a log where the dive sites have diverged */
	while (ds->uuid == 0 || get_dive_site_by_uuid(ds->uuid, *this) != NULL)
		++ds->uuid;

	return put(std::move(ds));
}

dive_site::dive_site()
{
}

dive_site::dive_site(const std::string &name) : name(name)
{
}

dive_site::dive_site(const std::string &name, const location_t *loc) : name(name), location(*loc)
{
}

dive_site::dive_site(uint32_t uuid) : uuid(uuid)
{
}

dive_site::~dive_site()
{
}

/* when parsing, dive sites are identified by uuid */
struct dive_site *alloc_or_get_dive_site(uint32_t uuid, dive_site_table &ds_table)
{
	struct dive_site *ds;

	if (uuid && (ds = get_dive_site_by_uuid(uuid, ds_table)) != NULL)
		return ds;

	return ds_table.register_site(std::make_unique<dive_site>(uuid)).ptr;
}

size_t nr_of_dives_at_dive_site(const dive_site &ds)
{
	return ds.dives.size();
}

bool is_dive_site_selected(const struct dive_site &ds)
{
	return any_of(ds.dives.begin(), ds.dives.end(),
		      [](dive *dive) { return dive->selected; });
}

/* allocate a new site and add it to the table */
struct dive_site *create_dive_site(const std::string &name, dive_site_table &ds_table)
{
	return ds_table.register_site(std::make_unique<dive_site>(name)).ptr;
}

/* same as before, but with GPS data */
struct dive_site *create_dive_site_with_gps(const std::string &name, const location_t *loc, dive_site_table &ds_table)
{
	return ds_table.register_site(std::make_unique<dive_site>(name, loc)).ptr;
}

/* if all fields are empty, the dive site is pointless */
bool dive_site_is_empty(struct dive_site *ds)
{
	return !ds ||
	       (ds->name.empty() &&
	        ds->description.empty() &&
	        ds->notes.empty() &&
	        !has_location(&ds->location));
}

static void merge_string(std::string &a, const std::string &b)
{
	if (b.empty())
		return;

	if (a == b)
		return;

	if (a.empty()) {
		a = b;
		return;
	}

	a = format_string_std("(%s) or (%s)", a.c_str(), b.c_str());
}

/* Used to check on import if two dive sites are equivalent.
 * Since currently no merging is performed, be very conservative
 * and only consider equal dive sites that are exactly the same.
 * Taxonomy is not compared, as no taxonomy is generated on
 * import.
 */
static bool same_dive_site(const struct dive_site &a, const struct dive_site &b)
{
	return a.name == b.name
	    && a.location == b.location
	    && a.description == b.description
	    && a.notes == b.notes;
}

struct dive_site *get_same_dive_site(const struct dive_site &site)
{
	return get_dive_site_by_predicate(*divelog.sites,
			       [site](const auto &ds) { return same_dive_site(*ds, site); });
}

void merge_dive_site(struct dive_site *a, struct dive_site *b)
{
	if (!has_location(&a->location)) a->location = b->location;
	merge_string(a->name, b->name);
	merge_string(a->notes, b->notes);
	merge_string(a->description, b->description);

	if (a->taxonomy.empty())
		a->taxonomy = std::move(b->taxonomy);
}

struct dive_site *find_or_create_dive_site_with_name(const std::string &name, dive_site_table &ds_table)
{
	struct dive_site *ds = get_dive_site_by_name(name, ds_table);
	if (ds)
		return ds;
	return create_dive_site(name, ds_table);
}

void purge_empty_dive_sites(dive_site_table &ds_table)
{
	for (const auto &ds: ds_table) {
		if (!dive_site_is_empty(ds.get()))
			continue;
		while (!ds->dives.empty()) {
			struct dive *d = ds->dives.back();
			if (d->dive_site != ds.get()) {
				report_info("Warning: dive %d registered to wrong dive site in %s", d->number, __func__);
				ds->dives.pop_back();
			} else {
				unregister_dive_from_dive_site(d);
			}
		}
	}
}

void add_dive_to_dive_site(struct dive *d, struct dive_site *ds)
{
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
	ds->dives.push_back(d);
	d->dive_site = ds;
}

struct dive_site *unregister_dive_from_dive_site(struct dive *d)
{
	struct dive_site *ds = d->dive_site;
	if (!ds)
		return nullptr;
	auto it = std::find(ds->dives.begin(), ds->dives.end(), d);
	if (it != ds->dives.end())
		ds->dives.erase(it);
	else
		report_info("Warning: dive not found in divesite table, even though it should be registered there.");
	d->dive_site = nullptr;
	return ds;
}

std::string constructLocationTags(const taxonomy_data &taxonomy, bool for_maintab)
{
	using namespace std::string_literals;
	std::string locationTag;

	if (taxonomy.empty())
		return locationTag;

	/* Check if the user set any of the 3 geocoding categories */
	bool prefs_set = false;
	for (int i = 0; i < 3; i++) {
		if (prefs.geocoding.category[i] != TC_NONE)
			prefs_set = true;
	}

	if (!prefs_set && !for_maintab) {
		locationTag = "<small><small>" + gettextFromC::tr("No dive site layout categories set in preferences!").toStdString() +
			      "</small></small>"s;
		return locationTag;
	}
	else if (!prefs_set)
		return locationTag;

	if (for_maintab)
		locationTag = "<small><small>("s + gettextFromC::tr("Tags").toStdString() + ": "s;
	else
		locationTag = "<small><small>"s;
	std::string connector;
	for (int i = 0; i < 3; i++) {
		if (prefs.geocoding.category[i] == TC_NONE)
			continue;
		for (auto const &t: taxonomy) {
			if (t.category == prefs.geocoding.category[i]) {
				if (!t.value.empty()) {
					locationTag += connector + t.value;
					connector = " / "s;
				}
				break;
			}
		}
	}

	if (for_maintab)
		locationTag += ")</small></small>"s;
	else
		locationTag += "</small></small>"s;
	return locationTag;
}
