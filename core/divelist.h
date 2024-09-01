// SPDX-License-Identifier: GPL-2.0
#ifndef DIVELIST_H
#define DIVELIST_H

#include "triptable.h"
#include "divesitetable.h"
#include "units.h"
#include <memory>
#include <optional>

struct dive;
struct divelog;
struct device;
struct deco_state;

int comp_dives(const struct dive &a, const struct dive &b);
int comp_dives_ptr(const struct dive *a, const struct dive *b);

struct merge_result {
	std::unique_ptr<struct dive> dive;
	std::optional<location_t> set_location; // change location of dive site
};

struct dive_table : public sorted_owning_table<dive, &comp_dives> {
	dive *get_by_uniq_id(int id) const;
	void record_dive(std::unique_ptr<dive> d);	// call fixup_dive() before adding dive to table.
	struct dive *register_dive(std::unique_ptr<dive> d);
	std::unique_ptr<dive> unregister_dive(int idx);
	std::unique_ptr<dive> default_dive();		// generate a sensible looking defaultdive 1h from now.

	// Some of these functions act on dives, but need data from adjacent dives,
	// notably to calculate CNS, surface interval, etc. Therefore, they are called
	// on the dive_table and not on the dive.
	void fixup_dive(struct dive &dive) const;
	void force_fixup_dive(struct dive &d) const;
	int init_decompression(struct deco_state *ds, const struct dive *dive, bool in_planner) const;
	void update_cylinder_related_info(struct dive &dive) const;
	int get_dive_nr_at_idx(int idx) const;
	timestamp_t get_surface_interval(timestamp_t when) const;
	struct dive *find_next_visible_dive(timestamp_t when);
	std::array<std::unique_ptr<dive>, 2> split_divecomputer(const struct dive &src, int num) const;
	std::array<std::unique_ptr<dive>, 2> split_dive(const struct dive &dive) const;
	std::array<std::unique_ptr<dive>, 2> split_dive_at_time(const struct dive &dive, duration_t time) const;
	merge_result merge_dives(const std::vector<dive *> &dives) const;
	std::unique_ptr<dive> try_to_merge(const struct dive &a, const struct dive &b, bool prefer_downloaded) const;
	bool has_dive(unsigned int deviceid, unsigned int diveid) const;
	std::unique_ptr<dive> clone_delete_divecomputer(const struct dive &d, int dc_number);
private:
	int calculate_cns(struct dive &dive) const; // Note: writes into dive->cns
	std::array<std::unique_ptr<dive>, 2> split_dive_at(const struct dive &dive, int a, int b) const;
	std::unique_ptr<dive> merge_two_dives(const struct dive &a_in, const struct dive &b_in, int offset, bool prefer_downloaded) const;
};

void clear_dive_file_data();

#endif // DIVELIST_H
