// SPDX-License-Identifier: GPL-2.0
#include "divelog.h"
#include "divelist.h"
#include "divesite.h"
#include "device.h"
#include "dive.h"
#include "errorhelper.h"
#include "filterpreset.h"
#include "filterpresettable.h"
#include "qthelper.h" // for emit_reset_signal() -> should be removed
#include "range.h"
#include "selection.h" // clearly, a layering violation -> should be removed
#include "trip.h"

struct divelog divelog;

divelog::divelog() = default;
divelog::~divelog() = default;
divelog::divelog(divelog &&) = default;
struct divelog &divelog::operator=(divelog &&) = default;

/* this implements the mechanics of removing the dive from the
 * dive log and the trip, but doesn't deal with updating dive trips, etc */
void divelog::delete_single_dive(int idx)
{
	if (idx < 0 || static_cast<size_t>(idx) >= dives.size()) {
		report_info("Warning: deleting non-existing dive with index %d", idx);
		return;
	}
	struct dive *dive = dives[idx].get();
	struct dive_trip *trip = unregister_dive_from_trip(dive);

	// Deleting a dive may change the order of trips!
	if (trip)
		trips.sort();

	if (trip && trip->dives.empty())
		trips.pull(trip);
	unregister_dive_from_dive_site(dive);
	dives.erase(dives.begin() + idx);
}

void divelog::delete_multiple_dives(const std::vector<dive *> &dives_to_delete)
{
	bool trips_changed = false;

	for (dive *d: dives_to_delete) {
		// Delete dive from trip and delete trip if empty
		struct dive_trip *trip = unregister_dive_from_trip(d);
		if (trip && trip->dives.empty()) {
			trips_changed = true;
			trips.pull(trip);
		}

		unregister_dive_from_dive_site(d);
		dives.pull(d);
	}

	// Deleting a dive may change the order of trips!
	if (trips_changed)
		trips.sort();
}

void divelog::clear()
{
	dives.clear();
	sites.clear();
	trips.clear();
	devices.clear();
	filter_presets.clear();
}

/* check if we have a trip right before / after this dive */
bool divelog::is_trip_before_after(const struct dive *dive, bool before) const
{
	auto it = std::find_if(dives.begin(), dives.end(),
			       [dive](auto &d) { return d.get() == dive; });
	if (it == dives.end())
		return false;

	if (before) {
		do {
			if (it == dives.begin())
				return false;
			--it;
		} while ((*it)->invalid);
		return (*it)->divetrip != nullptr;
	} else {
		++it;
		while (it != dives.end() && (*it)->invalid)
			++it;
		return it != dives.end() && (*it)->divetrip != nullptr;
	}
}

/*
 * Merge subsequent dives in a table, if mergeable. This assumes
 * that the dives are neither selected, not part of a trip, as
 * is the case of freshly imported dives.
 */
static void merge_imported_dives(struct dive_table &table)
{
	for (size_t i = 1; i < table.size(); i++) {
		auto &prev = table[i - 1];
		auto &dive = table[i];

		/* only try to merge overlapping dives - or if one of the dives has
		 * zero duration (that might be a gps marker from the webservice) */
		if (prev->duration.seconds && dive->duration.seconds &&
		    prev->endtime() < dive->when)
			continue;

		auto merged = table.try_to_merge(*prev, *dive, false);
		if (!merged)
			continue;

		/* Add dive to dive site; try_to_merge() does not do that! */
		struct dive_site *ds = merged->dive_site;
		if (ds) {
			merged->dive_site = NULL;
			ds->add_dive(merged.get());
		}
		unregister_dive_from_dive_site(prev.get());
		unregister_dive_from_dive_site(dive.get());
		unregister_dive_from_trip(prev.get());
		unregister_dive_from_trip(dive.get());

		/* Overwrite the first of the two dives and remove the second */
		table[i - 1] = std::move(merged);
		table.erase(table.begin() + i);

		/* Redo the new 'i'th dive */
		i--;
	}
}

