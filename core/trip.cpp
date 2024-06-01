// SPDX-License-Identifier: GPL-2.0

#include "trip.h"
#include "dive.h"
#include "divelog.h"
#include "subsurface-time.h"
#include "subsurface-string.h"
#include "selection.h"
#include "table.h"
#include "core/errorhelper.h"

#ifdef DEBUG_TRIP
void dump_trip_list()
{
	timestamp_t last_time = 0;

	for (auto &trip: divelog.trips) {
		struct tm tm;
		utc_mkdate(trip_date(*trip), &tm);
		if (trip_date(*trip) < last_time)
			printf("\n\ntrip_table OUT OF ORDER!!!\n\n\n");
		printf("%s trip %d to \"%s\" on %04u-%02u-%02u %02u:%02u:%02u (%d dives - %p)\n",
		       trip->autogen ? "autogen " : "",
		       i + 1, trip->location.c_str(),
		       tm.tm_year, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
		       trip->dives.nr, trip.get());
		last_time = trip_date(*trip);
	}
	printf("-----\n");
}
#endif

dive_trip::dive_trip() : id(dive_getUniqID())
{
}

dive_trip::~dive_trip()
{
	free(dives.dives);
}

timestamp_t trip_date(const struct dive_trip &trip)
{
	if (trip.dives.nr == 0)
		return 0;
	return trip.dives.dives[0]->when;
}

static timestamp_t trip_enddate(const struct dive_trip &trip)
{
	if (trip.dives.nr == 0)
		return 0;
	return dive_endtime(trip.dives.dives[trip.dives.nr - 1]);
}

/* check if we have a trip right before / after this dive */
bool is_trip_before_after(const struct dive *dive, bool before)
{
	int idx = get_idx_by_uniq_id(dive->id);
	if (before) {
		const struct dive *d = get_dive(idx - 1);
		if (d && d->divetrip)
			return true;
	} else {
		const struct dive *d = get_dive(idx + 1);
		if (d && d->divetrip)
			return true;
	}
	return false;
}

/* Add dive to a trip. Caller is responsible for removing dive
 * from trip beforehand. */
void add_dive_to_trip(struct dive *dive, dive_trip *trip)
{
	if (dive->divetrip == trip)
		return;
	if (dive->divetrip)
		report_info("Warning: adding dive to trip that has trip set\n");
	insert_dive(&trip->dives, dive);
	dive->divetrip = trip;
}

/* remove a dive from the trip it's associated to, but don't delete the
 * trip if this was the last dive in the trip. the caller is responsible
 * for removing the trip, if the trip->dives.nr went to 0.
 */
struct dive_trip *unregister_dive_from_trip(struct dive *dive)
{
	dive_trip *trip = dive->divetrip;

	if (!trip)
		return NULL;

	remove_dive(dive, &trip->dives);
	dive->divetrip = NULL;
	return trip;
}

std::unique_ptr<dive_trip> create_trip_from_dive(const struct dive *dive)
{
	auto trip = std::make_unique<dive_trip>();
	trip->location = get_dive_location(dive);

	return trip;
}

/* random threshold: three days without diving -> new trip
 * this works very well for people who usually dive as part of a trip and don't
 * regularly dive at a local facility; this is why trips are an optional feature */
#define TRIP_THRESHOLD 3600 * 24 * 3

/*
 * Find a trip a new dive should be autogrouped with. If no such trips
 * exist, allocate a new trip. A unique_ptr is returned  if a new trip
 * was allocated. The caller has to store it.
 */
std::pair<dive_trip *, std::unique_ptr<dive_trip>> get_trip_for_new_dive(const struct dive *new_dive)
{
	dive *d;
	int i;

	/* Find dive that is within TRIP_THRESHOLD of current dive */
	for_each_dive(i, d) {
		/* Check if we're past the range of possible dives */
		if (d->when >= new_dive->when + TRIP_THRESHOLD)
			break;

		if (d->when + TRIP_THRESHOLD >= new_dive->when && d->divetrip)
			return { d->divetrip, nullptr }; /* Found a dive with trip in the range */
	}

	/* Didn't find a trip -> allocate a new one */
	auto trip = create_trip_from_dive(new_dive);
	trip->autogen = true;
	auto t = trip.get();
	return { t, std::move(trip) };
}

/* lookup of trip in main trip_table based on its id */
dive_trip *trip_table::get_by_uniq_id(int tripId) const
{
	auto it = std::find_if(begin(), end(), [tripId](auto &t) { return t->id == tripId; });
	return it != end() ? it->get() : nullptr;
}

