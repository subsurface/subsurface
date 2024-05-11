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
	dive_site(const std::string &name, const location_t *loc);
	dive_site(uint32_t uuid);
	~dive_site();
};

inline int divesite_comp_uuid(const dive_site &ds1, const dive_site &ds2)
{
	if (ds1.uuid == ds2.uuid)
		return 0;
	return ds1.uuid < ds2.uuid ? -1 : 1;
}

class dive_site_table : public sorted_owning_table<dive_site, &divesite_comp_uuid> {
public:
	put_result register_site(std::unique_ptr<dive_site> site);
};

int get_divesite_idx(const struct dive_site *ds, dive_site_table &ds_table);
struct dive_site *get_dive_site_by_uuid(uint32_t uuid, dive_site_table &ds_table);
struct dive_site *alloc_or_get_dive_site(uint32_t uuid, dive_site_table &ds_table);
size_t nr_of_dives_at_dive_site(const struct dive_site &ds);
bool is_dive_site_selected(const struct dive_site &ds);
struct dive_site *create_dive_site(const std::string &name, dive_site_table &ds_table);
struct dive_site *create_dive_site_with_gps(const std::string &name, const location_t *, dive_site_table &ds_table);
struct dive_site *get_dive_site_by_name(const std::string &name, dive_site_table &ds_table);
struct dive_site *get_dive_site_by_gps(const location_t *, dive_site_table &ds_table);
struct dive_site *get_dive_site_by_gps_and_name(const std::string &name, const location_t *, dive_site_table &ds_table);
struct dive_site *get_dive_site_by_gps_proximity(const location_t *, int distance, dive_site_table &ds_table);
struct dive_site *get_same_dive_site(const struct dive_site &);
bool dive_site_is_empty(struct dive_site *ds);
void merge_dive_site(struct dive_site *a, struct dive_site *b);
unsigned int get_distance(const location_t *loc1, const location_t *loc2);
struct dive_site *find_or_create_dive_site_with_name(const std::string &name, dive_site_table &ds_table);
void purge_empty_dive_sites(dive_site_table &ds_table);
void add_dive_to_dive_site(struct dive *d, struct dive_site *ds);
struct dive_site *unregister_dive_from_dive_site(struct dive *d);
std::string constructLocationTags(const taxonomy_data &taxonomy, bool for_maintab);

/* Make pointer-to-dive_site a "Qt metatype" so that we can pass it through QVariants */
Q_DECLARE_METATYPE(dive_site *);

#endif // DIVESITE_H
