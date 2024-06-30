// SPDX-License-Identifier: GPL-2.0

#include "trip.h"
#include "dive.h"
#include "divelog.h"
#include "errorhelper.h"
#include "range.h"
#include "subsurface-time.h"
#include "subsurface-string.h"
#include "selection.h"

dive_trip::dive_trip() : id(dive_getUniqID())
{
}

dive_trip::~dive_trip() = default;

timestamp_t dive_trip::date() const
{
	if (dives.empty())
		return 0;
	return dives[0]->when;
}

static timestamp_t trip_enddate(const struct dive_trip &trip)
{
	if (trip.dives.empty())
		return 0;
	return trip.dives.back()->endtime();
}

/* Add dive to a trip. Caller is responsible for removing dive
 * from trip beforehand. */
void dive_trip::add_dive(struct dive *dive)
{
	if (dive->divetrip == this)
		return;
	if (dive->divetrip)
		report_info("Warning: adding dive to trip, which already has a trip set");
	range_insert_sorted(dives, dive, comp_dives_ptr);
	dive->divetrip = this;
}

/* remove a dive from the trip it's associated to, but don't delete the
 * trip if this was the last dive in the trip. the caller is responsible
 * for removing the trip, if the trip->dives.size() went to 0.
 */
struct dive_trip *unregister_dive_from_trip(struct dive *dive)
{
	dive_trip *trip = dive->divetrip;

	if (!trip)
		return NULL;

	range_remove(trip->dives, dive);
	dive->divetrip = NULL;
	return trip;
}

std::unique_ptr<dive_trip> create_trip_from_dive(const struct dive *dive)
{
	auto trip = std::make_unique<dive_trip>();
	trip->location = dive->get_location();

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
std::pair<dive_trip *, std::unique_ptr<dive_trip>> get_trip_for_new_dive(const struct divelog &log, const struct dive *new_dive)
{
	/* Find dive that is within TRIP_THRESHOLD of current dive */
	for (auto &d: log.dives) {
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

/* Check if two trips overlap time-wise up to trip threshold. */
bool trips_overlap(const struct dive_trip &t1, const struct dive_trip &t2)
{
	/* First, handle the empty-trip cases. */
	if (t1.dives.empty() || t2.dives.empty())
		return 0;

	if (t1.date() < t2.date())
		return trip_enddate(t1) + TRIP_THRESHOLD >= t2.date();
	else
		return trip_enddate(t2) + TRIP_THRESHOLD >= t1.date();
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
std::vector<dives_to_autogroup_result> get_dives_to_autogroup(const struct dive_table &table)
{
	std::vector<dives_to_autogroup_result> res;
	struct dive *lastdive = NULL;

	/* Find first dive that should be merged and remember any previous
	 * dive that could be merged into.
	 */
	for (size_t i = 0; i < table.size(); ++i) {
		auto &dive = table[i];

		if (dive->divetrip) {
			lastdive = dive.get();
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
		dive_trip *trip;
		if (!lastdive || dive->when >= lastdive->when + TRIP_THRESHOLD) {
			/* allocate new trip */
			allocated = create_trip_from_dive(dive.get());
			allocated->autogen = true;
			trip = allocated.get();
		} else {
			/* use trip of previous dive */
			trip = lastdive->divetrip;
		}

		// Now, find all dives that will be added to this trip
		lastdive = dive.get();
		size_t to;
		for (to = i + 1; to < table.size(); to++) {
			auto &dive = table[to];
			if (dive->divetrip || dive->notrip ||
			    dive->when >= lastdive->when + TRIP_THRESHOLD)
				break;
			if (trip->location.empty())
				trip->location = dive->get_location();
			lastdive = dive.get();
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
	if (a.dives.empty() && b.dives.empty())
		return &a < &b ? -1 : 1;
	if (a.dives.empty())
		return -1;
	if (b.dives.empty())
		return 1;
	return comp_dives(*a.dives[0], *b.dives[0]);
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

bool dive_trip::is_single_day() const
{
	if (dives.size() <= 1)
		return true;
	return is_same_day(dives.front()->when, dives.back()->when);
}

int dive_trip::shown_dives() const
{
	return std::count_if(dives.begin(), dives.end(),
			     [](const dive *d) { return !d->hidden_by_filter; });
}

void dive_trip::sort_dives()
{
	std::sort(dives.begin(), dives.end(), [] (dive *d1, dive *d2) { return comp_dives(*d1, *d2) < 0; });
}
