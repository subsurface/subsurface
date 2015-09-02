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
 * clear_deco()
 * cache_deco_state()
 * restore_deco_state()
 * dump_tissues()
 */
#include <math.h>
#include <string.h>
#include "dive.h"
#include <assert.h>
#include <planner.h>

#define cube(x) (x * x * x)

// Subsurface appears to produce marginally less conservative plans than our benchmarks
// Introduce 1.2% additional conservatism
#define subsurface_conservatism_factor 1.012


extern bool in_planner();

extern pressure_t first_ceiling_pressure;

//! Option structure for Buehlmann decompression.
struct buehlmann_config {
	double satmult;			    //! safety at inert gas accumulation as percentage of effect (more than 100).
	double desatmult;		    //! safety at inert gas depletion as percentage of effect (less than 100).
	unsigned int last_deco_stop_in_mtr; //! depth of last_deco_stop.
	double gf_high;			    //! gradient factor high (at surface).
	double gf_low;			    //! gradient factor low (at bottom/start of deco calculation).
	double gf_low_position_min;	 //! gf_low_position below surface_min_shallow.
	bool gf_low_at_maxdepth;	    //! if true, gf_low applies at max depth instead of at deepest ceiling.
};

struct buehlmann_config buehlmann_config = {
	.satmult = 1.0,
	.desatmult = 1.01,
	.last_deco_stop_in_mtr =  0,
	.gf_high = 0.75,
	.gf_low = 0.35,
	.gf_low_position_min = 1.0,
	.gf_low_at_maxdepth = false
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
};

struct vpmb_config vpmb_config = {
	.crit_radius_N2 = 0.55,
	.crit_radius_He = 0.45,
	.crit_volume_lambda = 199.58,
	.gradient_of_imperm = 8.30865,		// = 8.2 atm
	.surface_tension_gamma = 0.18137175,	// = 0.0179 N/msw
	.skin_compression_gammaC = 2.6040525,	// = 0.257 N/msw
	.regeneration_time = 20160.0,
	.other_gases_pressure = 0.1359888
};

const double buehlmann_N2_a[] = { 1.1696, 1.0, 0.8618, 0.7562,
				  0.62, 0.5043, 0.441, 0.4,
				  0.375, 0.35, 0.3295, 0.3065,
				  0.2835, 0.261, 0.248, 0.2327 };

const double buehlmann_N2_b[] = { 0.5578, 0.6514, 0.7222, 0.7825,
				  0.8126, 0.8434, 0.8693, 0.8910,
				  0.9092, 0.9222, 0.9319, 0.9403,
				  0.9477, 0.9544, 0.9602, 0.9653 };

const double buehlmann_N2_t_halflife[] = { 5.0, 8.0, 12.5, 18.5,
					   27.0, 38.3, 54.3, 77.0,
					   109.0, 146.0, 187.0, 239.0,
					   305.0, 390.0, 498.0, 635.0 };

const double buehlmann_N2_factor_expositon_one_second[] = {
	2.30782347297664E-003, 1.44301447809736E-003, 9.23769302935806E-004, 6.24261986779007E-004,
	4.27777107246730E-004, 3.01585140931371E-004, 2.12729727268379E-004, 1.50020603047807E-004,
	1.05980191127841E-004, 7.91232600646508E-005, 6.17759153688224E-005, 4.83354552742732E-005,
	3.78761777920511E-005, 2.96212356654113E-005, 2.31974277413727E-005, 1.81926738960225E-005
};

const double buehlmann_He_a[] = { 1.6189, 1.383, 1.1919, 1.0458,
				  0.922, 0.8205, 0.7305, 0.6502,
				  0.595, 0.5545, 0.5333, 0.5189,
				  0.5181, 0.5176, 0.5172, 0.5119 };

const double buehlmann_He_b[] = { 0.4770, 0.5747, 0.6527, 0.7223,
				  0.7582, 0.7957, 0.8279, 0.8553,
				  0.8757, 0.8903, 0.8997, 0.9073,
				  0.9122, 0.9171, 0.9217, 0.9267 };

const double buehlmann_He_t_halflife[] = { 1.88, 3.02, 4.72, 6.99,
					   10.21, 14.48, 20.53, 29.11,
					   41.20, 55.19, 70.69, 90.34,
					   115.29, 147.42, 188.24, 240.03 };

