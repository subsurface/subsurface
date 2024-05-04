// SPDX-License-Identifier: GPL-2.0
#ifndef DIVESITE_H
#define DIVESITE_H

#include "units.h"
#include "taxonomy.h"
#include "divelist.h"
#include <stdlib.h>

#ifdef __cplusplus
#include <QString>
#include <QObject>

struct dive_site
{
	uint32_t uuid = 0;
	char *name = nullptr;
	struct dive_table dives = { 0, 0, nullptr };
	location_t location = { { 9 }, { 0 } };
	char *description = nullptr;
	char *notes = nullptr;
	taxonomy_data taxonomy;
	dive_site();
	dive_site(const char *name);
	dive_site(const char *name, const location_t *loc);
	~dive_site();
};

typedef struct dive_site_table {
	int nr, allocated;
	struct dive_site **dive_sites;
} dive_site_table_t;

static const dive_site_table_t empty_dive_site_table = { 0, 0, (struct dive_site **)0 };

static inline struct dive_site *get_dive_site(int nr, struct dive_site_table *ds_table)
{
	if (nr >= ds_table->nr || nr < 0)
		return NULL;
	return ds_table->dive_sites[nr];
}

/* iterate over each dive site */
#define for_each_dive_site(_i, _x, _ds_table) \
	for ((_i) = 0; ((_x) = get_dive_site(_i, _ds_table)) != NULL; (_i)++)

int get_divesite_idx(const struct dive_site *ds, struct dive_site_table *ds_table);
struct dive_site *get_dive_site_by_uuid(uint32_t uuid, struct dive_site_table *ds_table);
void sort_dive_site_table(struct dive_site_table *ds_table);
int add_dive_site_to_table(struct dive_site *ds, struct dive_site_table *ds_table);
struct dive_site *alloc_or_get_dive_site(uint32_t uuid, struct dive_site_table *ds_table);
struct dive_site *alloc_dive_site();
int nr_of_dives_at_dive_site(struct dive_site *ds);
bool is_dive_site_selected(struct dive_site *ds);
int unregister_dive_site(struct dive_site *ds);
int register_dive_site(struct dive_site *ds);
void delete_dive_site(struct dive_site *ds, struct dive_site_table *ds_table);
struct dive_site *create_dive_site(const char *name, struct dive_site_table *ds_table);
struct dive_site *create_dive_site_with_gps(const char *name, const location_t *, struct dive_site_table *ds_table);
struct dive_site *get_dive_site_by_name(const char *name, struct dive_site_table *ds_table);
struct dive_site *get_dive_site_by_gps(const location_t *, struct dive_site_table *ds_table);
struct dive_site *get_dive_site_by_gps_and_name(const char *name, const location_t *, struct dive_site_table *ds_table);
struct dive_site *get_dive_site_by_gps_proximity(const location_t *, int distance, struct dive_site_table *ds_table);
struct dive_site *get_same_dive_site(const struct dive_site *);
bool dive_site_is_empty(struct dive_site *ds);
void copy_dive_site_taxonomy(struct dive_site *orig, struct dive_site *copy);
void copy_dive_site(struct dive_site *orig, struct dive_site *copy);
void merge_dive_site(struct dive_site *a, struct dive_site *b);
unsigned int get_distance(const location_t *loc1, const location_t *loc2);
struct dive_site *find_or_create_dive_site_with_name(const char *name, struct dive_site_table *ds_table);
void purge_empty_dive_sites(struct dive_site_table *ds_table);
void clear_dive_site_table(struct dive_site_table *ds_table);
void move_dive_site_table(struct dive_site_table *src, struct dive_site_table *dst);
void add_dive_to_dive_site(struct dive *d, struct dive_site *ds);
struct dive_site *unregister_dive_from_dive_site(struct dive *d);

QString constructLocationTags(taxonomy_data &taxonomy, bool for_maintab);

/* Make pointer-to-dive_site a "Qt metatype" so that we can pass it through QVariants */
Q_DECLARE_METATYPE(dive_site *);

#endif

#endif // DIVESITE_H
