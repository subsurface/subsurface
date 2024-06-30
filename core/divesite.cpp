// SPDX-License-Identifier: GPL-2.0
/* divesite.c */
#include "divesite.h"
#include "dive.h"
#include "divelist.h"
#include "errorhelper.h"
#include "format.h"
#include "membuffer.h"
#include "subsurface-string.h"
#include "sha1.h"

#include <math.h>

int divesite_comp_uuid(const dive_site &ds1, const dive_site &ds2)
{
	if (ds1.uuid == ds2.uuid)
		return 0;
	return ds1.uuid < ds2.uuid ? -1 : 1;
}

template <typename PRED>
dive_site *get_by_predicate(const dive_site_table &ds_table, PRED pred)
{
	auto it = std::find_if(ds_table.begin(), ds_table.end(), pred);
	return it != ds_table.end() ? it->get() : NULL;
}

dive_site *dive_site_table::get_by_uuid(uint32_t uuid) const
{
	// The table is sorted by uuid
	auto it = std::lower_bound(begin(), end(), uuid,
				   [] (const auto &ds, auto uuid) { return ds->uuid < uuid; });
	return it != end() && (*it)->uuid == uuid ? it->get() : NULL;
}

/* there could be multiple sites of the same name - return the first one */
dive_site *dive_site_table::get_by_name(const std::string &name) const
{
	return get_by_predicate(*this, [&name](const auto &ds) { return ds->name == name; });
}

/* there could be multiple sites at the same GPS fix - return the first one */
dive_site *dive_site_table::get_by_gps(const location_t *loc) const
{
	return get_by_predicate(*this, [loc](const auto &ds) { return ds->location == *loc; });
}

/* to avoid a bug where we have two dive sites with different name and the same GPS coordinates
 * and first get the gps coordinates (reading a V2 file) and happen to get back "the other" name,
 * this function allows us to verify if a very specific name/GPS combination already exists */
dive_site *dive_site_table::get_by_gps_and_name(const std::string &name, const location_t loc) const
{
	return get_by_predicate(*this, [&name, loc](const auto &ds) { return ds->location == loc &&
									     ds->name == name; });
}

/* find the closest one, no more than distance meters away - if more than one at same distance, pick the first */
dive_site *dive_site_table::get_by_gps_proximity(location_t loc, int distance) const
{
	struct dive_site *res = nullptr;
	unsigned int cur_distance, min_distance = distance;
	for (const auto &ds: *this) {
		if (ds->has_gps_location() &&
		    (cur_distance = get_distance(ds->location, loc)) < min_distance) {
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
	while (ds->uuid == 0 || get_by_uuid(ds->uuid) != NULL)
		++ds->uuid;

	return put(std::move(ds));
}

dive_site::dive_site()
{
}

dive_site::dive_site(const std::string &name) : name(name)
{
}

dive_site::dive_site(const std::string &name, const location_t loc) : name(name), location(loc)
{
}

dive_site::dive_site(uint32_t uuid) : uuid(uuid)
{
}

dive_site::~dive_site()
{
}

/* when parsing, dive sites are identified by uuid */
dive_site *dive_site_table::alloc_or_get(uint32_t uuid)
{
	struct dive_site *ds;

	if (uuid && (ds = get_by_uuid(uuid)) != NULL)
		return ds;

	return register_site(std::make_unique<dive_site>(uuid)).ptr;
}

size_t dive_site::nr_of_dives() const
{
	return dives.size();
}

bool dive_site::is_selected() const
{
	return any_of(dives.begin(), dives.end(),
		      [](dive *dive) { return dive->selected; });
}

bool dive_site::has_gps_location() const
{
	return has_location(&location);
}

/* allocate a new site and add it to the table */
dive_site *dive_site_table::create(const std::string &name)
{
	return register_site(std::make_unique<dive_site>(name)).ptr;
}

/* same as before, but with GPS data */
dive_site *dive_site_table::create(const std::string &name, const location_t loc)
{
	return register_site(std::make_unique<dive_site>(name, loc)).ptr;
}

/* if all fields are empty, the dive site is pointless */
bool dive_site::is_empty() const
{
	return name.empty() &&
	       description.empty() &&
	       notes.empty() &&
	       !has_location(&location);
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
static bool same(const struct dive_site &a, const struct dive_site &b)
{
	return a.name == b.name
	    && a.location == b.location
	    && a.description == b.description
	    && a.notes == b.notes;
}

dive_site *dive_site_table::get_same(const struct dive_site &site) const
{
	return get_by_predicate(*this, [site](const auto &ds) { return same(*ds, site); });
}

void dive_site::merge(dive_site &b)
{
	if (!has_location(&location)) location = b.location;
	merge_string(name, b.name);
	merge_string(notes, b.notes);
	merge_string(description, b.description);

	if (taxonomy.empty())
		taxonomy = std::move(b.taxonomy);
}

dive_site *dive_site_table::find_or_create(const std::string &name)
{
	struct dive_site *ds = get_by_name(name);
	if (ds)
		return ds;
	return create(name);
}

void dive_site_table::purge_empty()
{
	for (const auto &ds: *this) {
		if (!ds->is_empty())
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

void dive_site::add_dive(struct dive *d)
{
	if (!d) {
		report_info("Warning: dive_site::add_dive() called with NULL dive");
		return;
	}
	if (d->dive_site == this)
		return;
	if (d->dive_site) {
		report_info("Warning: adding dive that already belongs to a dive site to a different site");
		unregister_dive_from_dive_site(d);
	}
	dives.push_back(d);
	d->dive_site = this;
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
