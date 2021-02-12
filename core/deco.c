// SPDX-License-Identifier: GPL-2.0
/* calculate deco values
 * based on Bühlmann ZHL-16b
 * based on an implemention by heinrichs weikamp for the DR5
 * the original file was given to Subsurface under the GPLv2
 * by Matthias Heinrichs
 *
 * The implementation below is a fairly complete rewrite since then
 * (C) Robert C. Helling 2013 and released under the GPLv2
 *
 * add_segment()	- add <seconds> at the given pressure, breathing gasmix
 * deco_allowed_depth() - ceiling based on lead tissue, surface pressure, 3m increments or smooth
 * set_gf()		- set Buehlmann gradient factors
 * set_vpmb_conservatism() - set VPM-B conservatism value
 * clear_deco()
 * cache_deco_state()
 * restore_deco_state()
 * dump_tissues()
 */
#include <math.h>
#include <string.h>
#include <assert.h>

#include "deco.h"
#include "ssrf.h"
#include "dive.h"
#include "gas.h"
#include "subsurface-string.h"
#include "errorhelper.h"
#include "planner.h"
#include "qthelper.h"

#define cube(x) (x * x * x)

// Subsurface until v4.6.2 appeared to produce marginally less conservative plans than our benchmarks.
// This factor was used to correct this. Since a fix for the saturation and desaturation rates
// was introduced in v4.6.3 this can be set to a value of 1.0 which means no correction.
#define subsurface_conservatism_factor 1.0

//! Option structure for Buehlmann decompression.
struct buehlmann_config {
	double satmult;			//! safety at inert gas accumulation as percentage of effect (more than 100).
	double desatmult;		//! safety at inert gas depletion as percentage of effect (less than 100).
	int last_deco_stop_in_mtr;	//! depth of last_deco_stop.
	double gf_high;			//! gradient factor high (at surface).
	double gf_low;			//! gradient factor low (at bottom/start of deco calculation).
	double gf_low_position_min;	//! gf_low_position below surface_min_shallow.
};

struct buehlmann_config buehlmann_config = {
	.satmult = 1.0,
	.desatmult = 1.0,
	.last_deco_stop_in_mtr =  0,
	.gf_high = 0.75,
	.gf_low = 0.35,
	.gf_low_position_min = 1.0,
};

//! Option structure for VPM-B decompression.
struct vpmb_config {
	double crit_radius_N2;            //! Critical radius of N2 nucleon (microns).
	double crit_radius_He;            //! Critical radius of He nucleon (microns).
	double crit_volume_lambda;        //! Constant corresponding to critical gas volume (bar * min).
	double gradient_of_imperm;        //! Gradient after which bubbles become impermeable (bar).
	double surface_tension_gamma;     //! Nucleons surface tension constant (N / bar = m2).
	double skin_compression_gammaC;   //! Skin compression gammaC (N / bar = m2).
	double regeneration_time;         //! Time needed for the bubble to regenerate to the start radius (min).
	double other_gases_pressure;      //! Always present pressure of other gasses in tissues (bar).
	short conservatism;		  //! VPM-B conservatism level (0-4)
};

static struct vpmb_config vpmb_config = {
	.crit_radius_N2 = 0.55,
	.crit_radius_He = 0.45,
	.crit_volume_lambda = 199.58,
	.gradient_of_imperm = 8.30865,		// = 8.2 atm
	.surface_tension_gamma = 0.18137175,	// = 0.0179 N/msw
	.skin_compression_gammaC = 2.6040525,	// = 0.257 N/msw
	.regeneration_time = 20160.0,
	.other_gases_pressure = 0.1359888,
	.conservatism = 3
};

static const double buehlmann_N2_a[] = { 1.1696, 1.0, 0.8618, 0.7562,
					 0.62, 0.5043, 0.441, 0.4,
					 0.375, 0.35, 0.3295, 0.3065,
					 0.2835, 0.261, 0.248, 0.2327 };

static const double buehlmann_N2_b[] = { 0.5578, 0.6514, 0.7222, 0.7825,
					 0.8126, 0.8434, 0.8693, 0.8910,
					 0.9092, 0.9222, 0.9319, 0.9403,
					 0.9477, 0.9544, 0.9602, 0.9653 };

