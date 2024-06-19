// SPDX-License-Identifier: GPL-2.0
// A structure that contains all the data we store in divelog files
#ifndef DIVELOG_H
#define DIVELOG_H

#include "divelist.h"
#include "divesitetable.h"
#include "filterpresettable.h"
#include "triptable.h"

#include <vector>

struct device;

/* flags for process_imported_dives() */
struct import_flags {
	static constexpr int prefer_imported = 1 << 0;
	static constexpr int is_downloaded = 1 << 1;
	static constexpr int merge_all_trips = 1 << 2;
	static constexpr int add_to_new_trip = 1 << 3;
};

struct divelog {
	dive_table dives;
	trip_table trips;
	dive_site_table sites;
	std::vector<device> devices;
	filter_preset_table filter_presets;
	bool autogroup = false;

	divelog();
	~divelog();
	divelog(divelog &&); // move constructor (argument is consumed).
	divelog &operator=(divelog &&); // move assignment (argument is consumed).

	void delete_single_dive(int idx);
	void delete_multiple_dives(const std::vector<dive *> &dives);
	void clear();
	bool is_trip_before_after(const struct dive *dive, bool before) const;

	struct process_imported_dives_result {
		dive_table dives_to_add;
		std::vector<dive *> dives_to_remove;
		trip_table trips_to_add;
		dive_site_table sites_to_add;
		std::vector<device> devices_to_add;
	};

	/* divelist core logic functions */
	process_imported_dives_result process_imported_dives(struct divelog &import_log, int flags); // import_log will be consumed
	void process_loaded_dives();
	void add_imported_dives(struct divelog &log, int flags); // log will be consumed
};

extern struct divelog divelog;

#endif
