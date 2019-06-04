// SPDX-License-Identifier: GPL-2.0
#include "gas.h"
#include "pref.h"
#include <stdio.h>
#include <string.h>

/* Perform isobaric counterdiffusion calculations for gas changes in trimix dives.
 * Here we use the rule-of-fifths where, during a change involving trimix gas, the increase in nitrogen
 * should not exceed one fifth of the decrease in helium.
 * Parameters: 1) pointers to two gas mixes, the gas being switched from and the gas being switched to.
 *             2) a pointer to an icd_data structure.
 * Output:     i) The icd_data stucture is filled with the delta_N2 and delta_He numbers (as permille).
 *            ii) Function returns a boolean indicating an exceeding of the rule-of-fifths. False = no icd problem.
 */
bool isobaric_counterdiffusion(struct gasmix oldgasmix, struct gasmix newgasmix, struct icd_data *results)
{
	if (!prefs.show_icd)
		return false;
	results->dN2 = get_he(oldgasmix) + get_o2(oldgasmix) - get_he(newgasmix) - get_o2(newgasmix);
	results->dHe = get_he(newgasmix) - get_he(oldgasmix);
	return get_he(oldgasmix) > 0 && results->dN2 > 0 && results->dHe < 0 && get_he(oldgasmix) && results->dN2 > 0 && 5 * results->dN2 > -results->dHe;
}

static bool gasmix_is_invalid(struct gasmix mix)
{
	return mix.o2.permille < 0;
}

int same_gasmix(struct gasmix a, struct gasmix b)
{
	if (gasmix_is_invalid(a) || gasmix_is_invalid(b))
		return 0;
	if (gasmix_is_air(a) && gasmix_is_air(b))
		return 1;
	return a.o2.permille == b.o2.permille && a.he.permille == b.he.permille;
}

void sanitize_gasmix(struct gasmix *mix)
{
	unsigned int o2, he;

	o2 = mix->o2.permille;
	he = mix->he.permille;

	/* Regular air: leave empty */
	if (!he) {
		if (!o2)
			return;
		/* 20.8% to 21% O2 is just air */
		if (gasmix_is_air(*mix)) {
			mix->o2.permille = 0;
			return;
		}
	}

	/* Sane mix? */
	if (o2 <= 1000 && he <= 1000 && o2 + he <= 1000)
		return;
	fprintf(stderr, "Odd gasmix: %u O2 %u He\n", o2, he);
	memset(mix, 0, sizeof(*mix));
}

int gasmix_distance(struct gasmix a, struct gasmix b)
{
	int a_o2 = get_o2(a), b_o2 = get_o2(b);
	int a_he = get_he(a), b_he = get_he(b);
	int delta_o2 = a_o2 - b_o2, delta_he = a_he - b_he;

	delta_he = delta_he * delta_he;
	delta_o2 = delta_o2 * delta_o2;
	return delta_he + delta_o2;
}

bool gasmix_is_air(struct gasmix gasmix)
{
	int o2 = gasmix.o2.permille;
	int he = gasmix.he.permille;
	return (he == 0) && (o2 == 0 || ((o2 >= O2_IN_AIR - 1) && (o2 <= O2_IN_AIR + 1)));
}