const double buehlmann_N2_t_halflife[] = { 5.0, 8.0, 12.5, 18.5,
					   27.0, 38.3, 54.3, 77.0,
					   109.0, 146.0, 187.0, 239.0,
					   305.0, 390.0, 498.0, 635.0 };

// 1 - exp(-1 / (halflife * 60) * ln(2))
static const double buehlmann_N2_factor_expositon_one_second[] = {
	2.30782347297664E-003, 1.44301447809736E-003, 9.23769302935806E-004, 6.24261986779007E-004,
	4.27777107246730E-004, 3.01585140931371E-004, 2.12729727268379E-004, 1.50020603047807E-004,
	1.05980191127841E-004, 7.91232600646508E-005, 6.17759153688224E-005, 4.83354552742732E-005,
	3.78761777920511E-005, 2.96212356654113E-005, 2.31974277413727E-005, 1.81926738960225E-005
};

static const double buehlmann_He_a[] = { 1.6189, 1.383, 1.1919, 1.0458,
					 0.922, 0.8205, 0.7305, 0.6502,
					 0.595, 0.5545, 0.5333, 0.5189,
					 0.5181, 0.5176, 0.5172, 0.5119 };

static const double buehlmann_He_b[] = { 0.4770, 0.5747, 0.6527, 0.7223,
					 0.7582, 0.7957, 0.8279, 0.8553,
					 0.8757, 0.8903, 0.8997, 0.9073,
					 0.9122, 0.9171, 0.9217, 0.9267 };

static const double buehlmann_He_t_halflife[] = { 1.88, 3.02, 4.72, 6.99,
						  10.21, 14.48, 20.53, 29.11,
						  41.20, 55.19, 70.69, 90.34,
						  115.29, 147.42, 188.24, 240.03 };

static const double buehlmann_He_factor_expositon_one_second[] = {
	6.12608039419837E-003, 3.81800836683133E-003, 2.44456078654209E-003, 1.65134647076792E-003,
	1.13084424730725E-003, 7.97503165599123E-004, 5.62552521860549E-004, 3.96776399429366E-004,
	2.80360036664540E-004, 2.09299583354805E-004, 1.63410794820518E-004, 1.27869320250551E-004,
	1.00198406028040E-004, 7.83611475491108E-005, 6.13689891868496E-005, 4.81280465299827E-005
};

static const double vpmb_conservatism_lvls[] = { 1.0, 1.05, 1.12, 1.22, 1.35 };

/* Inspired gas loading equations depend on the partial pressure of inert gas in the alveolar.
 * P_alv = (P_amb - P_H2O + (1 - Rq) / Rq * P_CO2) * f
 * where:
 * P_alv	alveolar partial pressure of inert gas
 * P_amb	ambient pressure
 * P_H2O	water vapour partial pressure = ~0.0627 bar
 * P_CO2	carbon dioxide partial pressure = ~0.0534 bar
 * Rq	respiratory quotient (O2 consumption / CO2 production)
 * f	fraction of inert gas
 *
 * In our calculations, we simplify this to use an effective water vapour pressure
 * WV = P_H20 - (1 - Rq) / Rq * P_CO2
 *
 * Buhlmann ignored the contribution of CO2 (i.e. Rq = 1.0), whereas Schreiner adopted Rq = 0.8.
 * WV_Buhlmann = PP_H2O = 0.0627 bar
 * WV_Schreiner = 0.0627 - (1 - 0.8) / Rq * 0.0534 = 0.0493 bar

 * Buhlmann calculations use the Buhlmann value, VPM-B calculations use the Schreiner value.
*/
#define WV_PRESSURE 0.0627 		// water vapor pressure in bar, based on respiratory quotient Rq = 1.0 (Buhlmann value)
#define WV_PRESSURE_SCHREINER 0.0493	// water vapor pressure in bar, based on respiratory quotient Rq = 0.8 (Schreiner value)

#define DECO_STOPS_MULTIPLIER_MM 3000.0
#define NITROGEN_FRACTION 0.79

#define TISSUE_ARRAY_SZ sizeof(ds->tissue_n2_sat)

static double get_crit_radius_He()
{
	if (vpmb_config.conservatism <= 4)
		return vpmb_config.crit_radius_He * vpmb_conservatism_lvls[vpmb_config.conservatism] * subsurface_conservatism_factor;
	return vpmb_config.crit_radius_He;
}

