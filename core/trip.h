// SPDX-License-Identifier: GPL-2.0
#ifndef TRIP_H
#define TRIP_H

#include "divelist.h"

extern "C" {

typedef struct dive_trip
{
	char *location;
	char *notes;
	struct dive_table dives;
	int id; /* unique ID for this trip: used to pass trips through QML. */
	/* Used by the io-routines to mark trips that have already been written. */
	bool saved;
	bool autogen;
	bool selected;
} dive_trip_t;

typedef struct trip_table {
	int nr, allocated;
	struct dive_trip **trips;
} trip_table_t;

static const trip_table_t empty_trip_table = { 0, 0, (struct dive_trip **)0 };

extern void add_dive_to_trip(struct dive *, dive_trip_t *);
extern struct dive_trip *unregister_dive_from_trip(struct dive *dive);
extern void remove_dive_from_trip(struct dive *dive, struct trip_table *trip_table_arg);

extern void insert_trip(dive_trip_t *trip, struct trip_table *trip_table_arg);
extern int remove_trip(const dive_trip_t *trip, struct trip_table *trip_table_arg);
extern void free_trip(dive_trip_t *trip);
extern timestamp_t trip_date(const struct dive_trip *trip);
extern timestamp_t trip_enddate(const struct dive_trip *trip);

extern bool trip_less_than(const struct dive_trip *a, const struct dive_trip *b);
extern int comp_trips(const struct dive_trip *a, const struct dive_trip *b);
extern void sort_trip_table(struct trip_table *table);

extern dive_trip_t *alloc_trip(void);
extern dive_trip_t *create_trip_from_dive(struct dive *dive);
extern dive_trip_t *get_dives_to_autogroup(struct dive_table *table, int start, int *from, int *to, bool *allocated);
extern dive_trip_t *get_trip_for_new_dive(struct dive *new_dive, bool *allocated);
extern dive_trip_t *get_trip_by_uniq_id(int tripId);
extern bool trips_overlap(const struct dive_trip *t1, const struct dive_trip *t2);

extern dive_trip_t *combine_trips(struct dive_trip *trip_a, struct dive_trip *trip_b);
extern bool is_trip_before_after(const struct dive *dive, bool before);
extern bool trip_is_single_day(const struct dive_trip *trip);
extern int trip_shown_dives(const struct dive_trip *trip);

void move_trip_table(struct trip_table *src, struct trip_table *dst);
void clear_trip_table(struct trip_table *table);

#ifdef DEBUG_TRIP
extern void dump_trip_list(void);
#endif

}

/* Make pointers to dive_trip and trip_table "Qt metatypes" so that they can be
 * passed through QVariants and through QML. See comment in dive.h. */
#include <QObject>
Q_DECLARE_METATYPE(struct dive_trip *);
Q_DECLARE_METATYPE(trip_table_t *);

#endif
