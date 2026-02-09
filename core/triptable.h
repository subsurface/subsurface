// SPDX-License-Identifier: GPL-2.0
#ifndef TRIPTABLE_H
#define TRIPTABLE_H

#include "owning_table.h"

struct dive_trip;

int comp_trips(const dive_trip &t1, const dive_trip &t2);

struct trip_table : public sorted_owning_table<dive_trip, &comp_trips> {
	dive_trip *get_by_uniq_id(int tripId) const;
};

#ifdef DEBUG_TRIP
extern void dump_trip_list();
#endif

/* Make pointers to trip_table "Qt metatypes" so that they can be
 * passed through QVariants and through QML. See comment in dive.h. */
#include <QObject>
Q_DECLARE_METATYPE(trip_table *);

#endif