static double get_crit_radius_N2()
{
	if (vpmb_config.conservatism <= 4)
		return vpmb_config.crit_radius_N2 * vpmb_conservatism_lvls[vpmb_config.conservatism] * subsurface_conservatism_factor;
	return vpmb_config.crit_radius_N2;
}

// Solve another cubic equation, this time
// x^3 - B x - C == 0
// Use trigonometric formula for negative discriminants (see Wikipedia for details)

static double solve_cubic2(double B, double C)
{
	double discriminant = 27 * C * C - 4 * cube(B);
	if (discriminant < 0.0) {
		return 2.0 * sqrt(B / 3.0) * cos(acos(3.0 * C * sqrt(3.0 / B) / (2.0 * B)) / 3.0);
	}

	double denominator = pow(9 * C + sqrt(3 * discriminant), 1 / 3.0);

	return pow(2.0 / 3.0, 1.0 / 3.0) * B / denominator + denominator / pow(18.0, 1.0 / 3.0);
}

// This is a simplified formula avoiding radii. It uses the fact that Boyle's law says
// pV = (G + P_amb) / G^3 is constant to solve for the new gradient G.

static double update_gradient(struct deco_state *ds, double next_stop_pressure, double first_gradient)
{
	double B = cube(first_gradient) / (ds->first_ceiling_pressure.mbar / 1000.0 + first_gradient);
	double C = next_stop_pressure * B;

	double new_gradient = solve_cubic2(B, C);

	if (new_gradient < 0.0)
		report_error("Negative gradient encountered!");
	return new_gradient;
}

static double vpmb_tolerated_ambient_pressure(struct deco_state *ds, double reference_pressure, int ci)
{
	double n2_gradient, he_gradient, total_gradient;

	if (reference_pressure >= ds->first_ceiling_pressure.mbar / 1000.0 || !ds->first_ceiling_pressure.mbar) {
		n2_gradient = ds->bottom_n2_gradient[ci];
		he_gradient = ds->bottom_he_gradient[ci];
	} else {
		n2_gradient = update_gradient(ds, reference_pressure, ds->bottom_n2_gradient[ci]);
		he_gradient = update_gradient(ds, reference_pressure, ds->bottom_he_gradient[ci]);
	}

	total_gradient = ((n2_gradient * ds->tissue_n2_sat[ci]) + (he_gradient * ds->tissue_he_sat[ci])) / (ds->tissue_n2_sat[ci] + ds->tissue_he_sat[ci]);

	return ds->tissue_n2_sat[ci] + ds->tissue_he_sat[ci] + vpmb_config.other_gases_pressure - total_gradient;
}