const double buehlmann_He_factor_expositon_one_second[] = {
	6.12608039419837E-003, 3.81800836683133E-003, 2.44456078654209E-003, 1.65134647076792E-003,
	1.13084424730725E-003, 7.97503165599123E-004, 5.62552521860549E-004, 3.96776399429366E-004,
	2.80360036664540E-004, 2.09299583354805E-004, 1.63410794820518E-004, 1.27869320250551E-004,
	1.00198406028040E-004, 7.83611475491108E-005, 6.13689891868496E-005, 4.81280465299827E-005
};

const double conservatism_lvls[] = { 1.0, 1.05, 1.12, 1.22, 1.35 };

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

double tissue_n2_sat[16];
double tissue_he_sat[16];
int ci_pointing_to_guiding_tissue;
double gf_low_pressure_this_dive;
#define TISSUE_ARRAY_SZ sizeof(tissue_n2_sat)

double tolerated_by_tissue[16];
double tissue_inertgas_saturation[16];
double buehlmann_inertgas_a[16], buehlmann_inertgas_b[16];

double max_n2_crushing_pressure[16];
double max_he_crushing_pressure[16];

double crushing_onset_tension[16];            // total inert gas tension in the t* moment
double n2_regen_radius[16];                   // rs
double he_regen_radius[16];
double max_ambient_pressure;                  // last moment we were descending

double bottom_n2_gradient[16];
double bottom_he_gradient[16];

double initial_n2_gradient[16];
double initial_he_gradient[16];

double get_crit_radius_He()
{
	if (prefs.conservatism_level <= 4)
		return vpmb_config.crit_radius_He * conservatism_lvls[prefs.conservatism_level] * subsurface_conservatism_factor;
	return vpmb_config.crit_radius_He;
}

double get_crit_radius_N2()
{
	if (prefs.conservatism_level <= 4)
		return vpmb_config.crit_radius_N2 * conservatism_lvls[prefs.conservatism_level] * subsurface_conservatism_factor;
	return vpmb_config.crit_radius_N2;
}

// Solve another cubic equation, this time
// x^3 - B x - C == 0
// Use trigonometric formula for negative discriminants (see Wikipedia for details)

double solve_cubic2(double B, double C)
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

double update_gradient(double next_stop_pressure, double first_gradient)
{
	double B = cube(first_gradient) / (first_ceiling_pressure.mbar / 1000.0 + first_gradient);
	double C = next_stop_pressure * B;

	double new_gradient = solve_cubic2(B, C);

	if (new_gradient < 0.0)
		report_error("Negative gradient encountered!");
	return new_gradient;
}

double vpmb_tolerated_ambient_pressure(double reference_pressure, int ci)
{
	double n2_gradient, he_gradient, total_gradient;

	if (reference_pressure >= first_ceiling_pressure.mbar / 1000.0 || !first_ceiling_pressure.mbar) {
		n2_gradient = bottom_n2_gradient[ci];
		he_gradient = bottom_he_gradient[ci];
	} else {
		n2_gradient = update_gradient(reference_pressure, bottom_n2_gradient[ci]);
		he_gradient = update_gradient(reference_pressure, bottom_he_gradient[ci]);
	}

	total_gradient = ((n2_gradient * tissue_n2_sat[ci]) + (he_gradient * tissue_he_sat[ci])) / (tissue_n2_sat[ci] + tissue_he_sat[ci]);

	return tissue_n2_sat[ci] + tissue_he_sat[ci] + vpmb_config.other_gases_pressure - total_gradient;
}


