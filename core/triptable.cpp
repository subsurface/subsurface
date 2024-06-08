// SPDX-License-Identifier: GPL-2.0

#include "triptable.h"
#include "trip.h"

#ifdef DEBUG_TRIP
void dump_trip_list()
{
	timestamp_t last_time = 0;

	for (auto &trip: divelog.trips) {
		struct tm tm;
		utc_mkdate(trip->date(), &tm);
		if (trip->date() < last_time)
			printf("\n\ntrip_table OUT OF ORDER!!!\n\n\n");
		printf("%s trip %d to \"%s\" on %04u-%02u-%02u %02u:%02u:%02u (%d dives - %p)\n",
		       trip->autogen ? "autogen " : "",
		       i + 1, trip->location.c_str(),
		       tm.tm_year, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
		       static_cast<int>(trip->dives.size()), trip.get());
		last_time = trip->date();
	}
	printf("-----\n");
}
#endif

/* lookup of trip in main trip_table based on its id */
dive_trip *trip_table::get_by_uniq_id(int tripId) const
{
	auto it = std::find_if(begin(), end(), [tripId](auto &t) { return t->id == tripId; });
	return it != end() ? it->get() : nullptr;
}