double tissue_tolerance_calc(struct deco_state *ds, const struct dive *dive, double pressure, bool in_planner)
{
	int ci = -1;
	double ret_tolerance_limit_ambient_pressure = 0.0;
	double gf_high = buehlmann_config.gf_high;
	double gf_low = buehlmann_config.gf_low;
	double surface = get_surface_pressure_in_mbar(dive, true) / 1000.0;
	double lowest_ceiling = 0.0;
	double tissue_lowest_ceiling[16];

	for (ci = 0; ci < 16; ci++) {
		ds->buehlmann_inertgas_a[ci] = ((buehlmann_N2_a[ci] * ds->tissue_n2_sat[ci]) + (buehlmann_He_a[ci] * ds->tissue_he_sat[ci])) / ds->tissue_inertgas_saturation[ci];
		ds->buehlmann_inertgas_b[ci] = ((buehlmann_N2_b[ci] * ds->tissue_n2_sat[ci]) + (buehlmann_He_b[ci] * ds->tissue_he_sat[ci])) / ds->tissue_inertgas_saturation[ci];
	}

	if (decoMode(in_planner) != VPMB) {
		for (ci = 0; ci < 16; ci++) {

			/* tolerated = (tissue_inertgas_saturation - buehlmann_inertgas_a) * buehlmann_inertgas_b; */

			tissue_lowest_ceiling[ci] = (ds->buehlmann_inertgas_b[ci] * ds->tissue_inertgas_saturation[ci] - gf_low * ds->buehlmann_inertgas_a[ci] * ds->buehlmann_inertgas_b[ci]) /
						     ((1.0 - ds->buehlmann_inertgas_b[ci]) * gf_low + ds->buehlmann_inertgas_b[ci]);
			if (tissue_lowest_ceiling[ci] > lowest_ceiling)
				lowest_ceiling = tissue_lowest_ceiling[ci];
			if (lowest_ceiling > ds->gf_low_pressure_this_dive)
				ds->gf_low_pressure_this_dive = lowest_ceiling;
		}
		for (ci = 0; ci < 16; ci++) {
			double tolerated;

			if ((surface / ds->buehlmann_inertgas_b[ci] + ds->buehlmann_inertgas_a[ci] - surface) * gf_high + surface <
			    (ds->gf_low_pressure_this_dive / ds->buehlmann_inertgas_b[ci] + ds->buehlmann_inertgas_a[ci] - ds->gf_low_pressure_this_dive) * gf_low + ds->gf_low_pressure_this_dive)
				tolerated = (-ds->buehlmann_inertgas_a[ci] * ds->buehlmann_inertgas_b[ci] * (gf_high * ds->gf_low_pressure_this_dive - gf_low * surface) -
					     (1.0 - ds->buehlmann_inertgas_b[ci]) * (gf_high - gf_low) * ds->gf_low_pressure_this_dive * surface +
					     ds->buehlmann_inertgas_b[ci] * (ds->gf_low_pressure_this_dive - surface) * ds->tissue_inertgas_saturation[ci]) /
					    (-ds->buehlmann_inertgas_a[ci] * ds->buehlmann_inertgas_b[ci] * (gf_high - gf_low) +
					     (1.0 - ds->buehlmann_inertgas_b[ci]) * (gf_low * ds->gf_low_pressure_this_dive - gf_high * surface) +
					     ds->buehlmann_inertgas_b[ci] * (ds->gf_low_pressure_this_dive - surface));
			else
				tolerated = ret_tolerance_limit_ambient_pressure;


			ds->tolerated_by_tissue[ci] = tolerated;

			if (tolerated >= ret_tolerance_limit_ambient_pressure) {
				ds->ci_pointing_to_guiding_tissue = ci;
				ret_tolerance_limit_ambient_pressure = tolerated;
			}
		}
	} else {
		// VPM-B ceiling
		double reference_pressure;

		ret_tolerance_limit_ambient_pressure = pressure;
		// The Boyle compensated gradient depends on ambient pressure. For the ceiling, this should set the ambient pressure.
		do {
			reference_pressure = ret_tolerance_limit_ambient_pressure;
			ret_tolerance_limit_ambient_pressure = 0.0;
			for (ci = 0; ci < 16; ci++) {
				double tolerated = vpmb_tolerated_ambient_pressure(ds, reference_pressure, ci);
				if (tolerated >= ret_tolerance_limit_ambient_pressure) {
					ds->ci_pointing_to_guiding_tissue = ci;
					ret_tolerance_limit_ambient_pressure = tolerated;
				}
				ds->tolerated_by_tissue[ci] = tolerated;
			}
		// We are doing ok if the gradient was computed within ten centimeters of the ceiling.
		} while (fabs(ret_tolerance_limit_ambient_pressure - reference_pressure) > 0.01);
	}
	return ret_tolerance_limit_ambient_pressure;
}

/*
 * Return Buehlmann factor for a particular period and tissue index.
 */
static double factor(int period_in_seconds, int ci, enum gas_component gas)
{
	if (period_in_seconds == 1) {
		if (gas == N2)
			return buehlmann_N2_factor_expositon_one_second[ci];
		else
			return buehlmann_He_factor_expositon_one_second[ci];
	}

	// ln(2)/60 = 1.155245301e-02
	if (gas == N2)
		return 1.0 - exp(-period_in_seconds * 1.155245301e-02 / buehlmann_N2_t_halflife[ci]);
	else
		return 1.0 - exp(-period_in_seconds * 1.155245301e-02 / buehlmann_He_t_halflife[ci]);
}