double tissue_tolerance_calc(const struct dive *dive, double pressure)
{
	int ci = -1;
	double ret_tolerance_limit_ambient_pressure = 0.0;
	double gf_high = buehlmann_config.gf_high;
	double gf_low = buehlmann_config.gf_low;
	double surface = get_surface_pressure_in_mbar(dive, true) / 1000.0;
	double lowest_ceiling = 0.0;
	double tissue_lowest_ceiling[16];

	if (prefs.deco_mode != VPMB || !in_planner()) {
		for (ci = 0; ci < 16; ci++) {
			tissue_inertgas_saturation[ci] = tissue_n2_sat[ci] + tissue_he_sat[ci];
			buehlmann_inertgas_a[ci] = ((buehlmann_N2_a[ci] * tissue_n2_sat[ci]) + (buehlmann_He_a[ci] * tissue_he_sat[ci])) / tissue_inertgas_saturation[ci];
			buehlmann_inertgas_b[ci] = ((buehlmann_N2_b[ci] * tissue_n2_sat[ci]) + (buehlmann_He_b[ci] * tissue_he_sat[ci])) / tissue_inertgas_saturation[ci];


			/* tolerated = (tissue_inertgas_saturation - buehlmann_inertgas_a) * buehlmann_inertgas_b; */

			tissue_lowest_ceiling[ci] = (buehlmann_inertgas_b[ci] * tissue_inertgas_saturation[ci] - gf_low * buehlmann_inertgas_a[ci] * buehlmann_inertgas_b[ci]) /
						     ((1.0 - buehlmann_inertgas_b[ci]) * gf_low + buehlmann_inertgas_b[ci]);
			if (tissue_lowest_ceiling[ci] > lowest_ceiling)
				lowest_ceiling = tissue_lowest_ceiling[ci];
			if (!buehlmann_config.gf_low_at_maxdepth) {
				if (lowest_ceiling > gf_low_pressure_this_dive)
					gf_low_pressure_this_dive = lowest_ceiling;
			}
		}
		for (ci = 0; ci < 16; ci++) {
			double tolerated;

			if ((surface / buehlmann_inertgas_b[ci] + buehlmann_inertgas_a[ci] - surface) * gf_high + surface <
			    (gf_low_pressure_this_dive / buehlmann_inertgas_b[ci] + buehlmann_inertgas_a[ci] - gf_low_pressure_this_dive) * gf_low + gf_low_pressure_this_dive)
				tolerated = (-buehlmann_inertgas_a[ci] * buehlmann_inertgas_b[ci] * (gf_high * gf_low_pressure_this_dive - gf_low * surface) -
					     (1.0 - buehlmann_inertgas_b[ci]) * (gf_high - gf_low) * gf_low_pressure_this_dive * surface +
					     buehlmann_inertgas_b[ci] * (gf_low_pressure_this_dive - surface) * tissue_inertgas_saturation[ci]) /
					    (-buehlmann_inertgas_a[ci] * buehlmann_inertgas_b[ci] * (gf_high - gf_low) +
					     (1.0 - buehlmann_inertgas_b[ci]) * (gf_low * gf_low_pressure_this_dive - gf_high * surface) +
					     buehlmann_inertgas_b[ci] * (gf_low_pressure_this_dive - surface));
			else
				tolerated = ret_tolerance_limit_ambient_pressure;


			tolerated_by_tissue[ci] = tolerated;

			if (tolerated >= ret_tolerance_limit_ambient_pressure) {
				ci_pointing_to_guiding_tissue = ci;
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
				double tolerated = vpmb_tolerated_ambient_pressure(reference_pressure, ci);
				if (tolerated >= ret_tolerance_limit_ambient_pressure) {
					ci_pointing_to_guiding_tissue = ci;
					ret_tolerance_limit_ambient_pressure = tolerated;
				}
				tolerated_by_tissue[ci] = tolerated;
			}
		// We are doing ok if the gradient was computed within ten centimeters of the ceiling.
		} while (fabs(ret_tolerance_limit_ambient_pressure - reference_pressure) > 0.01);
	}
	return ret_tolerance_limit_ambient_pressure;
}

/*
 * Return buelman factor for a particular period and tissue index.
 *
 * We cache the last factor, since we commonly call this with the
 * same values... We have a special "fixed cache" for the one second
 * case, although I wonder if that's even worth it considering the
 * more general-purpose cache.
 */
struct factor_cache {
	int last_period;
	double last_factor;
};

double n2_factor(int period_in_seconds, int ci)
{
	static struct factor_cache cache[16];

	if (period_in_seconds == 1)
		return buehlmann_N2_factor_expositon_one_second[ci];

	if (period_in_seconds != cache[ci].last_period) {
		cache[ci].last_period = period_in_seconds;
		cache[ci].last_factor = 1 - pow(2.0, -period_in_seconds / (buehlmann_N2_t_halflife[ci] * 60));
	}

	return cache[ci].last_factor;
}

