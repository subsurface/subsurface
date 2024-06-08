// SPDX-License-Identifier: GPL-2.0
#ifndef TRIP_H
#define TRIP_H

#include "divelist.h"

struct divelog;

struct dive_trip
{
	std::string location;
	std::string notes;
	std::vector<dive *> dives;
	int id; /* unique ID for this trip: used to pass trips through QML. */
	/* Used by the io-routines to mark trips that have already been written. */
	bool saved = false;
	bool autogen = false;
	bool selected = false;

	dive_trip();
	~dive_trip();

	void sort_dives();
	void add_dive(struct dive *);
	timestamp_t date() const;
	bool is_single_day() const;
	int shown_dives() const;
};

int comp_trips(const dive_trip &t1, const dive_trip &t2);

extern struct dive_trip *unregister_dive_from_trip(struct dive *dive);

extern std::unique_ptr<dive_trip> create_trip_from_dive(const struct dive *dive);

// Result item of get_dives_to_autogroup()
struct dives_to_autogroup_result {
	size_t from, to;	// Group dives in the range [from, to)
	dive_trip *trip;	// Pointer to trip
	std::unique_ptr<dive_trip> created_trip;
				// Is set if the trip was newly created - caller has to store it.
};

extern std::vector<dives_to_autogroup_result> get_dives_to_autogroup(const struct dive_table &table);
extern std::pair<dive_trip *, std::unique_ptr<dive_trip>> get_trip_for_new_dive(const struct divelog &log, const struct dive *new_dive);
extern bool trips_overlap(const struct dive_trip &t1, const struct dive_trip &t2);

extern std::unique_ptr<dive_trip> combine_trips(struct dive_trip *trip_a, struct dive_trip *trip_b);

/* Make pointers to dive_trip "Qt metatypes" so that they can be
 * passed through QVariants and through QML. See comment in dive.h. */
#include <QObject>
Q_DECLARE_METATYPE(struct dive_trip *);

#endif