static double calc_surface_phase(double surface_pressure, double he_pressure, double n2_pressure, double he_time_constant, double n2_time_constant, bool in_planner)
{
	double inspired_n2 = (surface_pressure - ((in_planner && (decoMode(true) == VPMB)) ? WV_PRESSURE_SCHREINER : WV_PRESSURE)) * NITROGEN_FRACTION;

	if (n2_pressure > inspired_n2)
		return (he_pressure / he_time_constant + (n2_pressure - inspired_n2) / n2_time_constant) / (he_pressure + n2_pressure - inspired_n2);

	if (he_pressure + n2_pressure >= inspired_n2){
		double gradient_decay_time = 1.0 / (n2_time_constant - he_time_constant) * log ((inspired_n2 - n2_pressure) / he_pressure);
		double gradients_integral = he_pressure / he_time_constant * (1.0 - exp(-he_time_constant * gradient_decay_time)) + (n2_pressure - inspired_n2) / n2_time_constant * (1.0 - exp(-n2_time_constant * gradient_decay_time));
		return gradients_integral / (he_pressure + n2_pressure - inspired_n2);
	}

	return 0;
}

void vpmb_start_gradient(struct deco_state *ds)
{
	int ci;

	for (ci = 0; ci < 16; ++ci) {
		ds->initial_n2_gradient[ci] = ds->bottom_n2_gradient[ci] = 2.0 * (vpmb_config.surface_tension_gamma / vpmb_config.skin_compression_gammaC) * ((vpmb_config.skin_compression_gammaC - vpmb_config.surface_tension_gamma) / ds->n2_regen_radius[ci]);
		ds->initial_he_gradient[ci] = ds->bottom_he_gradient[ci] = 2.0 * (vpmb_config.surface_tension_gamma / vpmb_config.skin_compression_gammaC) * ((vpmb_config.skin_compression_gammaC - vpmb_config.surface_tension_gamma) / ds->he_regen_radius[ci]);
	}
}

void vpmb_next_gradient(struct deco_state *ds, double deco_time, double surface_pressure, bool in_planner)
{
	int ci;
	double n2_b, n2_c;
	double he_b, he_c;
	double desat_time;
	deco_time /= 60.0;

	for (ci = 0; ci < 16; ++ci) {
		desat_time = deco_time + calc_surface_phase(surface_pressure, ds->tissue_he_sat[ci], ds->tissue_n2_sat[ci], log(2.0) / buehlmann_He_t_halflife[ci], log(2.0) / buehlmann_N2_t_halflife[ci], in_planner);

		n2_b = ds->initial_n2_gradient[ci] + (vpmb_config.crit_volume_lambda * vpmb_config.surface_tension_gamma) / (vpmb_config.skin_compression_gammaC * desat_time);
		he_b = ds->initial_he_gradient[ci] + (vpmb_config.crit_volume_lambda * vpmb_config.surface_tension_gamma) / (vpmb_config.skin_compression_gammaC * desat_time);

		n2_c = vpmb_config.surface_tension_gamma * vpmb_config.surface_tension_gamma * vpmb_config.crit_volume_lambda * ds->max_n2_crushing_pressure[ci];
		n2_c = n2_c / (vpmb_config.skin_compression_gammaC * vpmb_config.skin_compression_gammaC * desat_time);
		he_c = vpmb_config.surface_tension_gamma * vpmb_config.surface_tension_gamma * vpmb_config.crit_volume_lambda * ds->max_he_crushing_pressure[ci];
		he_c = he_c / (vpmb_config.skin_compression_gammaC * vpmb_config.skin_compression_gammaC * desat_time);

		ds->bottom_n2_gradient[ci] = 0.5 * ( n2_b + sqrt(n2_b * n2_b - 4.0 * n2_c));
		ds->bottom_he_gradient[ci] = 0.5 * ( he_b + sqrt(he_b * he_b - 4.0 * he_c));
	}
}

// A*r^3 - B*r^2 - C == 0
// Solved with the help of mathematica

static double solve_cubic(double A, double B, double C)
{
	double BA = B/A;
	double CA = C/A;

	double discriminant = CA * (4 * cube(BA) + 27 * CA);

	// Let's make sure we have a real solution:
	if (discriminant < 0.0) {
		// This should better not happen
		report_error("Complex solution for inner pressure encountered!\n A=%f\tB=%f\tC=%f\n", A, B, C);
		return 0.0;
	}
	double denominator = pow(cube(BA) + 1.5 * (9 * CA + sqrt(3.0) * sqrt(discriminant)), 1/3.0);
	return (BA + BA * BA / denominator + denominator) / 3.0;

}


