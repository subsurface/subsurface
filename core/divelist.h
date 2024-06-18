// SPDX-License-Identifier: GPL-2.0
#ifndef DIVELIST_H
#define DIVELIST_H

#include "triptable.h"
#include "divesitetable.h"
#include "units.h"
#include <memory>
#include <vector>

struct dive;
struct divelog;
struct device;
struct deco_state;

int comp_dives(const struct dive &a, const struct dive &b);
int comp_dives_ptr(const struct dive *a, const struct dive *b);

struct dive_table : public sorted_owning_table<dive, &comp_dives> {
	dive *get_by_uniq_id(int id) const;
	void record_dive(std::unique_ptr<dive> d);	// call fixup_dive() before adding dive to table.
	struct dive *register_dive(std::unique_ptr<dive> d);
	std::unique_ptr<dive> unregister_dive(int idx);

	int get_dive_nr_at_idx(int idx) const;
	timestamp_t get_surface_interval(timestamp_t when) const;
	struct dive *find_next_visible_dive(timestamp_t when);
};

/* this is used for both git and xml format */
#define DATAFORMAT_VERSION 3

extern void update_cylinder_related_info(struct dive *);
extern int init_decompression(struct deco_state *ds, const struct dive *dive, bool in_planner);

/* divelist core logic functions */
extern void process_loaded_dives();
/* flags for process_imported_dives() */
#define IMPORT_PREFER_IMPORTED (1 << 0)
#define	IMPORT_IS_DOWNLOADED (1 << 1)
#define	IMPORT_MERGE_ALL_TRIPS (1 << 2)
#define	IMPORT_ADD_TO_NEW_TRIP (1 << 3)
extern void add_imported_dives(struct divelog &log, int flags);

struct process_imported_dives_result {
	dive_table dives_to_add;
	std::vector<dive *> dives_to_remove;
	trip_table trips_to_add;
	dive_site_table sites_to_add;
	std::vector<device> devices_to_add;
};

extern process_imported_dives_result process_imported_dives(struct divelog &import_log, int flags);

extern void get_dive_gas(const struct dive *dive, int *o2_p, int *he_p, int *o2low_p);

int get_min_datafile_version();
void report_datafile_version(int version);
void clear_dive_file_data();
extern bool has_dive(unsigned int deviceid, unsigned int diveid);

#endif // DIVELIST_H