/*
 * Walk the dives from the oldest dive in the given table, and see if we
 * can autogroup them. But only do this when the user selected autogrouping.
 */
static void autogroup_dives(struct divelog &log)
{
	if (!log.autogroup)
		return;

	for (auto &entry: get_dives_to_autogroup(log.dives)) {
		for (auto it = log.dives.begin() + entry.from; it != log.dives.begin() + entry.to; ++it)
			entry.trip->add_dive(it->get());
		/* If this was newly allocated, add trip to list */
		if (entry.created_trip)
			log.trips.put(std::move(entry.created_trip));
	}
	log.trips.sort(); // Necessary?
#ifdef DEBUG_TRIP
	dump_trip_list();
#endif
}

/* Check if a dive is ranked after the last dive of the global dive list */
static bool dive_is_after_last(const struct dive &d)
{
	if (divelog.dives.empty())
		return true;
	return dive_less_than(*divelog.dives.back(), d);
}

/*
 * Try to merge a new dive into the dive at position idx. Return
 * true on success. On success, the old dive will be added to the
 * dives_to_remove table and the merged dive to the dives_to_add
 * table. On failure everything stays unchanged.
 * If "prefer_imported" is true, use data of the new dive.
 */
static bool try_to_merge_into(struct dive &dive_to_add, struct dive *old_dive, bool prefer_imported,
			      /* output parameters: */
			      struct dive_table &dives_to_add, struct std::vector<dive *> &dives_to_remove)
{
	auto merged = dives_to_add.try_to_merge(*old_dive, dive_to_add, prefer_imported);
	if (!merged)
		return false;

	merged->divetrip = old_dive->divetrip;
	range_insert_sorted(dives_to_remove, old_dive, comp_dives_ptr);
	dives_to_add.put(std::move(merged));

	return true;
}

/* Merge dives from "dives_from", owned by "delete" into the owned by "dives_to".
 * Overlapping dives will be merged, non-overlapping dives will be moved. The results
 * will be added to the "dives_to_add" table. Dives that were merged are added to
 * the "dives_to_remove" table. Any newly added (not merged) dive will be assigned
 * to the trip of the "trip" paremeter. If "delete_from" is non-null dives will be
 * removed from this table.
 * This function supposes that all input tables are sorted.
 * Returns true if any dive was added (not merged) that is not past the
 * last dive of the global dive list (i.e. the sequence will change).
 * The integer referenced by "num_merged" will be increased for every
 * merged dive that is added to "dives_to_add" */
