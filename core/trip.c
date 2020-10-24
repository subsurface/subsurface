// SPDX-License-Identifier: GPL-2.0

#include "trip.h"
#include "dive.h"
#include "subsurface-time.h"
#include "subsurface-string.h"
#include "selection.h"
#include "table.h"
#include "core/qthelper.h"

struct trip_table trip_table;

#ifdef DEBUG_TRIP
void dump_trip_list(void)
{
	dive_trip_t *trip;
	int i = 0;
	timestamp_t last_time = 0;

	for (i = 0; i < trip_table.nr; ++i) {
		struct tm tm;
		trip = trip_table.trips[i];
		utc_mkdate(trip_date(trip), &tm);
		if (trip_date(trip) < last_time)
			printf("\n\ntrip_table OUT OF ORDER!!!\n\n\n");
		printf("%s trip %d to \"%s\" on %04u-%02u-%02u %02u:%02u:%02u (%d dives - %p)\n",
		       trip->autogen ? "autogen " : "",
		       i + 1, trip->location,
		       tm.tm_year, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
		       trip->dives.nr, trip);
		last_time = trip_date(trip);
	}
	printf("-----\n");
}
#endif

/* free resources associated with a trip structure */
void free_trip(dive_trip_t *trip)
{
	if (trip) {
		free(trip->location);
		free(trip->notes);
		free(trip->dives.dives);
		free(trip);
	}
}

/* Trip table functions */
static MAKE_GET_IDX(trip_table, struct dive_trip *, trips)
static MAKE_GROW_TABLE(trip_table, struct dive_trip *, trips)
static MAKE_GET_INSERTION_INDEX(trip_table, struct dive_trip *, trips, trip_less_than)
static MAKE_ADD_TO(trip_table, struct dive_trip *, trips)
static MAKE_REMOVE_FROM(trip_table, trips)
MAKE_SORT(trip_table, struct dive_trip *, trips, comp_trips)
MAKE_REMOVE(trip_table, struct dive_trip *, trip)
MAKE_CLEAR_TABLE(trip_table, trips, trip)

timestamp_t trip_date(const struct dive_trip *trip)
{
	if (!trip || trip->dives.nr == 0)
		return 0;
	return trip->dives.dives[0]->when;
}

