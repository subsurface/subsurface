// SPDX-License-Identifier: GPL-2.0
#ifndef DIVESITETABLE_H
#define DIVESITETABLE_H

#include "owning_table.h"
#include "units.h"

struct dive_site;
int divesite_comp_uuid(const dive_site &ds1, const dive_site &ds2);

class dive_site_table : public sorted_owning_table<dive_site, &divesite_comp_uuid> {
public:
	put_result register_site(std::unique_ptr<dive_site> site); // Creates or changes UUID if duplicate
	dive_site *get_by_uuid(uint32_t uuid) const;
	dive_site *alloc_or_get(uint32_t uuid);
	dive_site *create(const std::string &name);
	dive_site *create(const std::string &name, const location_t);
	dive_site *find_or_create(const std::string &name);
	dive_site *get_by_name(const std::string &name) const;
	dive_site *get_by_gps(const location_t *) const;
	dive_site *get_by_gps_and_name(const std::string &name, const location_t) const;
	dive_site *get_by_gps_proximity(location_t, int distance) const;
	dive_site *get_same(const struct dive_site &) const;
	void purge_empty();
};

#endif // DIVESITETABLE_H
