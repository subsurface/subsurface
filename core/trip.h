// SPDX-License-Identifier: GPL-2.0
#ifndef TRIP_H
#define TRIP_H

#include "divelist.h"
#include "owning_table.h"

#include <string>

struct dive_trip
{
	std::string location;
	std::string notes;
	struct dive_table dives = {};
	int id; /* unique ID for this trip: used to pass trips through QML. */
	/* Used by the io-routines to mark trips that have already been written. */
	bool saved = false;
	bool autogen = false;
	bool selected = false;

	dive_trip();
	~dive_trip();
};

int comp_trips(const dive_trip &t1, const dive_trip &t2);

struct trip_table : public sorted_owning_table<dive_trip, &comp_trips> {
	dive_trip *get_by_uniq_id(int tripId) const;
};

extern void add_dive_to_trip(struct dive *, dive_trip *);
extern struct dive_trip *unregister_dive_from_trip(struct dive *dive);

extern timestamp_t trip_date(const struct dive_trip &trip);

extern std::unique_ptr<dive_trip> create_trip_from_dive(const struct dive *dive);
extern dive_trip *create_and_hookup_trip_from_dive(const struct dive *dive, struct trip_table &trip_table_arg);

// Result item of get_dives_to_autogroup()
struct dives_to_autogroup_result {
	int from, to;		// Group dives in the range [from, to)
	dive_trip *trip;	// Pointer to trip
	std::unique_ptr<dive_trip> created_trip;
				// Is set if the trip was newly created - caller has to store it.
};

extern std::vector<dives_to_autogroup_result> get_dives_to_autogroup(struct dive_table *table);
extern std::pair<dive_trip *, std::unique_ptr<dive_trip>> get_trip_for_new_dive(const struct dive *new_dive);
extern bool trips_overlap(const struct dive_trip &t1, const struct dive_trip &t2);

extern std::unique_ptr<dive_trip> combine_trips(struct dive_trip *trip_a, struct dive_trip *trip_b);
extern bool is_trip_before_after(const struct dive *dive, bool before);
extern bool trip_is_single_day(const struct dive_trip &trip);
extern int trip_shown_dives(const struct dive_trip *trip);

#ifdef DEBUG_TRIP
extern void dump_trip_list();
#endif

/* Make pointers to dive_trip and trip_table "Qt metatypes" so that they can be
 * passed through QVariants and through QML. See comment in dive.h. */
#include <QObject>
Q_DECLARE_METATYPE(struct dive_trip *);
Q_DECLARE_METATYPE(trip_table *);

#endif