timestamp_t trip_enddate(const struct dive_trip *trip)
{
	if (!trip || trip->dives.nr == 0)
		return 0;
	return dive_endtime(trip->dives.dives[trip->dives.nr - 1]);
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
void add_dive_to_trip(struct dive *dive, dive_trip_t *trip)
{
	if (dive->divetrip == trip)
		return;
	if (dive->divetrip)
		SSRF_INFO("Warning: adding dive to trip that has trip set\n");
	insert_dive(&trip->dives, dive);
	dive->divetrip = trip;
}

/* remove a dive from the trip it's associated to, but don't delete the
 * trip if this was the last dive in the trip. the caller is responsible
 * for removing the trip, if the trip->dives.nr went to 0.
 */
struct dive_trip *unregister_dive_from_trip(struct dive *dive)
{
	dive_trip_t *trip = dive->divetrip;

	if (!trip)
		return NULL;

	remove_dive(dive, &trip->dives);
	dive->divetrip = NULL;
	return trip;
}

static void delete_trip(dive_trip_t *trip, struct trip_table *trip_table_arg)
{
	remove_trip(trip, trip_table_arg);
	free_trip(trip);
}

void remove_dive_from_trip(struct dive *dive, struct trip_table *trip_table_arg)
{
	struct dive_trip *trip = unregister_dive_from_trip(dive);
	if (trip && trip->dives.nr == 0)
		delete_trip(trip, trip_table_arg);
}

dive_trip_t *alloc_trip(void)
{
	dive_trip_t *res = calloc(1, sizeof(dive_trip_t));
	res->id = dive_getUniqID();
	return res;
}

/* insert the trip into the trip table */
void insert_trip(dive_trip_t *dive_trip, struct trip_table *trip_table_arg)
{
	int idx = trip_table_get_insertion_index(trip_table_arg, dive_trip);
	add_to_trip_table(trip_table_arg, idx, dive_trip);
#ifdef DEBUG_TRIP
	dump_trip_list();
#endif
}

dive_trip_t *create_trip_from_dive(struct dive *dive)
{
	dive_trip_t *trip;

	trip = alloc_trip();
	trip->location = copy_string(get_dive_location(dive));

	return trip;
}

dive_trip_t *create_and_hookup_trip_from_dive(struct dive *dive, struct trip_table *trip_table_arg)
{
	dive_trip_t *dive_trip;

	dive_trip = create_trip_from_dive(dive);

	add_dive_to_trip(dive, dive_trip);
	insert_trip(dive_trip, trip_table_arg);
	return dive_trip;
}

/* random threshold: three days without diving -> new trip
 * this works very well for people who usually dive as part of a trip and don't
 * regularly dive at a local facility; this is why trips are an optional feature */
#define TRIP_THRESHOLD 3600 * 24 * 3

/*
 * Find a trip a new dive should be autogrouped with. If no such trips
 * exist, allocate a new trip. The bool "*allocated" is set to true
 * if a new trip was allocated.
 */
dive_trip_t *get_trip_for_new_dive(struct dive *new_dive, bool *allocated)
{
	struct dive *d;
	dive_trip_t *trip;
	int i;

	/* Find dive that is within TRIP_THRESHOLD of current dive */
	for_each_dive(i, d) {
		/* Check if we're past the range of possible dives */
		if (d->when >= new_dive->when + TRIP_THRESHOLD)
			break;

		if (d->when + TRIP_THRESHOLD >= new_dive->when && d->divetrip) {
			/* Found a dive with trip in the range */
			*allocated = false;
			return d->divetrip;
		}
	}

	/* Didn't find a trip -> allocate a new one */
	trip = create_trip_from_dive(new_dive);
	trip->autogen = true;
	*allocated = true;
	return trip;
}

/* lookup of trip in main trip_table based on its id */
dive_trip_t *get_trip_by_uniq_id(int tripId)
{
	for (int i = 0; i < trip_table.nr; i++) {
		if (trip_table.trips[i]->id == tripId)
			return trip_table.trips[i];
	}
	return NULL;
}

/* Check if two trips overlap time-wise up to trip threshold. */
bool trips_overlap(const struct dive_trip *t1, const struct dive_trip *t2)
{
	/* First, handle the empty-trip cases. */
	if (t1->dives.nr == 0 || t2->dives.nr == 0)
		return 0;

	if (trip_date(t1) < trip_date(t2))
		return trip_enddate(t1) + TRIP_THRESHOLD >= trip_date(t2);
	else
		return trip_enddate(t2) + TRIP_THRESHOLD >= trip_date(t1);
}

/*
 * Collect dives for auto-grouping. Pass in first dive which should be checked.
 * Returns range of dives that should be autogrouped and trip it should be
 * associated to. If the returned trip was newly allocated, the last bool
 * is set to true. Caller still has to register it in the system. Note
 * whereas this looks complicated - it is needed by the undo-system, which
 * manually injects the new trips. If there are no dives to be autogrouped,
 * return NULL.
 */
dive_trip_t *get_dives_to_autogroup(struct dive_table *table, int start, int *from, int *to, bool *allocated)
{
	int i;
	struct dive *lastdive = NULL;

	/* Find first dive that should be merged and remember any previous
	 * dive that could be merged into.
	 */
	for (i = start; i < table->nr; i++) {
		struct dive *dive = table->dives[i];
		dive_trip_t *trip;

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
		if (!lastdive || dive->when >= lastdive->when + TRIP_THRESHOLD) {
			/* allocate new trip */
			trip = create_trip_from_dive(dive);
			trip->autogen = true;
			*allocated = true;
		} else {
			/* use trip of previous dive */
			trip = lastdive->divetrip;
			*allocated = false;
		}

		// Now, find all dives that will be added to this trip
		lastdive = dive;
		*from = i;
		for (*to = *from + 1; *to < table->nr; (*to)++) {
			dive = table->dives[*to];
			if (dive->divetrip || dive->notrip ||
			    dive->when >= lastdive->when + TRIP_THRESHOLD)
				break;
			if (get_dive_location(dive) && !trip->location)
				trip->location = copy_string(get_dive_location(dive));
			lastdive = dive;
		}
		return trip;
	}

	/* Did not find anyhting - mark as end */
	return NULL;
}

void deselect_dives_in_trip(struct dive_trip *trip)
{
	if (!trip)
		return;
	for (int i = 0; i < trip->dives.nr; ++i)
		deselect_dive(trip->dives.dives[i]);
}

void select_dives_in_trip(struct dive_trip *trip)
{
	struct dive *dive;
	if (!trip)
		return;
	for (int i = 0; i < trip->dives.nr; ++i) {
		dive = trip->dives.dives[i];
		if (!dive->hidden_by_filter)
			select_dive(dive);
	}
}

/* Out of two strings, copy the string that is not empty (if any). */
static char *copy_non_empty_string(const char *a, const char *b)
{
	return copy_string(empty_string(b) ? a : b);
}

/* This combines the information of two trips, generating a
 * new trip. To support undo, we have to preserve the old trips. */
dive_trip_t *combine_trips(struct dive_trip *trip_a, struct dive_trip *trip_b)
{
	dive_trip_t *trip;

	trip = alloc_trip();
	trip->location = copy_non_empty_string(trip_a->location, trip_b->location);
	trip->notes = copy_non_empty_string(trip_a->notes, trip_b->notes);

	return trip;
}

/* Trips are compared according to the first dive in the trip. */
int comp_trips(const struct dive_trip *a, const struct dive_trip *b)
{
	/* This should never happen, nevertheless don't crash on trips
	 * with no (or worse a negative number of) dives. */
	if (a->dives.nr <= 0)
		return b->dives.nr <= 0 ? 0 : -1;
	if (b->dives.nr <= 0)
		return 1;
	return comp_dives(a->dives.dives[0], b->dives.dives[0]);
}

bool trip_less_than(const struct dive_trip *a, const struct dive_trip *b)
{
	return comp_trips(a, b) < 0;
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

bool trip_is_single_day(const struct dive_trip *trip)
{
	if (trip->dives.nr <= 1)
		return true;
	return is_same_day(trip->dives.dives[0]->when,
			   trip->dives.dives[trip->dives.nr - 1]->when);
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