void nuclear_regeneration(struct deco_state *ds, double time)
{
	time /= 60.0;
	int ci;
	double crushing_radius_N2, crushing_radius_He;
	for (ci = 0; ci < 16; ++ci) {
		//rm
		crushing_radius_N2 = 1.0 / (ds->max_n2_crushing_pressure[ci] / (2.0 * (vpmb_config.skin_compression_gammaC - vpmb_config.surface_tension_gamma)) + 1.0 / get_crit_radius_N2());
		crushing_radius_He = 1.0 / (ds->max_he_crushing_pressure[ci] / (2.0 * (vpmb_config.skin_compression_gammaC - vpmb_config.surface_tension_gamma)) + 1.0 / get_crit_radius_He());
		//rs
		ds->n2_regen_radius[ci] = crushing_radius_N2 + (get_crit_radius_N2() - crushing_radius_N2) * (1.0 - exp (-time / vpmb_config.regeneration_time));
		ds->he_regen_radius[ci] = crushing_radius_He + (get_crit_radius_He() - crushing_radius_He) * (1.0 - exp (-time / vpmb_config.regeneration_time));
	}
}


// Calculates the nucleons inner pressure during the impermeable period
static double calc_inner_pressure(double crit_radius, double onset_tension, double current_ambient_pressure)
{
	double onset_radius = 1.0 / (vpmb_config.gradient_of_imperm / (2.0 * (vpmb_config.skin_compression_gammaC - vpmb_config.surface_tension_gamma)) + 1.0 / crit_radius);


	double A = current_ambient_pressure - vpmb_config.gradient_of_imperm + (2.0 * (vpmb_config.skin_compression_gammaC - vpmb_config.surface_tension_gamma)) / onset_radius;
	double B = 2.0 * (vpmb_config.skin_compression_gammaC - vpmb_config.surface_tension_gamma);
	double C = onset_tension * pow(onset_radius, 3);

	double current_radius = solve_cubic(A, B, C);

	return onset_tension * onset_radius * onset_radius * onset_radius / (current_radius * current_radius * current_radius);
}

// Calculates the crushing pressure in the given moment. Updates crushing_onset_tension and critical radius if needed
void calc_crushing_pressure(struct deco_state *ds, double pressure)
{
	int ci;
	double gradient;
	double gas_tension;
	double n2_crushing_pressure, he_crushing_pressure;
	double n2_inner_pressure, he_inner_pressure;

	for (ci = 0; ci < 16; ++ci) {
		gas_tension = ds->tissue_n2_sat[ci] + ds->tissue_he_sat[ci] + vpmb_config.other_gases_pressure;
		gradient = pressure - gas_tension;

		if (gradient <= vpmb_config.gradient_of_imperm) {	// permeable situation
			n2_crushing_pressure = he_crushing_pressure = gradient;
			ds->crushing_onset_tension[ci] = gas_tension;
		} else {	// impermeable
			if (ds->max_ambient_pressure >= pressure)
				return;

			n2_inner_pressure = calc_inner_pressure(get_crit_radius_N2(), ds->crushing_onset_tension[ci], pressure);
			he_inner_pressure = calc_inner_pressure(get_crit_radius_He(), ds->crushing_onset_tension[ci], pressure);

			n2_crushing_pressure = pressure - n2_inner_pressure;
			he_crushing_pressure = pressure - he_inner_pressure;
		}
		ds->max_n2_crushing_pressure[ci] = MAX(ds->max_n2_crushing_pressure[ci], n2_crushing_pressure);
		ds->max_he_crushing_pressure[ci] = MAX(ds->max_he_crushing_pressure[ci], he_crushing_pressure);
	}
	ds->max_ambient_pressure = MAX(pressure, ds->max_ambient_pressure);
}