static bool merge_dive_tables(const std::vector<dive *> &dives_from, struct dive_table &delete_from,
			      const std::vector<dive *> &dives_to,
			      bool prefer_imported, struct dive_trip *trip,
			      /* output parameters: */
			      struct dive_table &dives_to_add, struct std::vector<dive *> &dives_to_remove,
			      int &num_merged)
{
	bool sequence_changed = false;

	/* Merge newly imported dives into the dive table.
	 * Since both lists (old and new) are sorted, we can step
	 * through them concurrently and locate the insertions points.
	 * Once found, check if the new dive can be merged in the
	 * previous or next dive.
	 * Note that this doesn't consider pathological cases such as:
	 *  - New dive "connects" two old dives (turn three into one).
	 *  - New dive can not be merged into adjacent but some further dive.
	 */
	size_t j = 0; /* Index in dives_to */
	size_t last_merged_into = std::string::npos;
	for (dive *add: dives_from) {
		/* This gets an owning pointer to the dive to add and removes it from
		 * the delete_from table. If the dive is not explicitly stored, it will
		 * be automatically deleting when ending the loop iteration */
		auto [dive_to_add, idx] = delete_from.pull(add);
		if (!dive_to_add) {
			report_info("merging unknown dives!");
			continue;
		}

		/* Find insertion point. */
		while (j < dives_to.size() && dive_less_than(*dives_to[j], *dive_to_add))
			j++;

		/* Try to merge into previous dive.
		 * We are extra-careful to not merge into the same dive twice, as that
		 * would put the merged-into dive twice onto the dives-to-delete list.
		 * In principle that shouldn't happen as all dives that compare equal
		 * by is_same_dive() were already merged, and is_same_dive() should be
		 * transitive. But let's just go *completely* sure for the odd corner-case. */
		if (j > 0 && (last_merged_into == std::string::npos || j > last_merged_into + 1) &&
		    dives_to[j - 1]->endtime() > dive_to_add->when) {
			if (try_to_merge_into(*dive_to_add, dives_to[j - 1], prefer_imported,
					      dives_to_add, dives_to_remove)) {
				last_merged_into = j - 1;
				num_merged++;
				continue;
			}
		}

		/* That didn't merge into the previous dive.
		 * Try to merge into next dive. */
		if (j < dives_to.size() && (last_merged_into == std::string::npos || j > last_merged_into) &&
		    dive_to_add->endtime() > dives_to[j]->when) {
			if (try_to_merge_into(*dive_to_add, dives_to[j], prefer_imported,
					      dives_to_add, dives_to_remove)) {
				last_merged_into = j;
				num_merged++;
				continue;
			}
		}

		sequence_changed |= !dive_is_after_last(*dive_to_add);
		dives_to_add.put(std::move(dive_to_add));
	}

	return sequence_changed;
}

/* Helper function for process_imported_dives():
 * Try to merge a trip into one of the existing trips.
 * The bool pointed to by "sequence_changed" is set to true, if the sequence of
 * the existing dives changes.
 * The int pointed to by "start_renumbering_at" keeps track of the first dive
 * to be renumbered in the dives_to_add table.
 * For other parameters see process_imported_dives()
 * Returns true if trip was merged. In this case, the trip will be
 * freed.
 */
static bool try_to_merge_trip(dive_trip &trip_import, struct dive_table &import_table, bool prefer_imported,
			      /* output parameters: */
			      struct dive_table &dives_to_add, std::vector<dive *> &dives_to_remove,
			      bool &sequence_changed, int &start_renumbering_at)
{
	for (auto &trip_old: divelog.trips) {
		if (trips_overlap(trip_import, *trip_old)) {
			sequence_changed |= merge_dive_tables(trip_import.dives, import_table, trip_old->dives,
							       prefer_imported, trip_old.get(),
							       dives_to_add, dives_to_remove,
							       start_renumbering_at);
			/* we took care of all dives of the trip, clean up the table */
			trip_import.dives.clear();
			return true;
		}
	}

	return false;
}

// Helper function to convert a table of owned dives into a table of non-owning pointers.
// Used to merge *all* dives of a log into a different table.
static std::vector<dive *> dive_table_to_non_owning(const dive_table &dives)
{
	std::vector<dive *> res;
	res.reserve(dives.size());
	for (auto &d: dives)
		res.push_back(d.get());
	return res;
}

/* Process imported dives: take a log.trips of dives to be imported and
 * generate five lists:
 *	1) Dives to be added		(newly created, owned)
 *	2) Dives to be removed		(old, non-owned, references global divelog)
 *	3) Trips to be added		(newly created, owned)
 *	4) Dive sites to be added	(newly created, owned)
 *	5) Devices to be added		(newly created, owned)
 * The dives, trips and sites in import_log are consumed.
 * On return, the tables have * size 0.
 *
 * Note: The new dives will have their divetrip- and divesites-fields
 * set, but will *not* be part of the trip and site. The caller has to
 * add them to the trip and site.
 *
 * The lists are generated by merging dives if possible. This is
 * performed trip-wise. Finer control on merging is provided by
 * the "flags" parameter:
 * - If import_flags::prefer_imported is set, data of the new dives are
 *   prioritized on merging.
 * - If import_flags::merge_all_trips is set, all overlapping trips will
 *   be merged, not only non-autogenerated trips.
 * - If import_flags::is_downloaded is true, only the divecomputer of the
 *   first dive will be considered, as it is assumed that all dives
 *   come from the same computer.
 * - If import_flags::add_to_new_trip is true, dives that are not assigned
 *   to a trip will be added to a newly generated trip.
 */
