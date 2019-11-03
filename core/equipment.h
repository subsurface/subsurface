// SPDX-License-Identifier: GPL-2.0
#ifndef EQUIPMENT_H
#define EQUIPMENT_H

#include "gas.h"

#ifdef __cplusplus
extern "C" {
#endif

struct dive;

enum cylinderuse {OC_GAS, DILUENT, OXYGEN, NOT_USED, NUM_GAS_USE}; // The different uses for cylinders

typedef struct
{
	volume_t size;
	pressure_t workingpressure;
	const char *description; /* "LP85", "AL72", "AL80", "HP100+" or whatever */
} cylinder_type_t;

typedef struct
{
	cylinder_type_t type;
	struct gasmix gasmix;
	pressure_t start, end, sample_start, sample_end;
	depth_t depth;
	bool manually_added;
	volume_t gas_used;
	volume_t deco_gas_used;
	enum cylinderuse cylinder_use;
	bool bestmix_o2;
	bool bestmix_he;
} cylinder_t;

/* Table of cylinders. Attention: this stores cylinders,
 * *not* pointers to cylinders. This has two crucial consequences:
 * 1) Pointers to cylinders are not stable. They may be
 *    invalidated if the table is reallocated.
 * 2) add_to_cylinder_table(), etc. take ownership of the
 *    cylinder. Notably of the description string. */
struct cylinder_table {
	int nr, allocated;
	cylinder_t *cylinders;
};

typedef struct
{
	weight_t weight;
	const char *description; /* "integrated", "belt", "ankle" */
} weightsystem_t;

/* Table of weightsystems. Attention: this stores weightsystems,
 * *not* pointers * to weightsystems. This has two crucial
 * consequences:
 * 1) Pointers to weightsystems are not stable. They may be
 *    invalidated if the table is reallocated.
 * 2) add_to_weightsystem_table(), etc. takes ownership of the
 *    weightsystem. Notably of the description string */
struct weightsystem_table {
	int nr, allocated;
	weightsystem_t *weightsystems;
};

#define MAX_TANK_INFO (100)
#define MAX_WS_INFO (100)

extern int cylinderuse_from_text(const char *text);
extern void copy_weights(const struct weightsystem_table *s, struct weightsystem_table *d);
extern weightsystem_t clone_weightsystem(weightsystem_t ws);
extern void copy_cylinder_types(const struct dive *s, struct dive *d);
extern void add_cloned_weightsystem(struct weightsystem_table *t, weightsystem_t ws);
extern cylinder_t *add_empty_cylinder(struct cylinder_table *t);
extern void add_cloned_cylinder(struct cylinder_table *t, cylinder_t cyl);
extern cylinder_t *get_cylinder(const struct dive *d, int idx);
extern cylinder_t *get_or_create_cylinder(struct dive *d, int idx);
extern void add_cylinder_description(const cylinder_type_t *);
extern void add_weightsystem_description(const weightsystem_t *);
extern bool same_weightsystem(weightsystem_t w1, weightsystem_t w2);
extern bool same_cylinder(cylinder_t cyl1, cylinder_t cyl2);
extern void remove_cylinder(struct dive *dive, int idx);
extern void remove_weightsystem(struct dive *dive, int idx);
extern void reset_cylinders(struct dive *dive, bool track_gas);
extern int gas_volume(const cylinder_t *cyl, pressure_t p); /* Volume in mliter of a cylinder at pressure 'p' */
extern int find_best_gasmix_match(struct gasmix mix, const struct cylinder_table *cylinders);
#ifdef DEBUG_CYL
extern void dump_cylinders(struct dive *dive, bool verbose);
#endif

/* Weightsystem table functions */
extern void clear_weightsystem_table(struct weightsystem_table *);
extern void add_to_weightsystem_table(struct weightsystem_table *, int idx, weightsystem_t ws);

/* Cylinder table functions */
extern void clear_cylinder_table(struct cylinder_table *);
extern void add_to_cylinder_table(struct cylinder_table *, int idx, cylinder_t cyl);

void get_gas_string(struct gasmix gasmix, char *text, int len);
const char *gasname(struct gasmix gasmix);

struct tank_info_t {
	const char *name;
	int cuft, ml, psi, bar;
};
extern struct tank_info_t tank_info[MAX_TANK_INFO];

struct ws_info_t {
	const char *name;
	int grams;
};
extern struct ws_info_t ws_info[MAX_WS_INFO];

#ifdef __cplusplus
}
#endif

#endif // EQUIPMENT_H