/* Check if two trips overlap time-wise up to trip threshold. */
bool trips_overlap(const struct dive_trip &t1, const struct dive_trip &t2)
{
	/* First, handle the empty-trip cases. */
	if (t1.dives.nr == 0 || t2.dives.nr == 0)
		return 0;

	if (trip_date(t1) < trip_date(t2))
		return trip_enddate(t1) + TRIP_THRESHOLD >= trip_date(t2);
	else
		return trip_enddate(t2) + TRIP_THRESHOLD >= trip_date(t1);
}

/*
 * Collect dives for auto-grouping. Pass in first dive which should be checked.
 * Returns range of dives that should be autogrouped and trip it should be
 * associated to. If the returned trip was newly allocated, a std::unique_ptr<>
 * to the trip is returned.
 * is set to true. Caller still has to register it in the system. Note
 * whereas this looks complicated - it is needed by the undo-system, which
 * manually injects the new trips. If there are no dives to be autogrouped,
 * return NULL.
 */
std::vector<dives_to_autogroup_result> get_dives_to_autogroup(struct dive_table *table)
{
	std::vector<dives_to_autogroup_result> res;
	struct dive *lastdive = NULL;

	/* Find first dive that should be merged and remember any previous
	 * dive that could be merged into.
	 */
	for (int i = 0; i < table->nr; ++i) {
		struct dive *dive = table->dives[i];
		dive_trip *trip;

		if (dive->divetrip) {
			lastdive = dive;
			continue;
		}

		/* Only consider dives that have not been explicitly removed from
		 * a dive trip by the user.  */
		if (dive->notrip) {
			lastdive = NULL;
			continue;
		}

		/* We found a dive, let's see if we have to allocate a new trip */
		std::unique_ptr<dive_trip> allocated;
		if (!lastdive || dive->when >= lastdive->when + TRIP_THRESHOLD) {
			/* allocate new trip */
			allocated = create_trip_from_dive(dive);
			allocated->autogen = true;
			trip = allocated.get();
		} else {
			/* use trip of previous dive */
			trip = lastdive->divetrip;
		}

		// Now, find all dives that will be added to this trip
		lastdive = dive;
		int to;
		for (to = i + 1; to < table->nr; to++) {
			dive = table->dives[to];
			if (dive->divetrip || dive->notrip ||
			    dive->when >= lastdive->when + TRIP_THRESHOLD)
				break;
			if (trip->location.empty())
				trip->location = get_dive_location(dive);
			lastdive = dive;
		}
		res.push_back({ i, to, trip, std::move(allocated) });
		i = to - 1;
	}

	return res;
}

/* Out of two strings, get the string that is not empty (if any). */
static std::string non_empty_string(const std::string &a, const std::string &b)
{
	return b.empty() ? a : b;
}

/* This combines the information of two trips, generating a
 * new trip. To support undo, we have to preserve the old trips. */
std::unique_ptr<dive_trip> combine_trips(struct dive_trip *trip_a, struct dive_trip *trip_b)
{
	auto trip = std::make_unique<dive_trip>();

	trip->location = non_empty_string(trip_a->location, trip_b->location);
	trip->notes = non_empty_string(trip_a->notes, trip_b->notes);

	return trip;
}

/* Trips are compared according to the first dive in the trip. */
int comp_trips(const struct dive_trip &a, const struct dive_trip &b)
{
	// To make sure that trips never compare equal, compare by
	// address if both are empty.
	if (&a == &b)
		return 0;	// reflexivity. shouldn't happen.
	if (a.dives.nr <= 0 && b.dives.nr <= 0)
		return &a < &b ? -1 : 1;
	if (a.dives.nr <= 0)
		return -1;
	if (b.dives.nr <= 0)
		return 1;
	return comp_dives(a.dives.dives[0], b.dives.dives[0]);
}

static bool is_same_day(timestamp_t trip_when, timestamp_t dive_when)
{
	static timestamp_t twhen = (timestamp_t) 0;
	static struct tm tmt;
	struct tm tmd;

	utc_mkdate(dive_when, &tmd);

	if (twhen != trip_when) {
		twhen = trip_when;
		utc_mkdate(twhen, &tmt);
	}

	return (tmd.tm_mday == tmt.tm_mday) && (tmd.tm_mon == tmt.tm_mon) && (tmd.tm_year == tmt.tm_year);
}

bool trip_is_single_day(const struct dive_trip &trip)
{
	if (trip.dives.nr <= 1)
		return true;
	return is_same_day(trip.dives.dives[0]->when,
			   trip.dives.dives[trip.dives.nr - 1]->when);
}

int trip_shown_dives(const struct dive_trip *trip)
{
	int res = 0;
	for (int i = 0; i < trip->dives.nr; ++i) {
		if (!trip->dives.dives[i]->hidden_by_filter)
			res++;
	}
	return res;
}
