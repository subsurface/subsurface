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

typedef struct
{
	weight_t weight;
	const char *description; /* "integrated", "belt", "ankle" */
} weightsystem_t;

#define MAX_CYLINDERS (20)
#define MAX_WEIGHTSYSTEMS (6)
#define MAX_TANK_INFO (100)
#define MAX_WS_INFO (100)

extern int cylinderuse_from_text(const char *text);
extern void add_cylinder_description(const cylinder_type_t *);
extern void add_weightsystem_description(const weightsystem_t *);
extern bool same_weightsystem(weightsystem_t w1, weightsystem_t w2);
extern bool cylinder_nodata(const cylinder_t *cyl);
extern bool cylinder_none(const cylinder_t *cyl);
extern bool weightsystem_none(const weightsystem_t *ws);
extern void remove_cylinder(struct dive *dive, int idx);
extern void remove_weightsystem(struct dive *dive, int idx);
extern void reset_cylinders(struct dive *dive, bool track_gas);
extern int gas_volume(const cylinder_t *cyl, pressure_t p); /* Volume in mliter of a cylinder at pressure 'p' */
extern int find_best_gasmix_match(struct gasmix mix, const cylinder_t array[]);
#ifdef DEBUG_CYL
extern void dump_cylinders(struct dive *dive, bool verbose);
#endif

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
