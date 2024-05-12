// SPDX-License-Identifier: GPL-2.0
#ifndef DIVESITE_H
#define DIVESITE_H

#include "divelist.h"
#include "owning_table.h"
#include "taxonomy.h"
#include "units.h"
#include <stdlib.h>

#include <QObject>

struct dive_site
{
	uint32_t uuid = 0;
	std::string name;
	std::vector<dive *> dives;
	location_t location;
	std::string description;
	std::string notes;
	taxonomy_data taxonomy;

	dive_site();
	dive_site(const std::string &name);
	dive_site(const std::string &name, const location_t loc);
	dive_site(uint32_t uuid);
	~dive_site();

	size_t nr_of_dives() const;
	bool is_selected() const;
	bool is_empty() const;
	void merge(struct dive_site &b); // Note: b is consumed
	void add_dive(struct dive *d);
};

inline int divesite_comp_uuid(const dive_site &ds1, const dive_site &ds2)
{
	if (ds1.uuid == ds2.uuid)
		return 0;
	return ds1.uuid < ds2.uuid ? -1 : 1;
}

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

struct dive_site *unregister_dive_from_dive_site(struct dive *d);

/* Make pointer-to-dive_site a "Qt metatype" so that we can pass it through QVariants */
Q_DECLARE_METATYPE(dive_site *);

#endif // DIVESITE_H