double he_factor(int period_in_seconds, int ci)
{
	static struct factor_cache cache[16];

	if (period_in_seconds == 1)
		return buehlmann_He_factor_expositon_one_second[ci];

	if (period_in_seconds != cache[ci].last_period) {
		cache[ci].last_period = period_in_seconds;
		cache[ci].last_factor = 1 - pow(2.0, -period_in_seconds / (buehlmann_He_t_halflife[ci] * 60));
	}

	return cache[ci].last_factor;
}

double calc_surface_phase(double surface_pressure, double he_pressure, double n2_pressure, double he_time_constant, double n2_time_constant)
{
	double inspired_n2 = (surface_pressure - ((in_planner() && (prefs.deco_mode == VPMB)) ? WV_PRESSURE_SCHREINER : WV_PRESSURE)) * NITROGEN_FRACTION;

	if (n2_pressure > inspired_n2)
		return (he_pressure / he_time_constant + (n2_pressure - inspired_n2) / n2_time_constant) / (he_pressure + n2_pressure - inspired_n2);

	if (he_pressure + n2_pressure >= inspired_n2){
		double gradient_decay_time = 1.0 / (n2_time_constant - he_time_constant) * log ((inspired_n2 - n2_pressure) / he_pressure);
		double gradients_integral = he_pressure / he_time_constant * (1.0 - exp(-he_time_constant * gradient_decay_time)) + (n2_pressure - inspired_n2) / n2_time_constant * (1.0 - exp(-n2_time_constant * gradient_decay_time));
		return gradients_integral / (he_pressure + n2_pressure - inspired_n2);
	}

	return 0;
}

void vpmb_start_gradient()
{
	int ci;

	for (ci = 0; ci < 16; ++ci) {
		initial_n2_gradient[ci] = bottom_n2_gradient[ci] = 2.0 * (vpmb_config.surface_tension_gamma / vpmb_config.skin_compression_gammaC) * ((vpmb_config.skin_compression_gammaC - vpmb_config.surface_tension_gamma) / n2_regen_radius[ci]);
		initial_he_gradient[ci] = bottom_he_gradient[ci] = 2.0 * (vpmb_config.surface_tension_gamma / vpmb_config.skin_compression_gammaC) * ((vpmb_config.skin_compression_gammaC - vpmb_config.surface_tension_gamma) / he_regen_radius[ci]);
	}
}

void vpmb_next_gradient(double deco_time, double surface_pressure)
{
	int ci;
	double n2_b, n2_c;
	double he_b, he_c;
	double desat_time;
	deco_time /= 60.0;

	for (ci = 0; ci < 16; ++ci) {
		desat_time = deco_time + calc_surface_phase(surface_pressure, tissue_he_sat[ci], tissue_n2_sat[ci], log(2.0) / buehlmann_He_t_halflife[ci], log(2.0) / buehlmann_N2_t_halflife[ci]);

		n2_b = initial_n2_gradient[ci] + (vpmb_config.crit_volume_lambda * vpmb_config.surface_tension_gamma) / (vpmb_config.skin_compression_gammaC * desat_time);
		he_b = initial_he_gradient[ci] + (vpmb_config.crit_volume_lambda * vpmb_config.surface_tension_gamma) / (vpmb_config.skin_compression_gammaC * desat_time);

		n2_c = vpmb_config.surface_tension_gamma * vpmb_config.surface_tension_gamma * vpmb_config.crit_volume_lambda * max_n2_crushing_pressure[ci];
		n2_c = n2_c / (vpmb_config.skin_compression_gammaC * vpmb_config.skin_compression_gammaC * desat_time);
		he_c = vpmb_config.surface_tension_gamma * vpmb_config.surface_tension_gamma * vpmb_config.crit_volume_lambda * max_he_crushing_pressure[ci];
		he_c = he_c / (vpmb_config.skin_compression_gammaC * vpmb_config.skin_compression_gammaC * desat_time);

		bottom_n2_gradient[ci] = 0.5 * ( n2_b + sqrt(n2_b * n2_b - 4.0 * n2_c));
		bottom_he_gradient[ci] = 0.5 * ( he_b + sqrt(he_b * he_b - 4.0 * he_c));
	}
}

// A*r^3 - B*r^2 - C == 0
// Solved with the help of mathematica

double solve_cubic(double A, double B, double C)
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