divelog::process_imported_dives_result divelog::process_imported_dives(struct divelog &import_log, int flags)
{
	int start_renumbering_at = 0;
	bool sequence_changed = false;
	bool last_old_dive_is_numbered;

	process_imported_dives_result res;

	/* If no dives were imported, don't bother doing anything */
	if (import_log.dives.empty())
		return res;

	/* Check if any of the new dives has a number. This will be
	 * important later to decide if we want to renumber the added
	 * dives */
	bool new_dive_has_number = std::any_of(import_log.dives.begin(), import_log.dives.end(),
					       [](auto &d) { return d->number > 0; });

	/* Add only the devices that we don't know about yet. */
	for (auto &dev: import_log.devices) {
		if (!device_exists(devices, dev))
			add_to_device_table(res.devices_to_add, dev);
	}

	/* Sort the table of dives to be imported and combine mergable dives */
	import_log.dives.sort();
	merge_imported_dives(import_log.dives);

	/* Autogroup tripless dives if desired by user. But don't autogroup
	 * if tripless dives should be added to a new trip. */
	if (!(flags & import_flags::add_to_new_trip))
		autogroup_dives(import_log);

	/* If dive sites already exist, use the existing versions. */
	for (auto &new_ds: import_log.sites) {
		/* Check if it dive site is actually used by new dives. */
		if (std::none_of(import_log.dives.begin(), import_log.dives.end(), [ds=new_ds.get()]
				 (auto &d) { return d->dive_site == ds; }))
			continue;

		struct dive_site *old_ds = sites.get_same(*new_ds);
		if (!old_ds) {
			/* Dive site doesn't exist. Add it to list of dive sites to be added. */
			new_ds->dives.clear(); /* Caller is responsible for adding dives to site */
			res.sites_to_add.put(std::move(new_ds));
		} else {
			/* Dive site already exists - use the old one. */
			for (auto &d: import_log.dives) {
				if (d->dive_site == new_ds.get())
					d->dive_site = old_ds;
			}
		}
	}
	import_log.sites.clear();

	/* Merge overlapping trips. Since both trip tables are sorted, we
	 * could be smarter here, but realistically not a whole lot of trips
	 * will be imported so do a simple n*m loop until someone complains.
	 */
	for (auto &trip_import: import_log.trips) {
		if ((flags & import_flags::merge_all_trips) || trip_import->autogen) {
			if (try_to_merge_trip(*trip_import, import_log.dives, flags & import_flags::prefer_imported,
					      res.dives_to_add, res.dives_to_remove,
					      sequence_changed, start_renumbering_at))
				continue;
		}

		/* If no trip to merge-into was found, add trip as-is.
		 * First, add dives to list of dives to add */
		for (struct dive *d: trip_import->dives) {
			/* Add dive to list of dives to-be-added. */
			auto [owned, idx] = import_log.dives.pull(d);
			if (!owned)
				continue;
			sequence_changed |= !dive_is_after_last(*owned);
			res.dives_to_add.put(std::move(owned));
		}

		trip_import->dives.clear(); /* Caller is responsible for adding dives to trip */

		/* Finally, add trip to list of trips to add */
		res.trips_to_add.put(std::move(trip_import));
	}
	import_log.trips.clear(); /* All trips were consumed */

	if ((flags & import_flags::add_to_new_trip) && !import_log.dives.empty()) {
		/* Create a new trip for unassigned dives, if desired. */
		auto [new_trip, idx] = res.trips_to_add.put(
			create_trip_from_dive(import_log.dives.front().get())
		);

		/* Add all remaining dives to this trip */
		for (auto &d: import_log.dives) {
			sequence_changed |= !dive_is_after_last(*d);
			d->divetrip = new_trip;
			res.dives_to_add.put(std::move(d));
		}

		import_log.dives.clear(); /* All dives were consumed */
	} else if (!import_log.dives.empty()) {
		/* The remaining dives in import_log.dives are those that don't belong to
		 * a trip and the caller does not want them to be associated to a
		 * new trip. Merge them into the global table. */
		sequence_changed |= merge_dive_tables(dive_table_to_non_owning(import_log.dives),
						      import_log.dives,
						      dive_table_to_non_owning(dives),
						      flags & import_flags::prefer_imported, NULL,
						      res.dives_to_add, res.dives_to_remove, start_renumbering_at);
	}

	/* If new dives were only added at the end, renumber the added dives.
	 * But only if
	 *	- The last dive in the old dive table had a number itself (if there is a last dive).
	 *	- None of the new dives has a number.
	 */
	last_old_dive_is_numbered = dives.empty() || dives.back()->number > 0;

	/* We counted the number of merged dives that were added to dives_to_add.
	 * Skip those. Since sequence_changed is false all added dives are *after*
	 * all merged dives. */
	if (!sequence_changed && last_old_dive_is_numbered && !new_dive_has_number) {
		int nr = !dives.empty() ? dives.back()->number : 0;
		for (auto it = res.dives_to_add.begin() + start_renumbering_at; it < res.dives_to_add.end(); ++it)
			(*it)->number = ++nr;
	}

	return res;
}

