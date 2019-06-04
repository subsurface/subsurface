// SPDX-License-Identifier: GPL-2.0
#ifndef GAS_H
#define GAS_H

#include "units.h"

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif

// o2 == 0 && he == 0 -> air
// o2 < 0 -> invalid
struct gasmix {
	fraction_t o2;
	fraction_t he;
};
static const struct gasmix gasmix_invalid = { { -1 }, { -1 } };
static const struct gasmix gasmix_air = { { 0 }, { 0 } };

struct icd_data { // This structure provides communication between function isobaric_counterdiffusion() and the calling software.
	int dN2;      // The change in fraction (permille) of nitrogen during the change
	int dHe;      // The change in fraction (permille) of helium during the change
};

extern bool isobaric_counterdiffusion(struct gasmix oldgasmix, struct gasmix newgasmix, struct icd_data *results);

extern double gas_compressibility_factor(struct gasmix gas, double bar);
extern double isothermal_pressure(struct gasmix gas, double p1, int volume1, int volume2);
extern double gas_density(struct gasmix gas, int pressure);
extern int same_gasmix(struct gasmix a, struct gasmix b);

static inline int get_o2(struct gasmix mix)
{
	return mix.o2.permille ?: O2_IN_AIR;
}

static inline int get_he(struct gasmix mix)
{
	return mix.he.permille;
}

struct gas_pressures {
	double o2, n2, he;
};

extern void sanitize_gasmix(struct gasmix *mix);
extern int gasmix_distance(struct gasmix a, struct gasmix b);

extern bool gasmix_is_air(struct gasmix gasmix);

#ifdef __cplusplus
}
#endif

#endif