void nuclear_regeneration(double time)
{
	time /= 60.0;
	int ci;
	double crushing_radius_N2, crushing_radius_He;
	for (ci = 0; ci < 16; ++ci) {
		//rm
		crushing_radius_N2 = 1.0 / (max_n2_crushing_pressure[ci] / (2.0 * (vpmb_config.skin_compression_gammaC - vpmb_config.surface_tension_gamma)) + 1.0 / get_crit_radius_N2());
		crushing_radius_He = 1.0 / (max_he_crushing_pressure[ci] / (2.0 * (vpmb_config.skin_compression_gammaC - vpmb_config.surface_tension_gamma)) + 1.0 / get_crit_radius_He());
		//rs
		n2_regen_radius[ci] = crushing_radius_N2 + (get_crit_radius_N2() - crushing_radius_N2) * (1.0 - exp (-time / vpmb_config.regeneration_time));
		he_regen_radius[ci] = crushing_radius_He + (get_crit_radius_He() - crushing_radius_He) * (1.0 - exp (-time / vpmb_config.regeneration_time));
	}
}


// Calculates the nucleons inner pressure during the impermeable period
double calc_inner_pressure(double crit_radius, double onset_tension, double current_ambient_pressure)
{
	double onset_radius = 1.0 / (vpmb_config.gradient_of_imperm / (2.0 * (vpmb_config.skin_compression_gammaC - vpmb_config.surface_tension_gamma)) + 1.0 / crit_radius);


	double A = current_ambient_pressure - vpmb_config.gradient_of_imperm + (2.0 * (vpmb_config.skin_compression_gammaC - vpmb_config.surface_tension_gamma)) / onset_radius;
	double B = 2.0 * (vpmb_config.skin_compression_gammaC - vpmb_config.surface_tension_gamma);
	double C = onset_tension * pow(onset_radius, 3);

	double current_radius = solve_cubic(A, B, C);

	return onset_tension * onset_radius * onset_radius * onset_radius / (current_radius * current_radius * current_radius);
}

// Calculates the crushing pressure in the given moment. Updates crushing_onset_tension and critical radius if needed
void calc_crushing_pressure(double pressure)
{
	int ci;
	double gradient;
	double gas_tension;
	double n2_crushing_pressure, he_crushing_pressure;
	double n2_inner_pressure, he_inner_pressure;

	for (ci = 0; ci < 16; ++ci) {
		gas_tension = tissue_n2_sat[ci] + tissue_he_sat[ci] + vpmb_config.other_gases_pressure;
		gradient = pressure - gas_tension;

		if (gradient <= vpmb_config.gradient_of_imperm) {	// permeable situation
			n2_crushing_pressure = he_crushing_pressure = gradient;
			crushing_onset_tension[ci] = gas_tension;
		}
		else {	// impermeable
			if (max_ambient_pressure >= pressure)
				return;

			n2_inner_pressure = calc_inner_pressure(get_crit_radius_N2(), crushing_onset_tension[ci], pressure);
			he_inner_pressure = calc_inner_pressure(get_crit_radius_He(), crushing_onset_tension[ci], pressure);

			n2_crushing_pressure = pressure - n2_inner_pressure;
			he_crushing_pressure = pressure - he_inner_pressure;
		}
		max_n2_crushing_pressure[ci] = MAX(max_n2_crushing_pressure[ci], n2_crushing_pressure);
		max_he_crushing_pressure[ci] = MAX(max_he_crushing_pressure[ci], he_crushing_pressure);
	}
	max_ambient_pressure = MAX(pressure, max_ambient_pressure);
}

/* add period_in_seconds at the given pressure and gas to the deco calculation */
void add_segment(double pressure, const struct gasmix *gasmix, int period_in_seconds, int ccpo2, const struct dive *dive, int sac)
{
	int ci;
	struct gas_pressures pressures;

	fill_pressures(&pressures, pressure - ((in_planner() && (prefs.deco_mode == VPMB)) ? WV_PRESSURE_SCHREINER : WV_PRESSURE),
		       gasmix, (double) ccpo2 / 1000.0, dive->dc.divemode);

	if (buehlmann_config.gf_low_at_maxdepth && pressure > gf_low_pressure_this_dive)
		gf_low_pressure_this_dive = pressure;

	for (ci = 0; ci < 16; ci++) {
		double pn2_oversat = pressures.n2 - tissue_n2_sat[ci];
		double phe_oversat = pressures.he - tissue_he_sat[ci];
		double n2_f = n2_factor(period_in_seconds, ci);
		double he_f = he_factor(period_in_seconds, ci);
		double n2_satmult = pn2_oversat > 0 ? buehlmann_config.satmult : buehlmann_config.desatmult;
		double he_satmult = phe_oversat > 0 ? buehlmann_config.satmult : buehlmann_config.desatmult;

		tissue_n2_sat[ci] += n2_satmult * pn2_oversat * n2_f;
		tissue_he_sat[ci] += he_satmult * phe_oversat * he_f;
	}
	if(prefs.deco_mode == VPMB && in_planner())
		calc_crushing_pressure(pressure);
	return;
}