/* add period_in_seconds at the given pressure and gas to the deco calculation */
void add_segment(struct deco_state *ds, double pressure, struct gasmix gasmix, int period_in_seconds, int ccpo2, enum divemode_t divemode, int sac, bool in_planner)
{
	UNUSED(sac);
	int ci;
	struct gas_pressures pressures;
	bool icd = false;
	fill_pressures(&pressures, pressure - ((in_planner && (decoMode(true) == VPMB)) ? WV_PRESSURE_SCHREINER : WV_PRESSURE),
		       gasmix, (double) ccpo2 / 1000.0, divemode);

	for (ci = 0; ci < 16; ci++) {
		double pn2_oversat = pressures.n2 - ds->tissue_n2_sat[ci];
		double phe_oversat = pressures.he - ds->tissue_he_sat[ci];
		double n2_f = factor(period_in_seconds, ci, N2);
		double he_f = factor(period_in_seconds, ci, HE);
		double n2_satmult = pn2_oversat > 0 ? buehlmann_config.satmult : buehlmann_config.desatmult;
		double he_satmult = phe_oversat > 0 ? buehlmann_config.satmult : buehlmann_config.desatmult;

		// Report ICD if N2 is more on-gasing than He off-gasing in leading tissue
		if (ci == ds->ci_pointing_to_guiding_tissue && pn2_oversat > 0.0 && phe_oversat < 0.0 &&
		    pn2_oversat * n2_satmult * n2_f + phe_oversat * he_satmult * he_f > 0)
			icd = true;

		ds->tissue_n2_sat[ci] += n2_satmult * pn2_oversat * n2_f;
		ds->tissue_he_sat[ci] += he_satmult * phe_oversat * he_f;
		ds->tissue_inertgas_saturation[ci] = ds->tissue_n2_sat[ci] + ds->tissue_he_sat[ci];

	}
	if (decoMode(in_planner) == VPMB)
		calc_crushing_pressure(ds, pressure);
	ds->icd_warning = icd;
	return;
}

#if DECO_CALC_DEBUG
void dump_tissues(struct deco_state *ds)
{
	int ci;
	printf("N2 tissues:");
	for (ci = 0; ci < 16; ci++)
		printf(" %6.3e", ds->tissue_n2_sat[ci]);
	printf("\nHe tissues:");
	for (ci = 0; ci < 16; ci++)
		printf(" %6.3e", ds->tissue_he_sat[ci]);
	printf("\n");
}
#endif

void clear_vpmb_state(struct deco_state *ds)
{
	int ci;
	for (ci = 0; ci < 16; ci++) {
		ds->max_n2_crushing_pressure[ci] = 0.0;
		ds->max_he_crushing_pressure[ci] = 0.0;
	}
	ds->max_ambient_pressure = 0;
	ds->first_ceiling_pressure.mbar = 0;
	ds->max_bottom_ceiling_pressure.mbar = 0;
}

void clear_deco(struct deco_state *ds, double surface_pressure, bool in_planner)
{
	int ci;

	memset(ds, 0, sizeof(*ds));
	clear_vpmb_state(ds);
	for (ci = 0; ci < 16; ci++) {
		ds->tissue_n2_sat[ci] = (surface_pressure - ((in_planner && (decoMode(true) == VPMB)) ? WV_PRESSURE_SCHREINER : WV_PRESSURE)) * N2_IN_AIR / 1000;
		ds->tissue_he_sat[ci] = 0.0;
		ds->max_n2_crushing_pressure[ci] = 0.0;
		ds->max_he_crushing_pressure[ci] = 0.0;
		ds->n2_regen_radius[ci] = get_crit_radius_N2();
		ds->he_regen_radius[ci] = get_crit_radius_He();
	}
	ds->gf_low_pressure_this_dive = surface_pressure + buehlmann_config.gf_low_position_min;
	ds->max_ambient_pressure = 0.0;
	ds->ci_pointing_to_guiding_tissue = -1;
}

void cache_deco_state(struct deco_state *src, struct deco_state **cached_datap)
{
	struct deco_state *data = *cached_datap;

	if (!data) {
		data = malloc(sizeof(struct deco_state));
		*cached_datap = data;
	}
	*data = *src;
}

void restore_deco_state(struct deco_state *data, struct deco_state *target, bool keep_vpmb_state)
{
	if (keep_vpmb_state) {
		int ci;
		for (ci = 0; ci < 16; ci++) {
			data->bottom_n2_gradient[ci] = target->bottom_n2_gradient[ci];
			data->bottom_he_gradient[ci] = target->bottom_he_gradient[ci];
			data->initial_n2_gradient[ci] = target->initial_n2_gradient[ci];
			data->initial_he_gradient[ci] = target->initial_he_gradient[ci];
		}
		data->first_ceiling_pressure = target->first_ceiling_pressure;
		data->max_bottom_ceiling_pressure = target->max_bottom_ceiling_pressure;
	}
	*target = *data;

}

