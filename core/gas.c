// SPDX-License-Identifier: GPL-2.0
#include "gas.h"
#include "pref.h"
#include "gettext.h"
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
	if (!prefs.show_icd) {
		results->dN2 = results->dHe = 0;
		return false;
	}
	results->dN2 = get_n2(newgasmix) - get_n2(oldgasmix);
	results->dHe = get_he(newgasmix) - get_he(oldgasmix);
	return get_he(oldgasmix) > 0 && results->dN2 > 0 && results->dHe < 0 && get_he(oldgasmix) && results->dN2 > 0 && 5 * results->dN2 > -results->dHe;
}

bool gasmix_is_invalid(struct gasmix mix)
{
	return mix.o2.permille < 0;
}

int same_gasmix(struct gasmix a, struct gasmix b)
{
	if (gasmix_is_invalid(a) || gasmix_is_invalid(b))
		return 0;
	if (gasmix_is_air(a) && gasmix_is_air(b))
		return 1;
	return get_o2(a) == get_o2(b) && get_he(a) == get_he(b);
}

void sanitize_gasmix(struct gasmix *mix)
{
	unsigned int o2, he;

	o2 = get_o2(*mix);
	he = get_he(*mix);

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
	int o2 = get_o2(gasmix);
	int he = get_he(gasmix);
	return (he == 0) && (o2 == 0 || ((o2 >= O2_IN_AIR - 1) && (o2 <= O2_IN_AIR + 1)));
}

fraction_t make_fraction(int i)
{
	fraction_t res;
	res.permille = i;
	return res;
}

fraction_t get_gas_component_fraction(struct gasmix mix, enum gas_component component)
{
	switch (component) {
	case O2: return make_fraction(get_o2(mix));
	case N2: return make_fraction(get_n2(mix));
	case HE: return make_fraction(get_he(mix));
	default: return make_fraction(0);
	}
}

// O2 pressure in mbar according to the steady state model for the PSCR
// NB: Ambient pressure comes in bar!
int pscr_o2(const double amb_pressure, struct gasmix mix)
{
	int o2 = (int)(get_o2(mix) * amb_pressure -
		       (1.0 - get_o2(mix) / 1000.0) * prefs.o2consumption / (prefs.bottomsac * prefs.pscr_ratio) * 1000000);
	if (o2 < 0.0) // He's dead, Jim.
		o2 = 0.0;
	return o2;
}

/* fill_pressures(): Compute partial gas pressures in bar from gasmix and ambient pressures, possibly for OC or CCR, to be
 * extended to PSCT. This function does the calculations of gas pressures applicable to a single point on the dive profile.
 * The structure "pressures" is used to return calculated gas pressures to the calling software.
 * Call parameters:	po2 = po2 value applicable to the record in calling function
 *			amb_pressure = ambient pressure applicable to the record in calling function
 *			*pressures = structure for communicating o2 sensor values from and gas pressures to the calling function.
 *			*mix = structure containing cylinder gas mixture information.
 *			divemode = the dive mode pertaining to this point in the dive profile.
 * This function called by: calculate_gas_information_new() in profile.c; add_segment() in deco.c.
 */
void fill_pressures(struct gas_pressures *pressures, const double amb_pressure, struct gasmix mix, double po2, enum divemode_t divemode)
{
	if ((divemode != OC) && po2) {	// This is a rebreather dive where pressures->o2 is defined
		if (po2 >= amb_pressure) {
			pressures->o2 = amb_pressure;
			pressures->n2 = pressures->he = 0.0;
		} else {
			pressures->o2 = po2;
			if (get_o2(mix) == 1000) {
				pressures->he = pressures->n2 = 0;
			} else {
				pressures->he = (amb_pressure - pressures->o2) * (double)get_he(mix) / (1000 - get_o2(mix));
				pressures->n2 = amb_pressure - pressures->o2 - pressures->he;
			}
		}
	} else {
		if (divemode == PSCR) { /* The steady state approximation should be good enough */
			pressures->o2 = pscr_o2(amb_pressure, mix) / 1000.0;
			if (get_o2(mix) != 1000) {
				pressures->he = (amb_pressure - pressures->o2) * get_he(mix) / (1000.0 - get_o2(mix));
				pressures->n2 = (amb_pressure - pressures->o2) * get_n2(mix) / (1000.0 - get_o2(mix));
			} else {
				pressures->he = pressures->n2 = 0;
			}
		} else {
			// Open circuit dives: no gas pressure values available, they need to be calculated
			pressures->o2 = get_o2(mix) / 1000.0 * amb_pressure; // These calculations are also used if the CCR calculation above..
			pressures->he = get_he(mix) / 1000.0 * amb_pressure; // ..returned a po2 of zero (i.e. o2 sensor data not resolvable)
			pressures->n2 = get_n2(mix) / 1000.0 * amb_pressure;
		}
	}
}

enum gastype gasmix_to_type(struct gasmix mix)
{
	if (gasmix_is_air(mix))
		return GASTYPE_AIR;
	if (get_o2(mix) >= 980)
		return GASTYPE_OXYGEN;
	if (get_he(mix) == 0)
		return get_o2(mix) >= 230 ? GASTYPE_NITROX : GASTYPE_AIR;
	if (get_o2(mix) <= 180)
		return GASTYPE_HYPOXIC_TRIMIX;
	return get_o2(mix) <= 230 ? GASTYPE_NORMOXIC_TRIMIX : GASTYPE_HYPEROXIC_TRIMIX;
}

static const char *gastype_names[] = {
	QT_TRANSLATE_NOOP("gettextFromC", "Air"),
	QT_TRANSLATE_NOOP("gettextFromC", "Nitrox"),
	QT_TRANSLATE_NOOP("gettextFromC", "Hypoxic Trimix"),
	QT_TRANSLATE_NOOP("gettextFromC", "Normoxic Trimix"),
	QT_TRANSLATE_NOOP("gettextFromC", "Hyperoxic Trimix"),
	QT_TRANSLATE_NOOP("gettextFromC", "Oxygen")
};

const char *gastype_name(enum gastype type)
{
	if (type < 0 || type >= GASTYPE_COUNT)
		return "";
	return translate("gettextFromC", gastype_names[type]);
}