void dump_tissues()
{
	int ci;
	printf("N2 tissues:");
	for (ci = 0; ci < 16; ci++)
		printf(" %6.3e", tissue_n2_sat[ci]);
	printf("\nHe tissues:");
	for (ci = 0; ci < 16; ci++)
		printf(" %6.3e", tissue_he_sat[ci]);
	printf("\n");
}

void clear_deco(double surface_pressure)
{
	int ci;
	for (ci = 0; ci < 16; ci++) {
		tissue_n2_sat[ci] = (surface_pressure - ((in_planner() && (prefs.deco_mode == VPMB)) ? WV_PRESSURE_SCHREINER : WV_PRESSURE)) * N2_IN_AIR / 1000;
		tissue_he_sat[ci] = 0.0;
		max_n2_crushing_pressure[ci] = 0.0;
		max_he_crushing_pressure[ci] = 0.0;
		n2_regen_radius[ci] = get_crit_radius_N2();
		he_regen_radius[ci] = get_crit_radius_He();
	}
	gf_low_pressure_this_dive = surface_pressure;
	if (!buehlmann_config.gf_low_at_maxdepth)
		gf_low_pressure_this_dive += buehlmann_config.gf_low_position_min;
	max_ambient_pressure = 0.0;
}

void cache_deco_state(char **cached_datap)
{
	char *data = *cached_datap;

	if (!data) {
		data = malloc(2 * TISSUE_ARRAY_SZ + sizeof(double) + sizeof(int));
		*cached_datap = data;
	}
	memcpy(data, tissue_n2_sat, TISSUE_ARRAY_SZ);
	data += TISSUE_ARRAY_SZ;
	memcpy(data, tissue_he_sat, TISSUE_ARRAY_SZ);
	data += TISSUE_ARRAY_SZ;
	memcpy(data, &gf_low_pressure_this_dive, sizeof(double));
	data += sizeof(double);
	memcpy(data, &ci_pointing_to_guiding_tissue, sizeof(int));
}

void restore_deco_state(char *data)
{
	memcpy(tissue_n2_sat, data, TISSUE_ARRAY_SZ);
	data += TISSUE_ARRAY_SZ;
	memcpy(tissue_he_sat, data, TISSUE_ARRAY_SZ);
	data += TISSUE_ARRAY_SZ;
	memcpy(&gf_low_pressure_this_dive, data, sizeof(double));
	data += sizeof(double);
	memcpy(&ci_pointing_to_guiding_tissue, data, sizeof(int));
}

unsigned int deco_allowed_depth(double tissues_tolerance, double surface_pressure, struct dive *dive, bool smooth)
{
	unsigned int depth;
	double pressure_delta;

	/* Avoid negative depths */
	pressure_delta = tissues_tolerance > surface_pressure ? tissues_tolerance - surface_pressure : 0.0;

	depth = rel_mbar_to_depth(pressure_delta * 1000, dive);

	if (!smooth)
		depth = ceil(depth / DECO_STOPS_MULTIPLIER_MM) * DECO_STOPS_MULTIPLIER_MM;

	if (depth > 0 && depth < buehlmann_config.last_deco_stop_in_mtr * 1000)
		depth = buehlmann_config.last_deco_stop_in_mtr * 1000;

	return depth;
}

void set_gf(short gflow, short gfhigh, bool gf_low_at_maxdepth)
{
	if (gflow != -1)
		buehlmann_config.gf_low = (double)gflow / 100.0;
	if (gfhigh != -1)
		buehlmann_config.gf_high = (double)gfhigh / 100.0;
	buehlmann_config.gf_low_at_maxdepth = gf_low_at_maxdepth;
}