// TODO: This accesses global state, namely fulltext and selection.
// TODO: Move up the call chain?
void divelog::process_loaded_dives()
{
	dives.sort();
	trips.sort();

	/* Autogroup dives if desired by user. */
	autogroup_dives(*this);

	fulltext_populate();

	/* Inform frontend of reset data. This should reset all the models. */
	emit_reset_signal();

	/* Now that everything is settled, select the newest dive. */
	select_newest_visible_dive();
}

/* Merge the dives of the trip "from" and the dive_table "dives_from" into the trip "to"
 * and dive_table "dives_to". If "prefer_imported" is true, dive data of "from" takes
 * precedence */
void divelog::add_imported_dives(struct divelog &import_log, int flags)
{
	/* Process imported dives and generate lists of dives
	 * to-be-added and to-be-removed */
	auto [dives_to_add, dives_to_remove, trips_to_add, dive_sites_to_add, devices_to_add] =
		process_imported_dives(import_log, flags);

	/* Start by deselecting all dives, so that we don't end up with an invalid selection */
	select_single_dive(NULL);

	/* Add new dives to trip and site to get reference count correct. */
	for (auto &d: dives_to_add) {
		struct dive_trip *trip = d->divetrip;
		struct dive_site *site = d->dive_site;
		d->divetrip = NULL;
		d->dive_site = NULL;
		trip->add_dive(d.get());
		if (site)
			site->add_dive(d.get());
	}

	/* Remove old dives */
	delete_multiple_dives(dives_to_remove);

	/* Add new dives */
	for (auto &d: dives_to_add)
		dives.put(std::move(d));
	dives_to_add.clear();

	/* Add new trips */
	for (auto &trip: trips_to_add)
		trips.put(std::move(trip));
	trips_to_add.clear();

	/* Add new dive sites */
	for (auto &ds: dive_sites_to_add)
		sites.register_site(std::move(ds));

	/* Add new devices */
	for (auto &dev: devices_to_add)
		add_to_device_table(devices, dev);

	/* We might have deleted the old selected dive.
	 * Choose the newest dive as selected (if any) */
	current_dive = !dives.empty() ? dives.back().get() : nullptr;

	/* Inform frontend of reset data. This should reset all the models. */
	emit_reset_signal();
}