int deco_allowed_depth(double tissues_tolerance, double surface_pressure, const struct dive *dive, bool smooth)
{
	int depth;
	double pressure_delta;

	/* Avoid negative depths */
	pressure_delta = tissues_tolerance > surface_pressure ? tissues_tolerance - surface_pressure : 0.0;

	depth = rel_mbar_to_depth(lrint(pressure_delta * 1000), dive);

	if (!smooth)
		depth = lrint(ceil(depth / DECO_STOPS_MULTIPLIER_MM) * DECO_STOPS_MULTIPLIER_MM);

	if (depth > 0 && depth < buehlmann_config.last_deco_stop_in_mtr * 1000)
		depth = buehlmann_config.last_deco_stop_in_mtr * 1000;

	return depth;
}

void set_gf(short gflow, short gfhigh)
{
	if (gflow != -1)
		buehlmann_config.gf_low = (double)gflow / 100.0;
	if (gfhigh != -1)
		buehlmann_config.gf_high = (double)gfhigh / 100.0;
}

void set_vpmb_conservatism(short conservatism)
{
	if (conservatism < 0)
		vpmb_config.conservatism = 0;
	else if (conservatism > 4)
		vpmb_config.conservatism = 4;
	else
		vpmb_config.conservatism = conservatism;
}

double get_gf(struct deco_state *ds, double ambpressure_bar, const struct dive *dive)
{
	double surface_pressure_bar = get_surface_pressure_in_mbar(dive, true) / 1000.0;
	double gf_low = buehlmann_config.gf_low;
	double gf_high = buehlmann_config.gf_high;
	double gf;
	if (ds->gf_low_pressure_this_dive > surface_pressure_bar)
		gf = MAX((double)gf_low, (ambpressure_bar - surface_pressure_bar) /
			(ds->gf_low_pressure_this_dive - surface_pressure_bar) * (gf_low - gf_high) + gf_high);
	else
		gf = gf_low;
	return gf;
}

double regressiona(const struct deco_state *ds)
{
	if (ds->sum1 > 1) {
		double avxy = ds->sumxy / ds->sum1;
		double avx = (double)ds->sumx / ds->sum1;
		double avy = ds->sumy / ds->sum1;
		double avxx = (double) ds->sumxx / ds->sum1;
		return (avxy - avx * avy) / (avxx - avx*avx);
	}
	else
		return 0.0;
}

double regressionb(const struct deco_state *ds)
{
	if (ds->sum1)
		return ds->sumy / ds->sum1 - ds->sumx * regressiona(ds) / ds->sum1;
	else
		return 0.0;
}

void reset_regression(struct deco_state *ds)
{
	ds->sum1 = 0;
	ds->sumxx = ds->sumx = 0L;
	ds->sumy = ds->sumxy = 0.0;
}

void update_regression(struct deco_state *ds, const struct dive *dive)
{
	if (!ds->plot_depth)
		return;
	ds->sum1 += 1;
	ds->sumx += ds->plot_depth;
	ds->sumxx += (long)ds->plot_depth * ds->plot_depth;
	double n2_gradient, he_gradient, total_gradient;
	n2_gradient = update_gradient(ds, depth_to_bar(ds->plot_depth, dive), ds->bottom_n2_gradient[ds->ci_pointing_to_guiding_tissue]);
	he_gradient = update_gradient(ds, depth_to_bar(ds->plot_depth, dive), ds->bottom_he_gradient[ds->ci_pointing_to_guiding_tissue]);
	total_gradient = ((n2_gradient * ds->tissue_n2_sat[ds->ci_pointing_to_guiding_tissue]) + (he_gradient * ds->tissue_he_sat[ds->ci_pointing_to_guiding_tissue]))
			/ (ds->tissue_n2_sat[ds->ci_pointing_to_guiding_tissue] + ds->tissue_he_sat[ds->ci_pointing_to_guiding_tissue]);

	double buehlmann_gradient = (1.0 / ds->buehlmann_inertgas_b[ds->ci_pointing_to_guiding_tissue] - 1.0) * depth_to_bar(ds->plot_depth, dive) + ds->buehlmann_inertgas_a[ds->ci_pointing_to_guiding_tissue];
	double gf = (total_gradient - vpmb_config.other_gases_pressure) / buehlmann_gradient;
	ds->sumxy += gf * ds->plot_depth;
	ds->sumy += gf;
	ds->plot_depth = 0;
}
